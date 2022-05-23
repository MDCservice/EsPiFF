/*
 * db_funcs.h
 *
 *  Created on: 11.01.2022
 *      Author: michi
 */

#ifndef DB_FUNCS_H_
#define DB_FUNCS_H_
//#include "schrank_ant_pcb.h"
#include <vector>
#include <iostream>
#include <string>
#include "SimplePgSQL.h"

class TpgConnection { // rows
    public:
    TpgConnection();
    ~TpgConnection();
    int db_connect(const char *pg_server_ip, int pg_port, const char *s_db_name, const char *s_db_user, const char *s_db_passwd, const char *s_db_charset);
    int db_disconnect(void);
    void db_loop(void); 
    PGconnection *PGconn;
    int last_pgError;
 };


//int db_query(const char *sql);

class TResSetRows { // 1 entry is a collection of string fields
    public:
    TResSetRows(); //constructor
    ~TResSetRows(); //destructor
    std::vector<std::string> Spalte;
    int numCols;
    int CurrCol;

 };

class TResultSet { // rows
    public:
    TResultSet(PGconnection *PGconnFromCon);
    ~TResultSet();
    std::vector<TResSetRows> Zeile;
    int CurrentRow;
    int db_query(const char *sql);
    int db_exec(const char *sql);
    int NumRows;
    int NumCols;
    
    private:
    PGconnection *PGconn;
    void process_db_row(void);
    void process_db_col(void);    
    
    void process_db_summary(void);
    void process_db_haveMsg(void);
    void process_db_ready(void);
    void process_db_cmd_send(void);
    void process_db_have_error(void);
    void process_db_have_notice(void);
 };



#endif /* DB_FUNCS_H_ */
