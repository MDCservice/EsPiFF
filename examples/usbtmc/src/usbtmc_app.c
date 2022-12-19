/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2022 MDC
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#define IDN             "EsPiFF-USBTMC"
#define IDN_QUERY       "*idn?"
#define RST_CMD         "*rst"
#define DAC1_CMD         "sourc1:volt:lev " // SOURCe1:VOLTage:LEVel
#define DAC1_QUERY       "sourc1:volt:lev?" // SOURCe1:VOLTage:LEVel

//todo: add sourc2/3/4/.....

#define ADC1_QUERY       "sens1:volt?"      // SENSe1:VOLTage

//todo: add sens2/3/4/.....

#define GPIO_LEV_CMD    "gpio1:lev "       // GPIO1:LEVel
#define GPIO_LEV_QUERY  "gpio1:lev?"       // GPIO1:LEVel
#define GPIO_DIR_CMD    "gpio1:dir "       // GPIO1:DIRection
#define GPIO_DIR_QUERY  "gpio1:dir?"       // GPIO1:DIRection

//todo: add gpio2/3/4/.....

#define END_RESPONSE    "\n"               // USB488

#include <strings.h>
#include <stdlib.h>     /* atoi */
#include <stdio.h>      /* fprintf */
#include "tusb.h"
#include "bsp/board.h"
#include "main.h"
#include "pico/stdlib.h"
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

char * get_value(char *in_string);
uint32_t adc_get_sample(void);
void ftoa(float num, char *str);

#if (CFG_TUD_USBTMC_ENABLE_488)
static usbtmc_response_capabilities_488_t const
#else
static usbtmc_response_capabilities_t const
#endif
tud_usbtmc_app_capabilities  =
{
    .USBTMC_status = USBTMC_STATUS_SUCCESS,
    .bcdUSBTMC = USBTMC_VERSION,
    .bmIntfcCapabilities =
    {
        .listenOnly = 0,
        .talkOnly = 0,
        .supportsIndicatorPulse = 1
		// there is currently no direct LED addressable on the EsPiFF. Ether ask the ESP32 to light up a LED, or the HAT must provide an LED
    },
    .bmDevCapabilities = {
        .canEndBulkInOnTermChar = 0
    },

#if (CFG_TUD_USBTMC_ENABLE_488)
    .bcdUSB488 = USBTMC_488_VERSION,
    .bmIntfcCapabilities488 =
    {
        .supportsTrigger = 1,
        .supportsREN_GTL_LLO = 0,
        .is488_2 = 1
    },
    .bmDevCapabilities488 =
    {
      .SCPI = 1,
      .SR1 = 0,
      .RL1 = 0,
      .DT1 =0,
    }
#endif
};

#define IEEE4882_STB_QUESTIONABLE (0x08u)
#define IEEE4882_STB_MAV          (0x10u)
#define IEEE4882_STB_SER          (0x20u)
#define IEEE4882_STB_SRQ          (0x40u)

//static const char idn[] = "TinyUSB,Seeeduino Xiao,v1\r\n";
static volatile uint8_t status;

// 0=not query, 1=queried, 2=delay,set(MAV), 3=delay 4=ready?
// (to simulate delay)
static volatile uint16_t queryState = 0;
static volatile uint32_t queryDelayStart;
static volatile uint32_t bulkInStarted;

static volatile bool idnQuery;
static volatile bool rst_cmd;
static volatile bool dac_cmd1, dac_cmd2, dac_cmd3, dac_cmd4;
static volatile bool dac_query1, dac_query2, dac_query3, dac_query4;
static volatile bool adc_query1, adc_query2, adc_query3, adc_query4;
static volatile bool gpio_lev_cmd;
static volatile bool gpio_lev_query;
static volatile bool gpio_dir_cmd;
static volatile bool gpio_dir_query;

static uint32_t resp_delay = 125u; // Adjustable delay, to allow for better testing
static size_t   buffer_len;
static size_t   buffer_tx_ix;      // for transmitting using multiple transfers
static uint8_t  buffer[225];       // A few packets long should be enough.

char adc_voltage_str[10];
char dac_voltage_str[10];
char gpio_lev_str[2];
char gpio_dir_str[5];

// HAT specific implementation:
void gpio_setup(uint8_t ch);
void dac_setup(uint8_t ch);
void adc_setup(uint8_t ch);
uint32_t adc_get_sample(uint8_t ch);
void set_gpio_as_input(uint8_t ch);
void set_gpio_as_output(uint8_t ch);
bool is_gpio_output(uint8_t ch);
uint32_t get_DAC_value(uint8_t ch);
void dac_set_value(uint8_t ch, uint32_t new_val);


static usbtmc_msg_dev_dep_msg_in_header_t rspMsg = {
    .bmTransferAttributes =
    {
      .EOM = 1,
      .UsingTermChar = 0
    }
};

void tud_usbtmc_open_cb(uint8_t interface_id)
{
  (void)interface_id;
  tud_usbtmc_start_bus_read();
}

#if (CFG_TUD_USBTMC_ENABLE_488)
usbtmc_response_capabilities_488_t const *
#else
usbtmc_response_capabilities_t const *
#endif
tud_usbtmc_get_capabilities_cb()
{
  return &tud_usbtmc_app_capabilities;
}


bool tud_usbtmc_msg_trigger_cb(usbtmc_msg_generic_t* msg) {
  (void)msg;
  // Let trigger set the SRQ
  status |= IEEE4882_STB_SRQ;
  return true;
}

bool tud_usbtmc_msgBulkOut_start_cb(usbtmc_msg_request_dev_dep_out const * msgHeader)
{
  (void)msgHeader;
  buffer_len = 0;
  if(msgHeader->TransferSize > sizeof(buffer))
  {

    return false;
  }
  return true;
}

bool tud_usbtmc_msg_data_cb(void *data, size_t len, bool transfer_complete)
{
  // If transfer isn't finished, we just ignore it (for now)

  if(len + buffer_len < sizeof(buffer))
  {
    memcpy(&(buffer[buffer_len]), data, len);
    buffer_len += len;
  }
  else
  {
    return false; // buffer overflow!
  }

  queryState     = transfer_complete;
  idnQuery       = false;
  rst_cmd        = false;
  dac_cmd1        = false;
  dac_cmd2        = false;
  dac_cmd3        = false;
  dac_cmd4        = false;

  dac_query1      = false;
  dac_query2      = false;
  dac_query3      = false;
  dac_query4      = false;

  adc_query1      = false;
  adc_query2      = false;
  adc_query3      = false;
  adc_query4      = false;


  gpio_lev_cmd   = false;
  gpio_lev_query = false;
  gpio_dir_cmd   = false;
  gpio_dir_query = false;

  if(transfer_complete && (len >=4) && !strncasecmp(IDN_QUERY,data,5))
  {
    idnQuery = true;
  }
  else if (transfer_complete && (len >=4) && !strncasecmp(RST_CMD,data,4))
  {
    rst_cmd = true;
    clear_DAC_value();
  }
  else if (transfer_complete && (len >=16) && !strncasecmp(DAC1_CMD,data,16))
  {
    dac_cmd1            = true;
    char *ptr_value    = get_value(data);
    float dac_voltage  = strtof(ptr_value,NULL);
    uint16_t dac_value = (int)( (dac_voltage / DAC_REF_VOLTAGE) * DAC_MAX_VALUE );
    dac_set_value(1, dac_value);
  }
  else if (transfer_complete && (len >= 16) && !strncasecmp(DAC1_QUERY,data,16))
  {
    dac_query1 = true;
    float dac_voltage = (float)get_DAC_value();
    ftoa(dac_voltage,dac_voltage_str);
//    strcat(dac_voltage_str,"\n");
  }
  else if (transfer_complete && (len >= 11) && !strncasecmp(ADC1_QUERY,data,11))
  {
    adc_query1         = true;
    float adc_voltage =(float)(adc_get_sample()) / ADC_MAX_VALUE * ADC_REF_VOLTAGE;
    ftoa(adc_voltage,adc_voltage_str);
  }
  else if (transfer_complete && (len >= 10) && !strncasecmp(GPIO_LEV_CMD,data,10))
  {
    gpio_lev_cmd = true;
    char *ptr_value = get_value(data);
    int gpio_level = atoi(ptr_value);
    if (gpio_level == 1)
    {
      set_gpio_high(1, true);
    }
    else if (gpio_level == 0)
    {
    	set_gpio_high(1, false); // drive low value
    }
  }
  else if (transfer_complete && (len >= 10) && !strncasecmp(GPIO_LEV_QUERY,data,10))
  {
    gpio_lev_query = true;
    if ( is_gpio_high(1) )
    {
      strcpy(gpio_lev_str,"1");
    }
    else
    {
      strcpy(gpio_lev_str,"0");
    }
  }
  else if (transfer_complete && (len >= 10) && !strncasecmp(GPIO_DIR_CMD,data,10))
  {
    gpio_dir_cmd    = true;
    char *ptr_value = get_value(data);
    if (!strncasecmp("IN",ptr_value,2))
    {
    	set_gpio_as_input(1); // PA10 as input
    }
    else if (!strncasecmp("OUT",ptr_value,3))
    {
    	set_gpio_as_output(1); // PA10 as output
    }
  }
  else if (transfer_complete && (len >= 10) && !strncasecmp(GPIO_DIR_QUERY,data,10))
  {
    gpio_dir_query = true;
    if (is_gpio_output() )
    {
      strcpy(gpio_dir_str,"OUT");
    }
    else
    {
      strcpy(gpio_dir_str,"IN");
    }
  }

  if(transfer_complete && !strncasecmp("delay ",data,5))
  {
    queryState = 0;
    int d = atoi((char*)data + 5);
    if(d > 10000)
      d = 10000;
    if(d<0)
      d=0;
    resp_delay = (uint32_t)d;
  }
  tud_usbtmc_start_bus_read();
  return true;
}

bool tud_usbtmc_msgBulkIn_complete_cb()
{
  if((buffer_tx_ix == buffer_len) || idnQuery) // done
  {
    status &= (uint8_t)~(IEEE4882_STB_MAV); // clear MAV
    queryState = 0;
    bulkInStarted = 0;
    buffer_tx_ix = 0;
  }
  tud_usbtmc_start_bus_read();

  return true;
}

static unsigned int msgReqLen;

bool tud_usbtmc_msgBulkIn_request_cb(usbtmc_msg_request_dev_dep_in const * request)
{
  rspMsg.header.MsgID = request->header.MsgID,
  rspMsg.header.bTag = request->header.bTag,
  rspMsg.header.bTagInverse = request->header.bTagInverse;
  msgReqLen = request->TransferSize;

#ifdef xDEBUG
  uart_tx_str_sync("MSG_IN_DATA: Requested!\r\n");
#endif
  if(queryState == 0 || (buffer_tx_ix == 0))
  {
    TU_ASSERT(bulkInStarted == 0);
    bulkInStarted = 1;

    // > If a USBTMC interface receives a Bulk-IN request prior to receiving a USBTMC command message
    //   that expects a response, the device must NAK the request (*not stall*)
  }
  else
  {
    size_t txlen = tu_min32(buffer_len-buffer_tx_ix,msgReqLen);
    tud_usbtmc_transmit_dev_msg_data(&buffer[buffer_tx_ix], txlen,
        (buffer_tx_ix+txlen) == buffer_len, false);
    buffer_tx_ix += txlen;
  }
  // Always return true indicating not to stall the EP.
  return true;
}

void usbtmc_app_task_iter(void) {
  switch(queryState) {
  case 0:
    break;
  case 1:
    queryDelayStart = board_millis();
    queryState = 2;
    break;
  case 2:
    if( (board_millis() - queryDelayStart) > resp_delay) {
      queryDelayStart = board_millis();
      queryState=3;
      status |= 0x10u; // MAV
      status |= 0x40u; // SRQ
    }
    break;
  case 3:
    if( (board_millis() - queryDelayStart) > resp_delay) {
      queryState = 4;
    }
    break;
  case 4: // time to transmit;
    if(bulkInStarted && (buffer_tx_ix == 0)) {
      if(idnQuery)
      {
        tud_usbtmc_transmit_dev_msg_data(IDN, tu_min32(sizeof(IDN)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else if (adc_query1)
      {
        tud_usbtmc_transmit_dev_msg_data(adc_voltage_str, tu_min32(sizeof(adc_voltage_str)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else if (dac_query1)
      {
        tud_usbtmc_transmit_dev_msg_data(dac_voltage_str, tu_min32(sizeof(dac_voltage_str)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else if (gpio_lev_query)
      {
        tud_usbtmc_transmit_dev_msg_data(gpio_lev_str, tu_min32(sizeof(gpio_lev_str)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else if (gpio_dir_query)
      {
        tud_usbtmc_transmit_dev_msg_data(gpio_dir_str, tu_min32(sizeof(gpio_dir_str)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else if (rst_cmd || dac_cmd1 || gpio_lev_cmd || gpio_dir_cmd)
      { 
        tud_usbtmc_transmit_dev_msg_data(END_RESPONSE, tu_min32(sizeof(END_RESPONSE)-1,msgReqLen),true,false);
        queryState    = 0;
        bulkInStarted = 0;
      }
      else
      {
        buffer_tx_ix = tu_min32(buffer_len,msgReqLen);
        tud_usbtmc_transmit_dev_msg_data(buffer, buffer_tx_ix, buffer_tx_ix == buffer_len, false);
      }

      // MAV is cleared in the transfer complete callback.
    }
    break;
  default:
    TU_ASSERT(false,);
    return;
  }
}

bool tud_usbtmc_initiate_clear_cb(uint8_t *tmcResult)
{
  *tmcResult = USBTMC_STATUS_SUCCESS;
  queryState = 0;
  bulkInStarted = false;
  status = 0;
  return true;
}

bool tud_usbtmc_check_clear_cb(usbtmc_get_clear_status_rsp_t *rsp)
{
  queryState = 0;
  bulkInStarted = false;
  status = 0;
  buffer_tx_ix = 0u;
  buffer_len = 0u;
  rsp->USBTMC_status = USBTMC_STATUS_SUCCESS;
  rsp->bmClear.BulkInFifoBytes = 0u;
  return true;
}
bool tud_usbtmc_initiate_abort_bulk_in_cb(uint8_t *tmcResult)
{
  bulkInStarted = 0;
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;
}
bool tud_usbtmc_check_abort_bulk_in_cb(usbtmc_check_abort_bulk_rsp_t *rsp)
{
  (void)rsp;
  tud_usbtmc_start_bus_read();
  return true;
}

bool tud_usbtmc_initiate_abort_bulk_out_cb(uint8_t *tmcResult)
{
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;

}
bool tud_usbtmc_check_abort_bulk_out_cb(usbtmc_check_abort_bulk_rsp_t *rsp)
{
  (void)rsp;
  tud_usbtmc_start_bus_read();
  return true;
}

void tud_usbtmc_bulkIn_clearFeature_cb(void)
{
}
void tud_usbtmc_bulkOut_clearFeature_cb(void)
{
  tud_usbtmc_start_bus_read();
}

// Return status byte, but put the transfer result status code in the rspResult argument.
uint8_t tud_usbtmc_get_stb_cb(uint8_t *tmcResult)
{
  uint8_t old_status = status;
  status = (uint8_t)(status & ~(IEEE4882_STB_SRQ)); // clear SRQ

  *tmcResult = USBTMC_STATUS_SUCCESS;
  // Increment status so that we see different results on each read...

  return old_status;
}

bool tud_usbtmc_indicator_pulse_cb(tusb_control_request_t const * msg, uint8_t *tmcResult)
{
  (void)msg;
  led_indicator_pulse();
  *tmcResult = USBTMC_STATUS_SUCCESS;
  return true;
}

//---------------------------- New Code ----------------------------//

void adc_setup(void) {
  // todo: HAT specific implementation
}

uint32_t adc_get_sample(void) {
  // todo: HAT specific implementation. 
  //return the ADC value as an uint32_t
}

void dac_setup(void) {
  //todo: HAT specific
}

void gpio_setup(void) {
  //todo: HAT specific
}

char * get_value(char *in_string) {
  char *ptr = strrchr(in_string,' ') + 1;
  return ptr;
}

// char *ptr_value = get_value(scpi_string);
// float value     = strtof(ptr_value,NULL);
// char *command   = get_command(scpi_string,ptr_value);

char * get_command(char *in_string, char *ptr_value) {
  uint32_t command_len = ptr_value - in_string - 1;
  char *command = (char *) malloc(command_len +1);
  memcpy(command, in_string, command_len);
  command[command_len] = '\0';
  return command;
}

void ftoa(float num, char *str)
{
  int intpart = num;
  int intdecimal;
  uint32_t i;
  float decimal_part;
  char decimal[20];

  memset(str, 0x0, 20);
  itoa(num, str, 10);

  strcat(str, ".");

  decimal_part = num - intpart;
  intdecimal = decimal_part * 1000;

  if(intdecimal < 0)
  {
    intdecimal = -intdecimal;
  }
  itoa(intdecimal, decimal, 10);
  for(i =0;i < (3 - strlen(decimal));i++)
  {
    strcat(str, "0");
  }
  strcat(str, decimal);
}
