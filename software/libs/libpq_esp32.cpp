/*
 * libpq_esp32.cpp
 *
 *  Created on: 26.02.2022
 *      Author: michi
 *  SPDX-FileCopyrightText: 2022 MDC Service <info@mdc-service.de>
 *  SPDX-License-Identifier: GPL-3.0-or-later
 *  Ported from SimplePgSQL (https://github.com/ethanak/SimplePgSQL) to ESP IDF
 *  original SimplePgSQL:  * Copyright (C) Bohdan R. Rau 2016 <ethanak@polip.com>
 */
 
#include "libpq_esp32.h"



#ifdef ESP32
#define strchr_P strchr
#endif

#ifdef PG_USE_MD5

/**
 * bytesToHex: convert bytes to HEX
 *
 * @param b byte array to con.vert
 * @param s string result
 */

static void
bytesToHex(const uint8_t b[16], char *s)
{
	int q, w;
#ifndef ESP32	
    static PROGMEM const char hex[] = "0123456789abcdef";
	for (q = 0, w = 0; q < 16; q++)
	{
		s[w++] = pgm_read_byte(&hex[(b[q] >> 4) & 0x0F]);
		s[w++] = pgm_read_byte(&hex[b[q] & 0x0F]);
	}
#else
    static const char hex[] = "0123456789abcdef";
	for (q = 0, w = 0; q < 16; q++)
	{
		s[w++] = hex[(b[q] >> 4) & 0x0F];
		s[w++] = hex[b[q] & 0x0F];
	}    
#endif	
	s[w] = '\0';
}

#ifdef ESP32_IDF
#include "esp_log.h" 
#endif

#ifdef ESP32
#include <mbedtls/md5.h>

/**
 * pg_md5_encrypt: md5 encryption for PostgreSQL
 *
 * @param password as unecrypted string
 * @param salt parameter string for the md5 algorithm
 * @param salt_len length of the salt parameter
 * @param outbuf is the resulting md5 encrypted password
 */

static void pg_md5_encrypt(const char *password, char *salt, int salt_len, char *outbuf)
{
    md5_context_t context;
	uint8_t sum[16];
    *outbuf++ = 'm';
    *outbuf++ = 'd';
    *outbuf++ = '5';
    mbedtls_md5_init(&context);
    mbedtls_md5_update_ret(&context, (uint8_t *)password, strlen(password));
    mbedtls_md5_update_ret(&context, (uint8_t *)salt, salt_len);
    mbedtls_md5_finish_ret( &context, sum);
	bytesToHex(sum, outbuf);
}
#elif defined(ESP8266)
#include <md5.h>
/**
 * pg_md5_encrypt: md5 encryption for PostgreSQL
 *
 * @param password as unecrypted string
 * @param salt parameter string for the md5 algorithm
 * @param salt_len length of the salt parameter
 * @param outbuf is the resulting md5 encrypted password
 */
static void pg_md5_encrypt(const char *password, char *salt, int salt_len, char *outbuf)
{
    md5_context_t context;
	uint8_t sum[16];
    *outbuf++ = 'm';
    *outbuf++ = 'd';
    *outbuf++ = '5';
    MD5Init(&context);
    MD5Update(&context, (uint8_t *)password, strlen(password));
    MD5Update(&context, (uint8_t *)salt, salt_len);
    MD5Final(sum, &context);
	bytesToHex(sum, outbuf);
}
#else
//#include <MD5.h>
/**
 * pg_md5_encrypt: md5 encryption for PostgreSQL
 *
 * @param password as unecrypted string
 * @param salt parameter string for the md5 algorithm
 * @param salt_len length of the salt parameter
 * @param outbuf is the resulting md5 encrypted password
 */
static void pg_md5_encrypt(const char *password, char *salt, int salt_len, char *outbuf)
{
	MD5_CTX context;
	uint8_t sum[16];
    *outbuf++ = 'm';
    *outbuf++ = 'd';
    *outbuf++ = '5';

    MD5::MD5Init(&context);
    MD5::MD5Update(&context, (uint8_t *)password, strlen(password));
    MD5::MD5Update(&context, (uint8_t *)salt, salt_len);
    MD5::MD5Final(sum, &context);
	bytesToHex(sum, outbuf);
}
#endif
#endif

#define MD5_PASSWD_LEN	35
#define AUTH_REQ_OK			0	/* User is authenticated  */
#define AUTH_REQ_PASSWORD	3	/* Password */
#define AUTH_REQ_MD5		5	/* md5 password */

#ifndef ESP32_IDF
static PROGMEM const char EM_OOM [] = "Out of memory";
static PROGMEM const char EM_READ [] = "Backend read error";
static PROGMEM const char EM_WRITE [] = "Backend write error";
static PROGMEM const char EM_CONN [] = "Cannot connect to server";
static PROGMEM const char EM_SYNC [] = "Backend out of sync";
static PROGMEM const char EM_INTR [] = "Internal error";
static PROGMEM const char EM_UAUTH [] = "Unsupported auth method";
static PROGMEM const char EM_BIN [] = "Binary format not supported";
static PROGMEM const char EM_EXEC [] = "Previous execution not finished";
static PROGMEM const char EM_PASSWD [] = "Password required";
static PROGMEM const char EM_EMPTY [] = "Query is empty";
static PROGMEM const char EM_FORMAT [] = "Illegal formatting character";
#else
static  const char EM_OOM [] = "Out of memory";
static  const char EM_READ [] = "Backend read error";
static  const char EM_WRITE [] = "Backend write error";
static  const char EM_CONN [] = "Cannot connect to server";
static  const char EM_SYNC [] = "Backend out of sync";
static  const char EM_INTR [] = "Internal error";
static  const char EM_UAUTH [] = "Unsupported auth method";
static  const char EM_BIN [] = "Binary format not supported";
static  const char EM_EXEC [] = "Previous execution not finished";
static  const char EM_PASSWD [] = "Password required";
static  const char EM_EMPTY [] = "Query is empty";
static  const char EM_FORMAT [] = "Illegal formatting character";

#define PGTAG "PGSQL"
#endif

#ifndef ESP32_IDF  
/**
 * PGconnection constructor for Arduino
 *
 * @param c the Arduino Client pointer
 * @param flags for the connection. See the postgresql libpq documentation for more details
 * @param memory specify buffer size
 * @param foreignBuffer specify is internal or external memory is used.
 */  
PGconnection::PGconnection(Client *c,
        int flags,
        int memory,
        char *foreignBuffer)
{
    conn_status = CONNECTION_NEEDED;
    client = c;
    Buffer = foreignBuffer;
    _passwd = NULL;
    _flags = flags & ~PG_FLAG_STATIC_BUFFER;

    if (memory <= 0) bufSize = PG_BUFFER_SIZE;
    else bufSize = memory;
    if (foreignBuffer) {
        _flags |= PG_FLAG_STATIC_BUFFER;
    }
}
#else

/**
 * PGconnection constructor for ESP IDF 
 *
 * @param flags for the connection. See the postgresql libpq documentation for more details
 * @param _Buffer points to the external allocated buffer
 * @param _bufSize specify the buffer size.
 */  
PGconnection::PGconnection(const int flags, const unsigned char *_Buffer, const int _bufSize) {
	conn_status = CONNECTION_NEEDED;
	_passwd = NULL;
	_user = NULL;
	Buffer = (char *) _Buffer;
	bufSize = _bufSize;
	bufPos = 0;
	result_status=0;
	_available=0;
	_nfields=0;
	_ntuples=0;
	_flags=0;
	
    conn_status = CONNECTION_NEEDED;
    _flags = flags & ~PG_FLAG_STATIC_BUFFER;   
    bufSize = PG_BUFFER_SIZE; 	
}
#endif

#ifndef ESP32_IDF  

/**
 * PGconnection setDbLogin for Arduino 
 *
 * @param IPAddress for the connection. 
 * @param user name, the PostgreSQL user name.
 * @param passwd the PostgreSQL password.
 * @param db the name of the database.
 * @param charset the used char set
 * @param port the TCP/IP port, typically 5432
 * @returns the connection status
 */ 

int PGconnection::setDbLogin(IPAddress server,
    const char *user,
    const char *passwd,
    const char *db,
    const char *charset,
    int port)
{

    char	   *startpacket;
    int			packetlen;
    int len;

    close();
    if (!db) db = user;
    len = strlen(user) + 1;
    if (passwd) {
        len += strlen(passwd) + 1;
    }
    _user = (char *)malloc(len);
    strcpy(_user, user);
    if (passwd) {
        _passwd = _user + strlen(user) + 1;
        strcpy(_passwd, passwd);
    }
    else {
        _passwd = NULL;
    }
    if (!Buffer) Buffer = (char *) malloc(bufSize);
    byte connected = client -> connect(server, port);
    if (!connected) {
        setMsg_P(EM_CONN, PG_RSTAT_HAVE_ERROR);
        return conn_status = CONNECTION_BAD;
    }
    packetlen = build_startup_packet(NULL, db, charset);
    if (packetlen > bufSize - 10) {
        setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
        conn_status = CONNECTION_BAD;
        return conn_status;
    }
    startpacket=Buffer + (bufSize - (packetlen + 1));
    build_startup_packet(startpacket, db, charset);
    if (pqPacketSend(0, startpacket, packetlen) < 0) {
        setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
        return conn_status = CONNECTION_BAD;
    }
    attempts = 0;
    return conn_status = CONNECTION_AWAITING_RESPONSE;
}
#else

/**
 * PGconnection setDbLogin for ESP IDF  
 *
 * @param ServerIP as char string. 
 * @param ServerPort, typically 5432
 * @param dbName the name of the database.
 * @param dbUser the user name of the database.
 * @param dbPasswd the PostgreSQL password.
 * @param charset the used char set
 * @returns the connection status
 */ 

int PGconnection::setDbLogin(const char *ServerIP, int ServerPort, const char *dbName, const char *dbUser, const char *dbPasswd, const char *dbCharset) {

	char *startpacket;
	int packetlen;
	int len;

//	close();
	memset(&DestAddr, 0, sizeof(DestAddr));
	AddrFamily = AF_INET;
	ipProtocol = IPPROTO_IP;
	DestAddr.sin_addr.s_addr = inet_addr(ServerIP);
	DestAddr.sin_family = AF_INET;
	DestAddr.sin_port = htons(ServerPort);

	if (!dbName)
		dbName = dbUser;
	len = strlen(dbUser) + 1;
	if (dbPasswd) {
		len += strlen(dbPasswd) + 1;
	}
	_user = (char *) malloc(len);
	strcpy(_user, dbUser);
	if (dbPasswd) {
		_passwd = _user + strlen(dbUser) + 1;
		strcpy(_passwd, dbPasswd);
	} else {
		_passwd = NULL;
	}

	//int8_t connected = connect(client->connect(server, port);
	SockH = socket(AddrFamily, SOCK_STREAM, ipProtocol);
	if (SockH < 0) {
		ESP_LOGE(PGTAG, "Unable to create socket: errno %d", errno);
		setMsg_P(EM_CONN, PG_RSTAT_HAVE_ERROR);
		return conn_status = CONNECTION_BAD;
	} else {
		int err = connect(SockH, (struct sockaddr *) &DestAddr, sizeof(DestAddr));
		if (err != 0) {
			ESP_LOGE(PGTAG, "Socket unable to connect: errno %d", errno);
			return conn_status = CONNECTION_BAD;
		}
		NetConnected = 1;
		ESP_LOGI(PGTAG, "Successfully connected");
	}

	packetlen = build_startup_packet(NULL, dbName, dbCharset);
	if (packetlen > bufSize - 10) {
		setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
		conn_status = CONNECTION_BAD;
		return conn_status;
	}

	startpacket = Buffer + (bufSize - (packetlen + 1));
	build_startup_packet(startpacket, dbName, dbCharset);
	if (pqPacketSend(0, startpacket, packetlen) < 0) {
		setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
		return conn_status = CONNECTION_BAD;
	}
	attempts = 0;
	return conn_status = CONNECTION_AWAITING_RESPONSE;
}

/**
 * PGconnection close_socket for ESP IDF  
 *
 * @param _SockH the socket handle. 
 */ 
void close_socket(int _SockH)
{
   close(_SockH);
}
#endif

/**
 * PGconnection close. Free allocated resources  
 *
 */ 

void PGconnection::close(void)
{
#ifndef ESP32_IDF  
    if (client->connected()) {
#else
    if (NetConnected) {
#endif    
        pqPacketSend('X', NULL, 0);
#ifndef ESP32_IDF          
        client->stop();
#else
	shutdown(SockH, 0);
	close_socket(SockH);
#endif        
    }
    if (Buffer && !(_flags & PG_FLAG_STATIC_BUFFER)) {
        free(Buffer);
        Buffer = NULL;
    }
    if (_user) {
        free(_user);
        _user = _passwd = NULL;
    }
    conn_status = CONNECTION_NEEDED;

#ifndef ESP32_IDF  
	NetConnected = 0; 
#endif    
}

#ifdef ESP32_IDF
/**
 * PGconnection dataAvailable.   
 *
 * @returns number of Bytes available
 */ 
int PGconnection::dataAvailable() {
	int res=0;
//	if (_available) return _available;
	ioctl(SockH,FIONREAD,&res);
	return res;
}
#endif

/**
 * PGconnection dataAvailable.   
 *
 * @returns the current status of the connection
 */ 

int PGconnection::status(void)
{
    char bereq;
    char rc;
    int32_t msgLen;
    int32_t areq;
    char * pwd = _passwd;
#ifdef PG_USE_MD5
    char salt[4];
#endif

    switch(conn_status) {
        case CONNECTION_NEEDED:
        case CONNECTION_OK:
        case CONNECTION_BAD:

        return conn_status;

        case CONNECTION_AWAITING_RESPONSE:
#ifndef ESP32_IDF         
        if (!client->available()) return conn_status;
#else
        if (dataAvailable() == 0) return conn_status;
#endif        
        if (attempts++ >= 2) {
            setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
            return conn_status = CONNECTION_BAD;
        }
        if (pqGetc(&bereq)) {
            goto read_error;
        }
        if (bereq == 'E') {
            pqGetInt4(&msgLen);
            pqGetNotice(PG_RSTAT_HAVE_ERROR);
            return conn_status = CONNECTION_BAD;
        }
        if (bereq != 'R') {
            setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
            return conn_status = CONNECTION_BAD;
        }
        if (pqGetInt4(&msgLen)) {
            goto read_error;
        }
        if (pqGetInt4(&areq)) {
            goto read_error;
        }
        if (areq == AUTH_REQ_OK) {
            if (_user) {
                free(_user);
                _user = _passwd=NULL;
            }
            result_status = PG_RSTAT_READY;
            return conn_status = CONNECTION_AUTH_OK;
        }
        if (
#ifdef PG_USE_MD5
                areq != AUTH_REQ_MD5 &&
#endif
                areq !=  AUTH_REQ_PASSWORD) {
            setMsg_P(EM_UAUTH, PG_RSTAT_HAVE_ERROR);
            return conn_status = CONNECTION_BAD;
        }
        if (!_passwd || !*_passwd) {
            setMsg_P(EM_PASSWD, PG_RSTAT_HAVE_ERROR);
            return conn_status = CONNECTION_BAD;
        }
        pwd = _passwd;
#ifdef PG_USE_MD5
        if (areq == AUTH_REQ_MD5) {
            if (pqGetnchar(salt, 4)) goto read_error;
            if (bufSize < 3 * MD5_PASSWD_LEN + 10) {
                setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
                return conn_status = CONNECTION_BAD;
            }
            char *crypt_pwd = Buffer + (bufSize - (2 * (MD5_PASSWD_LEN + 1)));
            char *crypt_pwd2 = crypt_pwd + MD5_PASSWD_LEN + 1;
            pg_md5_encrypt(pwd, _user, strlen(_user), crypt_pwd2);
            pg_md5_encrypt(crypt_pwd2 + 3, salt,4, crypt_pwd);
            pwd = crypt_pwd;
        }
#endif
        rc=pqPacketSend('p', pwd, strlen(pwd) + 1);
        if (rc) {
            goto write_error;
        }
        return conn_status;

        case CONNECTION_AUTH_OK:
        for (;;) {
#ifndef ESP32_IDF        
            if (!client -> available()) return conn_status;
#else
            if (dataAvailable() == 0) return conn_status;
#endif            
            if (pqGetc(&bereq)) goto read_error;
            if (pqGetInt4(&msgLen)) goto read_error;
            msgLen -= 4;
            if (bereq == 'A' || bereq == 'N' || bereq == 'S' || bereq == 'K') {
                if (pqSkipnchar(msgLen))  goto read_error;
                continue;
            }
            if (bereq == 'E') {
                pqGetNotice(PG_RSTAT_HAVE_ERROR);
                return conn_status = CONNECTION_BAD;
            }

/*            if (bereq == 'K') {
                if (pqGetInt4(&be_pid)) goto read_error;
                if (pqGetInt4(&be_key)) goto read_error;
                continue;
            }
*/
            if (bereq == 'Z') {
                pqSkipnchar(msgLen);
                return conn_status = CONNECTION_OK;
            }
            return conn_status = CONNECTION_BAD;
        }

        default:
        setMsg_P(EM_INTR, PG_RSTAT_HAVE_ERROR);
        return conn_status = CONNECTION_BAD;
    }
read_error:
    setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
    return conn_status = CONNECTION_BAD;
write_error:
    setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
    return conn_status = CONNECTION_BAD;
}

/**
 * PGconnection execute.   
 *
 * @param query the SQL command.
 * @param progmen inicates the usage of progmem on Arduino, not used on ESP IDF 
 * @returns 0 if ok, otherwise -1
 */ 

int PGconnection::execute(const char *query, int progmem)
{
#ifndef ESP32
// for unknown reason, this status check get wrong on ESP32-IDF.
// uncommenting solve the problem for now, but need invesitgation
    if (!(result_status & PG_RSTAT_READY)) {
        setMsg_P(EM_EXEC, PG_RSTAT_HAVE_ERROR);
        return -1;
    }
#endif    
    int len =
#ifndef ESP32
     progmem ? strlen_P(query) :
#endif
        strlen(query);
        
#ifndef ESP32_IDF        
    if (pqPacketSend('Q', query, len+1, progmem)) {
#else
	if (pqPacketSend('Q', query, len + 1)) {
#endif    
        setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
        conn_status = CONNECTION_BAD;
        return -1;
    }
    result_status = PG_RSTAT_COMMAND_SENT;
    return 0;   
    
}
/*
int PGconnection::PGexecute(const char *query) {
	int len = strlen(query);
	if (pqPacketSend('Q', query, len + 1)) {
		setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
		conn_status = CONNECTION_BAD;
		return -1;
	}
	result_status = PG_RSTAT_COMMAND_SENT;
	return 0;
}
*/

/**
 * PGconnection escapeName.   
 *
 * @param inbuf the SQL command before names are escaped.
 * @param outbuf the SQL command after names are escaped. 
 * @returns the lenght of the outbuf result
 */ 

int PGconnection::escapeName(const char *inbuf, char *outbuf)
{
    const char *c;
    int l = 2;
    for (c=inbuf; *c; c++) {
        l++;
        if (*c == '\\' || *c == '"') l++;
    }
    if (outbuf) {
        *outbuf++='"';
        for (c=inbuf; *c; c++) {
            *outbuf++ = *c;
            if (*c == '\\' || *c == '"') *outbuf++ = *c;
        }
        *outbuf++='"';
    }
    return l;
}

/**
 * PGconnection escapeString.   
 *
 * @param inbuf the SQL command before names are escaped.
 * @param outbuf the SQL command after names are escaped. 
 * @returns the lenght of the outbuf result
 */ 

int PGconnection::escapeString(const char *inbuf, char *outbuf)
{
    const char *c;
    int e = 0, l;
    for (c=inbuf; *c; c++) {
        if (*c == '\\' || *c == '\'') e++;
    }
    l = e + (c - inbuf) + (e ? 4 : 2);
    if (outbuf) {
        if (e) {
            *outbuf++=' ';
            *outbuf++='E';
        }
        *outbuf++='\'';
        for (c=inbuf; *c; c++) {
            *outbuf++ = *c;
            if (*c == '\\' || *c == '\'') *outbuf++ = *c;
        }
        *outbuf++='\'';
    }
    return l;
}

/**
 * PGconnection getValue.   
 *
 * @param nr number of the row.
 * @returns the value as char pointer.
 */ 

char * PGconnection::getValue(int nr)
{
    int i;
    if (_null & (1<<nr)) return NULL;
    char *c=Buffer;
    if (nr < 0 || nr >= _nfields) return NULL;
    for (i=0; i < nr; i++) {
        if (_null & (1 <<i)) continue;
        c += strlen(c) + 1;
    }
    return c;
}

/**
 * PGconnection getColumn.   
 *
 * @param n the column.
 * @returns the char pointer to the column.
 */ 
char *PGconnection::getColumn(int n)
{
    char *c;int i;
    if (!(result_status & PG_RSTAT_HAVE_COLUMNS)) return NULL;
    if (n < 0 || n >= _nfields) return NULL;
    for (c = Buffer, i = 0; i<n; i++) {
        c += strlen(c) + 1;
    }
    return c;
}

/**
 * PGconnection getMessage.   
 *
 * @returns the last message, if any.
 */ 

char *PGconnection::getMessage(void)
{
    if (!(result_status & PG_RSTAT_HAVE_MESSAGE)) return NULL;
    return Buffer;
}

/**
 * PGconnection getData.   
 *
 * @returns the availability status of data
 */ 

int PGconnection::getData(void)
{
    char id;
    int32_t msgLen;
    int rc;
    char *c;
#ifndef ESP32_IDF        
            if (!client -> available()) return  0;
#else
            if (dataAvailable() == 0) return  0;
#endif     
    
    
    if (pqGetc(&id)) goto read_error;
    if (pqGetInt4(&msgLen)) goto read_error;
    //Serial.printf("ID=%c\n", id);
    msgLen -= 4;
    switch(id) {
        case 'T':
        if ((rc=pqGetRowDescriptions())) {
            if (rc == -2) setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
            else if (rc == -3) setMsg_P(EM_BIN, PG_RSTAT_HAVE_ERROR);
            goto read_error;
        }
        if (_flags & PG_FLAG_IGNORE_COLUMNS) {
            result_status &= ~PG_RSTAT_HAVE_MASK;
            return 0;
        }
        return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_COLUMNS;

        case 'E':
        if (pqGetNotice(PG_RSTAT_HAVE_ERROR)) goto read_error;
        return result_status;

        case 'N':
        if (_flags & PG_FLAG_IGNORE_NOTICES) {
            if (pqSkipnchar(msgLen)) goto read_error;
            return 0;
        }
        if(pqGetNotice(PG_RSTAT_HAVE_NOTICE)) goto read_error;
        return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_NOTICE;

        case 'A':
        if (_flags & PG_FLAG_IGNORE_NOTICES) {
            if (pqSkipnchar(msgLen)) goto read_error;
            return 0;
        }
        if (pqGetNotify(msgLen)) goto read_error;
        return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_NOTICE;

        case 'Z':
        if (pqSkipnchar(msgLen)) goto read_error;
        result_status = (result_status & PG_RSTAT_HAVE_SUMMARY) | PG_RSTAT_READY;
        return PG_RSTAT_READY;

        case 'S': // parameters setting ignored
        case 'K': // should not be here?
        if (pqSkipnchar(msgLen)) goto read_error;
        return 0;

        case 'C': // summary
        if (msgLen > bufSize - 1) goto oom;
        if (pqGetnchar(Buffer, msgLen)) goto read_error;
        Buffer[msgLen] = 0;
        _ntuples = 0;
        result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_SUMMARY;
        for (c = Buffer; *c && !isdigit(*c); c++);
        if (!*c) return result_status;
        if (strncmp(Buffer,"SELECT ",7)) {
            for (; *c && isdigit(*c); c++);
            for (; *c && !isdigit(*c); c++);
        }
        if (*c) _ntuples = strtol(c, NULL, 10);
        return result_status;

        case 'D':
        if ((rc=pqGetRow())) {
            if (rc == -2) setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
            else if (rc == -3) setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
            goto read_error;
        }
        if (_flags & PG_FLAG_IGNORE_COLUMNS) {
            result_status &= ~PG_RSTAT_HAVE_MASK;
            return 0;
        }
        return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_ROW;

        case 'I':
        if (pqSkipnchar(msgLen)) goto read_error;
        setMsg_P(EM_EMPTY, PG_RSTAT_HAVE_ERROR);
        return result_status;

        default:
        setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
        conn_status = CONNECTION_BAD;
        return -1;
    }

oom:
    setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);

read_error:
    if (!(result_status & PG_RSTAT_HAVE_ERROR)) {
        setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
    }
    conn_status = CONNECTION_BAD;
    return -1;
}

/**
 * PGconnection executeFormat.   
 *
 * @param progmem the SQL command.
 * @param format the format string. 
 * @returns 0 on sucess, otherwise -1.
 */ 

int PGconnection::executeFormat(int progmem, const char *format, ...)
{
    int32_t msgLen;
    va_list va;
    va_start(va, format);
    msgLen = writeFormattedQuery(0, progmem, format, va);
    va_end(va);
    if (msgLen < 0) return -1;
    va_start(va, format);
    msgLen = writeFormattedQuery(msgLen, progmem, format, va);
    va_end(va);
    if (msgLen) {
        return -1;
    }
    result_status = PG_RSTAT_COMMAND_SENT;
    return 0;
}

#ifdef ESP8266

// there is no strchr_P in ESP8266 ROM :(

static const char *strchr_P(const char *str, char c)
{
    char z;
    for (;;) {
        z = pgm_read_byte(str);
        if (!z) return NULL;
        if (z == c) return str;
        str++;
    }
}

#endif

#ifdef ESP32_IDF
//emulate Arduino PSTR()

/**
 * PSTR.    
 *
 * @param str input string.
 * @returns the result string.
 */ 

char *PSTR(char *str)
{
   return str;
}
const char *PSTR(const char *str)
{
   return str;
}

//emulate strcpy_P()
/**
 * strcpy_P(). for compatibility with the Arduino version on ESP IDF   
 *
 * @param destination buffer.
 * @param source buffer. 
 * @returns number of bytes copied
 */ 
char *strcpy_P(char * destination, const char * source )
{
    return strcpy(destination, source);
}

int strlen_P(char *str)
{
   return strlen(str);
}
int strlen_P(const char *str)
{
   return strlen(str);
}
#endif

/**
 * PGconnection build_startup_packet.   
 *
 * @param packet buffer for the packet.
 * @param db name of the database.
 * @param charset name of the character set
 * @returns the lenght of the packet
 */ 

int PGconnection::build_startup_packet(
    char *packet,
    const char *db,
    const char *charset)
{
    int packet_len = 4;
    if (packet) {
        memcpy(packet,"\0\003\0\0", 4);
    }
#define ADD_STARTUP_OPTION(optname, optval) \
	do { \
		if (packet) \
			strcpy_P(packet + packet_len, (char *)optname); \
		packet_len += strlen_P((char *)optname) + 1; \
		if (packet) \
			strcpy(packet + packet_len, (char *)optval); \
		packet_len += strlen((char *)optval) + 1; \
	} while(0)

#define ADD_STARTUP_OPTION_P(optname, optval) \
	do { \
		if (packet) \
			strcpy_P(packet + packet_len, (char *)optname); \
		packet_len += strlen_P((char *)optname) + 1; \
		if (packet) \
			strcpy_P(packet + packet_len, (char *)optval); \
		packet_len += strlen_P((char *)optval) + 1; \
	} while(0)

	if (_user && _user[0])
		ADD_STARTUP_OPTION(PSTR("user"), _user);
	if (db && db[0])
		ADD_STARTUP_OPTION(PSTR("database"), db);
	if (charset && charset[0])
		ADD_STARTUP_OPTION(PSTR("client_encoding"), charset);
    ADD_STARTUP_OPTION_P(PSTR("application_name"), PSTR("arduino"));
#undef ADD_STARTUP_OPTION
	if (packet)
		packet[packet_len] = '\0';
	packet_len++;

	return packet_len;
}

/**
 * PGconnection pqPacketSend.   
 *
 * @param pack_type type of the packet.
 * @param buf the result buffer.
 * @param buf_len length of the result buffer
 * @progmem to be used on Arduino
 * @returns 0 if success, network error code otherwise
 */ 

int PGconnection::pqPacketSend(char pack_type, const char *buf, int buf_len, int progmem)
{
    char *start = Buffer;
    int l = bufSize - 4;
#ifndef ESP32    
    int n;
#endif    
    if (pack_type) {
        *start++ = pack_type;
        l--;
    }
#ifdef __AVR__
    *start++=0;
    *start++=0;
#else
    *start++ = ((buf_len + 4) >> 24) & 0xff;
    *start++ = ((buf_len + 4) >> 16) & 0xff;
#endif
    *start++ = ((buf_len + 4) >> 8) & 0xff;
    *start++ = (buf_len + 4) & 0xff;
#ifndef ESP32
    if (progmem) {
        while (buf_len > 0) {
            while (buf_len > 0 && l > 0) {
                *start++ = pgm_read_byte(buf++);
                buf_len--;
            }
            n = client->write((const uint8_t *)Buffer, start - Buffer);
            if (n != start - Buffer) return -1;
            start = Buffer;
            l = bufSize;
        }
    }
    else {
#endif
        if (buf) {
            if (buf_len <= l) {
                memcpy(start, buf, buf_len);
                start += buf_len;
                buf_len = 0;
            }
            else {
                memcpy(start, buf, l);
                start += l;
                buf_len -= l;
                buf += l;
            }
        }
#ifndef ESP32_IDF        
        n = client->write((const uint8_t *)Buffer, start - Buffer);
        if (n != start - Buffer) return -1;
        if (buf && buf_len) {
            n = client->write((const uint8_t *)buf, buf_len);
            if (n != buf_len) return -1;
        }        
#else
	int err = send(SockH, /*_buffer*/Buffer, start - /*_buffer*/Buffer, 0);
	if (err < 0) {
		ESP_LOGE(PGTAG, "pqPacketSend: Send Error occurred during sending: errno %d", errno);
		return err;
	}
	if (buf && buf_len) {
		err = send(SockH, (const char *) buf, (size_t) buf_len, 0);
		if (err < 0) {
			ESP_LOGE(PGTAG, "Send2 Error occurred during sending: errno %d", errno);
			return err;

		}
	}	
#endif        

#ifndef ESP32
    }
#endif
    return 0;
}

/**
 * PGconnection pqGetc.   
 *
 * @param buf buffer to store received char.
 * @returns length of received data
 */ 

int PGconnection::pqGetc(char *buf)
{
    int i;
#ifndef ESP32_IDF    
    for (i=0; !client->available() && i < 10; i++) {
        delay (i * 10 + 10);
    }
    if (!client->available()) {
        return -1;
    }
    *buf = client->read();
    return 0;
#else    
        i = 0;
	while (i<10) {
		if (dataAvailable()>0) break;
		else {
			vTaskDelay(i++);
		}
	}
	if (i==10) return -1;

	int len = read(SockH, (void *) buf, 1);
	_available-=len;
	return -1+len;    
#endif    
}

/**
 * PGconnection pqGetInt4.   
 *
 * @param result the converted result to int4.
 * @returns 0 for sucess, otherwise -1.
 */ 

int PGconnection::pqGetInt4(int32_t *result)
{
	uint32_t tmp4 = 0;
    byte tmp,i;
    for (i = 0; i < 4; i++) {
        if (pqGetc((char *)&tmp)) return -1;
        tmp4 = (tmp4 << 8) | tmp;
    }
    *result = tmp4;
    return 0;
}

/**
 * PGconnection pqGetInt2.   
 *
 * @param result the converted result to int2.
 * @returns 0 for sucess, otherwise -1.
 */ 

int PGconnection::pqGetInt2(int16_t *result)
{
	uint16_t tmp2 = 0;
    byte tmp,i;
    for (i = 0; i < 2; i++) {
        if (pqGetc((char *)&tmp)) return -1;
        tmp2 = (tmp2 << 8) | tmp;
    }
    *result = tmp2;
    return 0;
}

/**
 * PGconnection pqGetnchar.   
 *
 * @param s the string to search in
 * @param len the lenght of the string
 * @returns 0 for sucess, otherwise -1.
 */ 

int PGconnection::pqGetnchar(char *s, int len)
{
    while (len-- > 0) {
        if (pqGetc(s++)) return -1;
    }
    return 0;
}

/**
 * PGconnection pqGets.   
 *
 * @param s the string to search in
 * @param maxlen the lenght of the string
 * @returns the position in the string, or -1 for an error
 */ 

int PGconnection::pqGets(char *s, int maxlen)
{
    int len;
    char z;
    for (len = 0;len < maxlen; len++) {
        if (pqGetc(&z)) return -1;
        if (s) *s++ = z;
        if (!z) return len+1;
    }
    return - (len + 1);
}

/**
 * PGconnection pqSkipnchar.   
 *
 * @param len the lenght of the string
 * @returns 0 for sucess, or -1 for an error
 */ 

int PGconnection::pqSkipnchar(int len)
{
    char dummy;
    while (len-- > 0) {
        if (pqGetc(&dummy)) return -1;
    }
    return 0;
}

/**
 * PGconnection pqGetRow.   
 *
 * @returns 0 for sucess, or -1 for an error
 */ 

int PGconnection::pqGetRow(void)
{
    int i;
    int bufpos = 0;
    int32_t len;
    int16_t cols;

    _null = 0;
    if (pqGetInt2(&cols)) return -1;
    if (cols != _nfields) {
        return -3;
    }
    for (i=0; i < _nfields; i++) {
        if (pqGetInt4(&len)) return -1;
        if (len < 0) {
            _null |= 1<<i;
            continue;
        }
        if (bufpos + len + 1 > bufSize) {
            return -2;
        }
        if (pqGetnchar(Buffer + bufpos, len)) return -1;
        bufpos += len;
        Buffer[bufpos++]=0;
    }
    return 0;
}

/**
 * PGconnection pqGetRowDescriptions.   
 *
 * @returns the row description, or -1 for an error
 */ 

int PGconnection::pqGetRowDescriptions(void)
{
    int i;
    int16_t format;
    int rc;
    int bufpos;
    if (pqGetInt2(&_nfields)) return -1;
    if (_nfields > PG_MAX_FIELDS) return -2; // implementation limit
    _formats = 0;
    bufpos = 0;
    for (i = 0;i < _nfields; i++) {
        if (!(_flags & PG_FLAG_IGNORE_COLUMNS)) {
            if (bufpos >= bufSize - 1) return -2;
            rc = pqGets(Buffer + bufpos, bufSize - bufpos);
            if (rc < 0) return -1;
            bufpos += rc;
        }
        else {
            if (pqGets(NULL, 8192) < 0) {
                return -1;
            }
        }
        if (pqSkipnchar(16)) return -1;
        if (pqGetInt2(&format)) return -1;
        format = format ? 1 : 0;
        _formats |= format << i;
    }
    if (_formats) return -3;
    return 0;
}

/**
 * PGconnection setMsg.   
 *
 * @param s the message
 * @param len the lenght of the message
 */ 
void PGconnection::setMsg(const char *s, int type)
{
    strcpy(Buffer, s);
    result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
}

/**
 * PGconnection setMsg_P.   
 *
 * @param s the message
 * @param len the lenght of the message
 */ 
void PGconnection::setMsg_P(const char *s, int type)
{
    strcpy_P(Buffer, s);
    result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
}

/**
 * PGconnection pqGetNotice.   
 *
 * @param type the notice type
 * @returns 0 on success, or -1 for an error
 */ 
int PGconnection::pqGetNotice(int type)
{
    int bufpos = 0;
    char id;
    int rc;
    for (;;) {
        if (pqGetc(&id)) goto read_error;
        if (!id) break;
        if (id == 'S' || id == 'M') {
            if (bufpos && bufpos < bufSize - 1) Buffer[bufpos++]=':';
            rc = pqGets(Buffer + bufpos, bufSize - bufpos);
            if (rc < 0) goto read_error;
            bufpos += rc -1;
        }
        else {
            rc = pqGets(NULL, 8192);
            if (rc < 0) goto read_error;
        }
    }
    Buffer[bufpos] = 0;
    result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
    return 0;

read_error:
    if (!bufpos) setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
    return -1;
}

/**
/* PGconnection pqGetNotify.   
 *
 * @param msgLen the length of the message
 * @returns 0 on success, or -1 for an error
 */ 
int PGconnection::pqGetNotify(int32_t msgLen)
{
    int32_t pid;
    int bufpos, i;
    if (pqGetInt4(&pid)) return -1;
    msgLen -= 4;
    bufpos = sprintf(Buffer,"%d:",pid);
    if (msgLen > bufSize - (bufpos + 1)) {
        if (pqGetnchar(Buffer+bufpos, bufSize - (bufpos + 1)))
            return -1;
        msgLen -= bufSize - (bufpos + 1);
        if (pqSkipnchar(msgLen)) return -1;
        Buffer[msgLen = bufSize - 1] = 0;

    }
    else {
        if (pqGetnchar(Buffer+ bufpos, msgLen)) return -1;
        Buffer[bufpos + msgLen] = 0;
        msgLen += bufpos;
    }
    for (i=0; i<msgLen; i++) if (!Buffer[i]) Buffer[i] = ':';
    return 0;
}

#ifndef ESP32
int PGconnection::writeMsgPart_P(const char *s, int len, int fine)
{
    while (len > 0) {
        if (bufPos >= bufSize) {
            if (client->write((uint8_t *)Buffer, bufPos) != (size_t)bufPos) {
                return -1;
            }
            bufPos = 0;
        }
        Buffer[bufPos++] = pgm_read_byte(s++);
        len--;
    }
    if (bufPos && fine) {
        if (client->write((uint8_t *)Buffer, bufPos) != (size_t)bufPos) {
            return -1;
        }
        bufPos = 0;
    }
    return 0;
}
#endif

/**
/* PGconnection writeMsgPart.   
 *
 * @param s message string
 * @param len the length of the message
 * @param fine write end
 * @returns 0 on success, or -1 for an error
 */ 
int PGconnection::writeMsgPart(const char *s, int len, int fine)
{
    while (len > 0) {
        int n = len;
        if (n > bufSize - bufPos) n = bufSize - bufPos;
        memcpy(Buffer + bufPos, s, n);
        bufPos += n;
        s += n;
        len -= n;
        if (bufPos >= bufSize) {
#ifndef ESP32_IDF
            if (client->write((uint8_t *)Buffer, bufPos) != (size_t)bufPos) {
                return -1;
            }            
#else
	    int err = send(SockH, /*_buffer*/Buffer, bufPos, 0);
	    if (err < 0)
		return -1;	    
#endif            

            bufPos = 0;
        }
    }
    if (bufPos && fine) {
#ifndef ESP32_IDF    
        if (client->write((uint8_t *)Buffer, bufPos) != (size_t)bufPos) {
            return -1;
        }
#else
	int err = send(SockH, /*_buffer*/Buffer, bufPos, 0);
	if (err < 0)
		return -1;
#endif        
        bufPos = 0;
    }

    return 0;
}

/**
/* PGconnection writeFormattedQuery.   
 *
 * @param length the length of the message
 * @param progmem the command
 * @param format format string
 * @param va parameters
 * @returns 0 on success, or -1 for an error
 */ 
int32_t PGconnection::writeFormattedQuery(int32_t length, int progmem, const char *format, va_list va)
{
    int32_t msgLen = 0;
    const char *percent;
    int blen, rc;
    char buf[32], znak;
#ifdef ESP32
        (void) progmem;
#endif
    if (length) {
        length += 4;
        bufPos = 0;
        Buffer[bufPos++] = 'Q';
        Buffer[bufPos++] = (length >> 24) & 0xff;
        Buffer[bufPos++] = (length >> 16) & 0xff;
        Buffer[bufPos++] = (length >> 8) & 0xff;
        Buffer[bufPos++] = (length) & 0xff;
    }
    for (;;) {
#ifndef ESP32
        if (progmem) {
            percent = strchr_P(format, '%');
        }
        else {
#endif
            percent = strchr(format, '%');
#ifndef ESP32
        }
#endif
        if (!percent) break;
#ifndef ESP32
        if (progmem) {
            znak = pgm_read_byte(percent+1);
        }
        else {
#endif
            znak = percent[1];
#ifndef ESP32
        }
#endif
        if (!length) {
            msgLen += (percent - format);
        }
        else {
#ifndef ESP32
            if (progmem) {
                rc = writeMsgPart_P(format, percent - format, false);
            }
            else {
#endif
                rc = writeMsgPart(format, percent - format, false);
#ifndef ESP32
            }
#endif
            if (rc) goto write_error;
        }
        format = percent + 2;
        if (znak == 's' || znak == 'n') {
            char *str = va_arg(va, char *);
            blen = (znak == 's') ? escapeString(str, NULL) : escapeName(str, NULL);
            if (!length) {
                msgLen += blen;
            }
            else {
                if (bufPos + blen > bufSize) {
                    rc = writeMsgPart(NULL, 0, true);
                    if (rc) goto write_error;
                }
            }
            if (znak == 's') {
                escapeString(str, Buffer + bufPos);
            }
            else {
                escapeName(str, Buffer + bufPos);
            }
            bufPos += blen;
            continue;
        }
        if (znak == 'l' || znak == 'd') {
            if (znak == 'l') {
                long n = va_arg(va, long);
                blen = snprintf(buf, 32, "'%ld'", n);
            }
            else {
                int n = va_arg(va, int);
                blen = snprintf(buf, 32, "'%d'", n);
            }
            if (length) {
                rc = writeMsgPart(buf, blen, false);
                if (rc) goto write_error;
            }
            else {
                msgLen += blen;
            }
        }
        setMsg_P(EM_FORMAT, PG_RSTAT_HAVE_ERROR);
        return -1;
    }
#ifndef ESP32
    if (progmem) {
        blen = strlen_P(format);
    }
    else {
#endif
        blen = strlen(format);
#ifndef ESP32
    }
#endif
    if (length) {
#ifndef ESP32
        if (progmem) {
            rc = writeMsgPart_P(format, blen, false);
        }
        else {
#endif
            rc = writeMsgPart(format, blen, false);
#ifndef ESP32
        }
#endif
        if (!rc) {
            rc = writeMsgPart("\0",1,true);
        }
        if (rc) goto write_error;
    }
    else {
        msgLen += blen + 1;
    }
    return msgLen;
write_error:
    setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
    conn_status = CONNECTION_BAD;
    return -1;
}
