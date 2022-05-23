/*
How to use db_funcs:
1. in main.cpp #include "db_funcs.h"
2. in main.cpp, in init()
	while (network_connected != 1) { vTaskDelay(2000 / portTICK_PERIOD_MS); }
	strcpy(dbnam ,(char *)"c10mm");
	strcpy(pg_server_ip ,(char *)"192.168.178.100");
	
2.a	pqConnection = new TpgConnection(); // Verbindungsobject erzeugen
2.b	if ( pqConnection->db_connect( pg_server_ip, srvpo, dbnam, PGUser, PGPassword, PGCharset) == 0) //verbinden	
2.c	  query1 = new TResultSet( pqConnection->PGconn );
2.d	  query1->db_query("select type, state from items limit 10;");
2.4	  for (int iRow = 0; iRow < query1->NumRows-1; iRow++)
	  {
	     std::cout << query1->Zeile[iRow].Spalte[0] << " | ";
	     std::cout << query1->Zeile[iRow].Spalte[1] << " | ";
	     printf("\n");
	  }	
	
3.	  query1->db_query("select bla bla;"); //naechste Abfrage
	  //oder neues Abfrage-Object erstellen: query2 = new TResultSet( pqConnection->PGconn );
	  
4. in main.cpp, main_loop() 
    db_loop(); //db_loop regelmaessig aufrufen, damit NOTIFY Nachrichten verarbeitet werden.
    
    
5. Auf Notification reagieren: Vor der mail() Schleife die NOTIFIY Nachrichten registrieren    
	db_query("listen door_open"); //register listen handler for NOTIFY door_open <door_no>
	db_query("listen door_close"); //register listen handler for NOTIFY door_close <door_no>
    
*/
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_netif.h"
#include "esp_eth.h"
#include "esp_log.h"
#include "SimplePgSQL.h"

//For ResultSet:
#include "db_funcs.h"
#include <vector>
#include <iostream>
#include <string>

static int pg_flags = 0;

// PGSQL
#define PGBufferSize				16384
#define PGCharset					"utf-8"
#define PGUser						"micha"
#define PGPassword					"micha"
const TickType_t delay1ms = 1 / portTICK_PERIOD_MS;
const TickType_t delay10ms = 10 / portTICK_PERIOD_MS;
const TickType_t delay100ms = 100 / portTICK_PERIOD_MS;

//static PGconnection *PGconn = NULL;
static unsigned char PGbuffer[PGBufferSize];
static char lastNotify[40];

static const char *PGTAG = "SCALADIS-PG";

static unsigned char pg_lastError_string[80];

//TResultSet ResultSetObject(1);

TpgConnection::TpgConnection()
{
   PGconn = NULL;
}


int TpgConnection::db_connect(const char *pg_server_ip, int pg_port, const char *s_db_name, const char *s_db_user, const char *s_db_passwd, const char *s_db_charset)
{
//CONNECTION_OK, CONNECTION_BAD, CONNECTION_NEEDED, /* setDbLogin() needed */
//CONNECTION_AWAITING_RESPONSE, /* Waiting for a response from the postmaster.        */
//	CONNECTION_AUTH_OK
   int loop_fuse = 0; 
   ESP_LOGI(PGTAG,"db_connect Enter");
   PGconn = new PGconnection(pg_flags, PGbuffer, PGBufferSize); 
   if (PGconn == NULL) { ESP_LOGE(PGTAG, "error creating PGconn object"); } 
   
   last_pgError = PGconn->setDbLogin(pg_server_ip, pg_port, s_db_name, s_db_user, s_db_passwd, s_db_charset);
   
   while ( (last_pgError != CONNECTION_OK) || (last_pgError != CONNECTION_BAD)
    || (last_pgError != CONNECTION_NEEDED)  )
   {
       last_pgError = PGconn->status();
       vTaskDelay(delay10ms); 
       loop_fuse++;
       if (loop_fuse > 100) { ESP_LOGE(PGTAG, "PGsetDbLogin break after 10 attempts. "); break; }
   }     
   
   //ResultSetObject.Zeile.resize(1);
      
   switch (last_pgError) {
	case CONNECTION_NEEDED:  { ESP_LOGE(PGTAG,"connection needed: %s", PGconn->getMessage()); return -1; }
	case CONNECTION_OK:      { ESP_LOGI(PGTAG,"db_connect sucessful (OK) to db %s", s_db_name); return 0; }
	case CONNECTION_BAD:     { ESP_LOGE(PGTAG,"ERROR: %s", PGconn->getMessage()); return -1; }
	case CONNECTION_AWAITING_RESPONSE: { ESP_LOGI(PGTAG,"ERROR AWAITING_RESPONSE: %s", PGconn->getMessage()); return -1; }
	case CONNECTION_AUTH_OK: { ESP_LOGI(PGTAG,"db_connect sucessful (AUTH_OK) to db %s", s_db_name); return 0; }
   }
  
   
   ESP_LOGI(PGTAG,"ResultSet created");
   
   ESP_LOGI(PGTAG,"db_connect EXIT. last_pgError=%d", last_pgError);
   return 0;
}

int TpgConnection::db_disconnect(void)
{
   ESP_LOGI(PGTAG,"db_disconnect ENTER");
   if (PGconn == NULL) { ESP_LOGE(PGTAG, "error destroy PGconn object. PGconn = NULL"); return -1;  }
   PGconn->close();
   delete PGconn;
   PGconn = NULL;
   ESP_LOGI(PGTAG,"db_disconnect EXIT"); 
   return 0;
}

void TpgConnection::db_loop(void)
{
//     bool readyToQuery = false;
     char *msg = NULL;
     last_pgError = PGconn->status();
     if (last_pgError == CONNECTION_BAD || last_pgError == CONNECTION_NEEDED) 
     {
          ESP_LOGE(PGTAG, "ERROR: %s", PGconn->getMessage() );
     } else
     {
        last_pgError = PGconn->getData();
        if (last_pgError < 0) 
        {
            ESP_LOGE(PGTAG, "ERROR: %s", PGconn->getMessage() );
        } else
           {
              if (last_pgError > 0)
              {
                //some data arrive
                msg = PGconn->getMessage();
                if (msg != NULL)
                {
                   sprintf( lastNotify, msg); //copy message, to save the NOTIFY
                   ESP_LOGI(PGTAG,"NOTIFY %s", lastNotify);
                }
              }
           }
     } 
}

// ********************************************************************************************

TResSetRows::TResSetRows() 
{
    Spalte.resize(1); 
    //ESP_LOGE(PGTAG, "--------------------------> TResSetRows constructor called");
}


TResSetRows::~TResSetRows() 
{
    Spalte.resize(0);
    //ESP_LOGE(PGTAG, "--------------------------> TResSetRows destructor called");
}

TResultSet::TResultSet(PGconnection *PGconnFromCon)
{
   PGconn = PGconnFromCon;
   if (PGconnFromCon == NULL)
   {
      ESP_LOGE(PGTAG, "ERROR"); 
      ESP_LOGE(PGTAG, "TResultSet::TResultSet(PGconnection *PGconnFromCon) -> PGconnFromCon ist NULL!!!!!!");
      ESP_LOGE(PGTAG, ""); 
   }
   Zeile.resize(1);
   NumRows = 0;
   //ESP_LOGE(PGTAG, "--------------------------> TResultSet constructor called"); 
}

TResultSet::~TResultSet()
{
  //Speicher aufraeumen
  ESP_LOGI(PGTAG,"\nTResultSet Destructor called"); //fflush (stdout);
}

int TResultSet::db_query(const char *sql)
{
   int last_pgError;
   bool queryEnded;
   
//   ESP_LOGI(PGTAG,"db_query ENTER: %s", sql); fflush (stdout);
    NumRows = 0;//reset NumRows.
   
   //sprintf(lbuf, "SELECT name,gname FROM accounts WHERE id=(cast(x'%s' AS int));", info);
   last_pgError = PGconn->execute(sql);
   if (last_pgError < 0)
   {
      printf("ERROR: TResultSet::db_query: PGexecute: %s", PGconn->getMessage());
      return last_pgError;
   } else
   {
        vTaskDelay(1);
        queryEnded = false;
        while (queryEnded == false)
        {
            last_pgError = PGconn->getData();
	      if (last_pgError & PG_RSTAT_HAVE_COLUMNS) { process_db_col(); }
	      if (last_pgError & PG_RSTAT_HAVE_ROW)     { process_db_row(); }
	      if (last_pgError & PG_RSTAT_HAVE_SUMMARY) { process_db_summary();  }
	      if (last_pgError & PG_RSTAT_HAVE_MESSAGE) { process_db_haveMsg();  }
	      if (last_pgError & PG_RSTAT_READY)        { process_db_ready();  }
	      if (last_pgError & PG_RSTAT_COMMAND_SENT) { process_db_cmd_send(); }
	      if (last_pgError & PG_RSTAT_HAVE_ERROR)   { process_db_have_error(); 
	        printf("\n Error from Postgres: ");
	        if ( PGconn->getMessage() != NULL ) { printf("---> %s\n", PGconn->getMessage() ); }
	        fflush (stdout);
	        queryEnded = true;  }
	      if (last_pgError & PG_RSTAT_HAVE_NOTICE)  { process_db_have_notice(); }   
	           
	     if (last_pgError < 0) 
	     {
	        queryEnded = true;  
	        printf("Get Data Error: %d\n", last_pgError );
	        if ( PGconn->getMessage() != NULL ) { printf("---> %s\n", PGconn->getMessage() ); }
	        fflush (stdout);
	     }  
	     if (last_pgError == CONNECTION_BAD || last_pgError == CONNECTION_NEEDED) 
     	     {
     	        queryEnded = true;
     	        //seem to be the normal end. No Error Message. 
                //ESP_LOGE(PGTAG, "ERROR: CONNECTION BAD or NEEDED:" );
                //if ( PGconn->getMessage() != NULL ) { ESP_LOGE(PGTAG, "----> %s", PGconn->getMessage() ); }
             }
             if (last_pgError == 0) { queryEnded = true; //no more data }      
        }


       } //while (queryEnded == false)
   }           
//   ESP_LOGI(PGTAG,"db_query EXIT"); fflush (stdout);
   return 0;  
}

int TResultSet::db_exec(const char *sql)
{
   int last_pgError;
   bool queryEnded;
   
//   ESP_LOGI(PGTAG,"db_query ENTER: %s", sql); fflush (stdout);
    NumRows = 0;//reset NumRows.
   
   last_pgError = PGconn->execute(sql);//call old version from 17.02.2022
   if (last_pgError < 0)
   {
      return last_pgError;
   } else
   {
        vTaskDelay(1);
        queryEnded = false;
        while (queryEnded == false)
        {
            last_pgError = PGconn->getData();    
	      if (last_pgError & PG_RSTAT_HAVE_COLUMNS) { process_db_col();  }
	      if (last_pgError & PG_RSTAT_HAVE_ROW)     { process_db_row(); }
	      if (last_pgError & PG_RSTAT_HAVE_SUMMARY) { process_db_summary(); }
	      if (last_pgError & PG_RSTAT_HAVE_MESSAGE) { process_db_haveMsg();  }
	      if (last_pgError & PG_RSTAT_READY)        { process_db_ready();  }
	      if (last_pgError & PG_RSTAT_COMMAND_SENT) { process_db_cmd_send();  }
	      if (last_pgError & PG_RSTAT_HAVE_ERROR)   { 
	        process_db_have_error(); 
	        printf("\n Error from Postgres: ");
	        if ( PGconn->getMessage() != NULL ) { printf("---> %s\n", PGconn->getMessage() ); }
	        fflush (stdout);
	        queryEnded = true;  }
	      if (last_pgError & PG_RSTAT_HAVE_NOTICE)  { process_db_have_notice(); }   
	           
	     if (last_pgError < 0) 
	     {
	        queryEnded = true;  
	        printf("Insert/Update Error: %d\n", last_pgError );
	        if ( PGconn->getMessage() != NULL ) { printf("---> %s\n", PGconn->getMessage() ); }
	        fflush (stdout);
	     }  
	     if (last_pgError == CONNECTION_BAD || last_pgError == CONNECTION_NEEDED) 
     	     {
     	        queryEnded = true;
     	        //seem to be the normal end. No Error Message. 
                //ESP_LOGE(PGTAG, "ERROR: CONNECTION BAD or NEEDED:" );
                //if ( PGconn->getMessage() != NULL ) { ESP_LOGE(PGTAG, "----> %s", PGconn->getMessage() ); }
             }
             if (last_pgError == 0) { queryEnded = true; //no more data }      
        }


       } //while (queryEnded == false)
   }     
   fflush (stdout);
   return 0; 
}


void TResultSet::process_db_col(void)
{
        int nCols;
        int iCurrCol;
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_HAVE_COLS");
	printf("Spalten gefunden: \n" );
	fflush (stdout);	
	
	//iRow = 1; // erste Zeile hat die Spaltennamen!
	CurrentRow = 1;
	nCols = PGconn->nfields();
	this->NumCols = nCols;
	Zeile[0].numCols = nCols;
		
	Zeile.resize( 1 ); //erste Zeile
	Zeile[0].Spalte.resize(nCols);
//	printf("Set Result buffer auf Spalten: %d\n", nCols );
	fflush (stdout);
	//alloc 1 row only, for the column names. More is added later.
	for (iCurrCol = 0; iCurrCol < nCols; iCurrCol++) 
	{
	   if (iCurrCol) printf(" | ");
	   this->Zeile[0].Spalte[iCurrCol] = PGconn->getColumn(iCurrCol);
//           std::cout <<  Zeile[0].Spalte[iCurrCol] ;		
//           fflush (stdout);
        }

	printf("\n==========\n");
        fflush(stdout);	
}

void TResultSet::process_db_row(void)
{
        //char *msg;
        int iRow; 
        int iCurrField;
        std::string sFeld;
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_HAVE_ROW");
        this->CurrentRow++;
        NumRows++;
        iRow = this->CurrentRow;
        this->Zeile.resize( iRow ); //naechste Zeile
        this->Zeile[iRow-1].Spalte.resize( this->Zeile[0].numCols );
        
//        printf("Row %d:", iRow );
        for (iCurrField = 0; iCurrField < PGconn->nfields(); iCurrField++) 
        {
              //printf(" | ");
              //msg = PGconn->getValue( iCurrField );
              //copy the string, to save the result
              if (!PGconn->getValue( iCurrField ) ) 
              { Zeile[iRow-1].Spalte[iCurrField].copy( (char *)"NULL", 0, 4); } 
              else 
              {
                 sFeld = PGconn->getValue( iCurrField );
                  Zeile[iRow-1].Spalte[iCurrField] = sFeld; //assign string to string
              }
              //printf(" %s", msg);fflush(stdout);
//              std::cout <<  Zeile[iRow-1].Spalte[iCurrField] << " | " ;
        } 	
//        std::cout << "\n ";
}





void TResultSet::process_db_summary(void)
{
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_HAVE_SUMMARY");
}

void TResultSet::process_db_haveMsg(void)
{
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_HAVE_MESSAGE");
}

void TResultSet::process_db_ready(void)
{
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_READY");
}

void TResultSet::process_db_cmd_send(void)
{
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_COMMAND_SENT");
}

void TResultSet::process_db_have_error(void)
{
//	ESP_LOGE(PGTAG,"---> PG_RSTAT_HAVE_ERROR");
}

void TResultSet::process_db_have_notice(void)
{
//	ESP_LOGI(PGTAG,"---> PG_RSTAT_HAVE_NOTICE");
}




