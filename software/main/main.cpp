
#include "main.h"
#include "sdkconfig.h"

#include "SimplePgSQL.h"
#include "MU80X.h"
#include "HMI.h"
#include "etc.h"
#include "uart.h"
#include "iic.h"
#include "config.h"

// PGSQL
#define PGBufferSize				16384
#define PGCharset					"utf-8"
#define PGUser						"mega"
#define PGPassword					"osteoglossum"
static PGconnection *PGconn;
static unsigned char PGbuffer[PGBufferSize];

// ETC
const char *TAG = "SCALADIS";
//static int cnt = 0;
//static int protcnt = 0;
static int serlen;
static int network_connected = 0;
MU80X *UHF;
HMI *LCD;


unsigned long long ReadAuth(void);

/** ETH */
static void eth_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	uint8_t mac_addr[6] = { 0 };
	/* we can get the ethernet driver handle from event data */
	esp_eth_handle_t eth_handle = *(esp_eth_handle_t*) event_data;

	switch (event_id) {
	case ETHERNET_EVENT_CONNECTED:
		esp_eth_ioctl(eth_handle, ETH_CMD_G_MAC_ADDR, mac_addr);
		ESP_LOGI(TAG, "Eth Link Up");
		ESP_LOGI(TAG, "Eth MAC %02x:%02x:%02x:%02x:%02x:%02x", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
		break;
	case ETHERNET_EVENT_DISCONNECTED:
		ESP_LOGI(TAG, "Eth Link Down");
		break;
	case ETHERNET_EVENT_START:
		ESP_LOGI(TAG, "Eth Started");
		break;
	case ETHERNET_EVENT_STOP:
		ESP_LOGI(TAG, "Eth Stopped");
		break;
	default:
		break;
	}
}
static void got_ip_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
	ip_event_got_ip_t *event = (ip_event_got_ip_t*) event_data;
	const esp_netif_ip_info_t *ip_info = &event->ip_info;

	ESP_LOGI(TAG, "Ethernet Got IP Address");
	ESP_LOGI(TAG, "~~~~~~~~~~~");
	ESP_LOGI(TAG, "ETHIP  :" IPSTR, IP2STR(&ip_info->ip));
	ESP_LOGI(TAG, "ETHMASK:" IPSTR, IP2STR(&ip_info->netmask));
	ESP_LOGI(TAG, "ETHGW  :" IPSTR, IP2STR(&ip_info->gw));
	ESP_LOGI(TAG, "~~~~~~~~~~~");
	network_connected = 1;
}
static void ETH_init(int dhcp) {
	ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Create new default instance of esp-netif for Ethernet
    esp_netif_config_t cfg = ESP_NETIF_DEFAULT_ETH();
    esp_netif_t *eth_netif = esp_netif_new(&cfg);

    // Init MAC and PHY configs to default
    eth_mac_config_t mac_config = ETH_MAC_DEFAULT_CONFIG();
    eth_phy_config_t phy_config = ETH_PHY_DEFAULT_CONFIG();

    phy_config.phy_addr = CONFIG_EXAMPLE_ETH_PHY_ADDR;
    phy_config.reset_gpio_num = CONFIG_EXAMPLE_ETH_PHY_RST_GPIO;
    mac_config.smi_mdc_gpio_num = CONFIG_EXAMPLE_ETH_MDC_GPIO;
    mac_config.smi_mdio_gpio_num = CONFIG_EXAMPLE_ETH_MDIO_GPIO;
    esp_eth_mac_t *mac = esp_eth_mac_new_esp32(&mac_config);
    esp_eth_phy_t *phy = esp_eth_phy_new_lan87xx(&phy_config);
    esp_eth_config_t config = ETH_DEFAULT_CONFIG(mac, phy);
    esp_eth_handle_t eth_handle = NULL;
    ESP_ERROR_CHECK(esp_eth_driver_install(&config, &eth_handle));
    ESP_ERROR_CHECK(esp_netif_attach(eth_netif, esp_eth_new_netif_glue(eth_handle)));

	if (dhcp == 0) {
		strcpy(netip, (char*) "192.168.178.32");
		strcpy(netgw, (char*) "192.168.178.1");
		strcpy(netma, (char*) "255.255.255.0");
		strcpy(srvip, (char*) "192.168.178.32");
		srvpo = 5432;
		boxid = 1;
		subbx = 1;
		ReadConfig();

		esp_netif_ip_info_t ip_info;
		memset(&ip_info, 0, sizeof(esp_netif_ip_info_t));
		esp_netif_str_to_ip4((const char*) netip, &ip_info.ip);
		esp_netif_str_to_ip4((const char*) netgw, &ip_info.gw);
		esp_netif_str_to_ip4((const char*) netma, &ip_info.netmask);
		esp_netif_set_ip_info(eth_netif, &ip_info);
	}
	ESP_ERROR_CHECK(esp_eth_start(eth_handle));
}

/** UHF */
void UHF_init(void) {
	int res;
	UHF = new MU80X(UHF_UART_CHANNEL);
	res = UHF->iSetRFRegion();
	res = UHF->iSetPower(30);
	UHF->DumpBuffer(UHF->RecvBuf, res);
	UHF->iClearBuffer();
}
void UHF_loop(void) {
	int i;
	unsigned long long ID;
	for (i = 0; i < 1000; i++) {
		if ((ID=ReadAuth())>0) {
			printf("ID:%lld",ID);fflush(stdout);
		}
		UHF->iBufferInventory(7, 0, 10);
		printf("%d -> Tags in Buffer:%d  Tags seen:%d\n", i, UHF->TagsInBuffer,UHF->TagsSeen);
		fflush(stdout);
		vTaskDelay(10);
	};
	while (0) {
		vTaskDelay(100);
	};
}

unsigned long long ReadAuth(void) {
	char authbuff[1024];
	char *pos;
	int received = 0;
	int timeout = 60;
	serlen = uart_read_bytes(RDR_UART_CHANNEL, authbuff, 256, 1);
	if (serlen > 0) {

		pos = strchr(authbuff, '\n');
		if (pos) {
			*pos = '\0';
		}

		if (strncmp(authbuff, "CONFIG", 6) == 0) { // we have to receive the init file
			printf("Ready to receive config file !\n");
			fflush(stdout);
			while ((received == 0) & (timeout > 0)) {
				serlen = uart_read_bytes(RDR_UART_CHANNEL, authbuff, 1023, 100);
				if (serlen) {
					authbuff[serlen] = 0;
					WriteConfig(authbuff);
					received = 1;
				} else {
					vTaskDelay(100);
					timeout--;
					printf("TO:%d\n", timeout);
					fflush(stdout);
				}
			}
		}
	}
	authbuff[serlen]=0;
	printf("SERLEN:%d->%s\n", serlen,authbuff);
	fflush(stdout);
	return sscanf("%X",authbuff);
}

/** GPIO **/
void IO_init(void) {
	// outputs

#define GPO_BIT_MASK (1ULL << PHY_PWR)
	gpio_config_t o_conf;
	o_conf.intr_type = GPIO_INTR_DISABLE;
	o_conf.mode = GPIO_MODE_OUTPUT;
	o_conf.pin_bit_mask = GPO_BIT_MASK;
	o_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	o_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	gpio_config(&o_conf);
	gpio_set_level((gpio_num_t) PHY_PWR, 1);

	// inputs
	/*
	 #define GPI_BIT_MASK ((1ULL << SWITCH)|(1ULL << SWITCH))
	 gpio_config_t i_conf;
	 i_conf.intr_type = GPIO_INTR_DISABLE;
	 i_conf.mode = GPIO_MODE_INPUT;
	 i_conf.pin_bit_mask = GPI_BIT_MASK;
	 i_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
	 i_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	 gpio_config(&i_conf);
	 */
}
void HMI_init(void) {
	LCD = new HMI(LCD_UART_CHANNEL);
}

/** PGSQL **/
void PGInit(void) {
	PGconn = new PGconnection(0,PGbuffer,PGBufferSize);
}
int getInfo(char *info) {
	int rc, i;
	int pgstat = 0;
	int cnt = 1000;
	char *msg;
	char lbuf[1024];
	rc = PGconn->PGsetDbLogin(srvip, srvpo, dbnam, PGUser, PGPassword, PGCharset);
	printf("Login result:%d\n", rc);
	if (rc < 1)
		return -1;
	pgstat = 1;
	while (cnt > 0) {

		if (pgstat == 1) { // we are connected to srv/db, now we need to rcv the msgs from srv
			rc = PGconn->PGstatus();
			printf("Status:%d\n", rc);
			fflush(stdout);
			if (rc == CONNECTION_BAD || rc == CONNECTION_NEEDED) {
				printf("ERROR: %s", PGconn->PGgetMessage());
				pgstat = -1;
			} else if (rc == CONNECTION_OK) {
				pgstat = 2;
				printf("Ready to submit qry\n");
			}
		}

		if (pgstat == 2) {
			sprintf(lbuf, "SELECT name,gname FROM accounts WHERE id=(cast(x'%s' AS int));", info);
			rc = PGconn->PGexecute(lbuf);
			printf("EXEC result:%d\n", rc);
//                      vTaskDelay(1);
			if (rc == 0) {
				pgstat = 3;
			} else {
				cnt = 0;
			}
		}
		if (pgstat == 3) {
			rc = PGconn->PGgetData();
			//                      printf("RC:%d\n",rc);fflush(stdout);
			if (rc < 0) {
				printf("Get Data Error:%d\n", rc);
				fflush(stdout);
			} else if (rc > 0) {
				if (rc & PG_RSTAT_HAVE_COLUMNS) {
					printf("We got columns !\n");
					fflush(stdout);
					int cols = PGconn->PGnfields();
					printf("Cols: %d\n", cols);
					fflush(stdout);
					/*
					 for (i = 0; i < cols; i++) {
					 if (i) printf(" | ");
					 printf(" %s ",PGconn->PGgetColumn(i));
					 }
					 */
					printf("\n==========\n");
					fflush(stdout);
				} else if (rc & PG_RSTAT_HAVE_ROW) {
					//                                      printf("We got rows !\n");fflush (stdout);
					for (i = 0; i < PGconn->PGnfields(); i++) {
						//                                              if (i) printf(" | ");
						//                                              msg = PGconn->PGgetValue(i);
						sprintf(lbuf, "Info:\r\n%s\r\n%s", PGconn->PGgetValue(1), PGconn->PGgetValue(0));
						//                                              if (!msg) msg = (char *) "NULL";
						//                                              printf(" %s", msg);fflush(stdout);
					}
					//                                      printf("\n");fflush(stdout);
				} else if (rc & PG_RSTAT_HAVE_SUMMARY) {
					printf("Rows affected: ");
					printf("%d\n", PGconn->PGntuples());
				} else if (rc & PG_RSTAT_HAVE_MESSAGE) {
					printf("We got msg !\n");
					fflush(stdout);
					msg = PGconn->PGgetMessage();
					if (!msg)
						msg = (char*) "NULL";
					printf("MSG: %s\n", msg);
					fflush(stdout);
				}
				if (rc & PG_RSTAT_READY) {
					printf("We made it !\n");
					fflush(stdout);
					cnt = 0;
					break;
				}
			}
		} // pgstat==3
		vTaskDelay(1);
		cnt--;
	} // while (cnt>0)
	PGconn->PGclose();
	return rc;
}

static void init(void) {
//	int res;
//	printf("UART-Init:\n\n");
//	UART_init();
//	printf("IO-Init:\n\n");
//	IO_init();
//	printf("ETH-Init:\n\n");
//	ETH_init(0);
//	printf("UHF-Init:\n\n");
//	UHF_init();
//	printf("HMI-Init:\n\n");
//	HMI_init();
	printf("I2C-Init:\n\n");
	ESP_ERROR_CHECK(I2C_init());
//	printf("ADC-Init:\n\n");
//	ESP_ERROR_CHECK(ADC_init());
	ESP_LOGI(TAG, "I2C initialized successfully");

}
static void main_loop() {
	int res1 = 0, res2 = 0;
	printf("Main loop\n");
	fflush(stdout);
//	res1 = LCD->iSendProt(6, (char*) "page 0", 1);
	printf("res:%d\n", res1);
	fflush(stdout);
//	int door=1;

//	UHF_loop();
//	EditConfig();
//	UHF_loop();

	while (1) {
//		ADCReadAll();

		I2C_buf[0] = 0x02;	// write data to output port 1
		I2C_buf[1] = 0x55;	// write data to output port 1
		I2C_buf[2] = 0xAA;	// write data to output port 1
		res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	//	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);
//		I2C_buf[0] = 0x00;	// lese 3 bytes
//		res2 = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

		printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);


		vTaskDelay(500 / portTICK_PERIOD_MS);
		I2C_buf[0] = 0x02;	// write data to output port 1
		I2C_buf[1] = 0xAA;	// write data to output port 1
		I2C_buf[2] = 0x55;	// write data to output port 1
		res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
	//	printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);
//		I2C_buf[0] = 0x00;	// lese 3 bytes
//		res2 = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_PORTEXP1, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);

		printf("\nMaster wrote %d %d %02X\n", res1,res2,I2C_buf[0]);


		vTaskDelay(500 / portTICK_PERIOD_MS);
	}

	while (0) {
		printf("Closed 1:%d\n",DoorIsClosed(LOCK1R));fflush(stdout);
		printf("Closed 2:%d\n",DoorIsClosed(LOCK2R));fflush(stdout);
		printf("Closed 3:%d\n",DoorIsClosed(LOCK3R));fflush(stdout);
		printf("Closed 4:%d\n",DoorIsClosed(LOCK4R));fflush(stdout);
		/*
		res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);

		vTaskDelay(1);

		I2C_buf[0] = 0x05;
		I2C_buf[1] = 0x00;
		res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP, I2C_buf, 2, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);

		I2C_buf[0] = 0x80;	// Setze CMD read port 0
		I2C_buf[1] = 0x00;
		I2C_buf[2] = 0x00;
		res1 = i2c_master_write_to_device(I2C_MASTER_NUM, I2C_PORTEXP, I2C_buf, 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		printf("\nI2C wrote 0x%02X%02X: %d\n", I2C_buf[0], I2C_buf[1], res1);
		I2C_buf[0] = 0x00;	// lese 3 bytes
		res2 = i2c_master_read_from_device(I2C_MASTER_NUM, I2C_PORTEXP, I2C_buf, 3, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
		printf("\nI2C read 0x %02X %02X  %02X: %d\n", I2C_buf[0], I2C_buf[1], I2C_buf[2], res2);
		vTaskDelay(2000 / portTICK_PERIOD_MS);
		printf(" #:S%d R:%d ", res1, res2);
		fflush(stdout);
		*/

		vTaskDelay(10000 / portTICK_PERIOD_MS);
		DoorOpen(1);
//		vTaskDelay(10);
		DoorOpen(2);
		vTaskDelay(10);
		DoorOpen(3);
		vTaskDelay(10);
		DoorOpen(4);
		vTaskDelay(10);
		printf("\n\n\n");fflush(stdout);
	}
}

extern "C" {
void app_main(void) {
	init();
	main_loop();
}
}
