/*
 * SimplePgSQL.h - Lightweight PostgreSQL connector for Arduino
 * Copyright (C) Bohdan R. Rau 2016 <ethanak@polip.com>
 *
 * SimplePgSQL is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * SimplePgSQL is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with SimplePgSQL.  If not, write to:
 * 	The Free Software Foundation, Inc.,
 * 	51 Franklin Street, Fifth Floor
 * 	Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"

#ifndef _SIMPLEPGSQL
#define _SIMPLEPGSQL 1

typedef enum {
	CONNECTION_OK, CONNECTION_BAD, CONNECTION_NEEDED, /* setDbLogin() needed */
	/* Internal states here */
	CONNECTION_AWAITING_RESPONSE, /* Waiting for a response from the postmaster.        */
	CONNECTION_AUTH_OK /* Received authentication; waiting for backend startup. */
} ConnStatusType;

#define PG_BUFFER_SIZE 4096

// maximum number of fields in backend response
// must not exceed number of bits in _formats and _null
#define PG_MAX_FIELDS 32

// ignore notices and notifications
#define PG_FLAG_IGNORE_NOTICES 1
// do not store column names
#define PG_FLAG_IGNORE_COLUMNS 2

// ready for next query
#define PG_RSTAT_READY        1   // command sent
#define PG_RSTAT_COMMAND_SENT 2   // column names in buffer
#define PG_RSTAT_HAVE_COLUMNS 4   // row values in buffer
#define PG_RSTAT_HAVE_ROW     8   // summary (number of tuples/affected rows) received
#define PG_RSTAT_HAVE_SUMMARY 16  // error message in buffer
#define PG_RSTAT_HAVE_ERROR   32  // notice/notification in buffer
#define PG_RSTAT_HAVE_NOTICE  64

#define PG_RSTAT_HAVE_MASK (PG_RSTAT_HAVE_COLUMNS | \
    PG_RSTAT_HAVE_ROW | \
    PG_RSTAT_HAVE_SUMMARY | \
    PG_RSTAT_HAVE_ERROR | \
    PG_RSTAT_HAVE_NOTICE)

#define PG_RSTAT_HAVE_MESSAGE (PG_RSTAT_HAVE_ERROR | PG_RSTAT_HAVE_NOTICE)

class PGconnection {
public:

	PGconnection(const int flags, const unsigned char *Buffer, const int bufSize);
	/*
	 * returns connection status.
	 * passwd may be null in case of 'trust' authorization.
	 * only 'trust', 'password' and 'md5' (if compiled in)
	 * authorization modes are implemented.
	 * ssl mode is not implemented.
	 * database name defaults to user name         *
	 */
	int PGsetDbLogin(const char *ServerIP, int ServerPort, const char *dbName, const char *dbUser, const char *dbPasswd, const char *charset);
	/*
	 * performs authorization tasks if needed
	 * returns current connection status
	 * must be called periodically until OK, BAD or NEEDED
	 */
	int PGstatus(void);
	/*
	 * sends termination command if possible
	 * closes client connection and frees internal buffer
	 */
	void PGclose(void);
	/*
	 * sends query to backend
	 * returns negative value on error
	 * or zero on success
	 */
	int PGexecute(const char *query);

	/* should be called periodically in idle state
	 * if notifications are enabled
	 * returns:
	 * - negative value on error
	 * - zero if no interesting data arrived
	 * - current data status if some data arrived
	 */
	int PGgetData(void);
	/*
	 * returns pointer to n-th column name in internal buffer
	 * if available or null if column number out of range
	 * will be invalidated on next getData call
	 */
	char *PGgetColumn(int n);
	/*
	 * returns pointer to n-th column value in internal buffer
	 * if available or null if column number out of range
	 * or value is NULL
	 * will be invalidated on next getData call
	 */
	char *PGgetValue(int);
	/*
	 * returns pointer to message (error or notice)
	 * if available or NULL
	 * will be invalidated on next getData call
	 */
	char *PGgetMessage(void);
	int PGdataStatus(void) {
		return result_status;
	}
	;
	int PGnfields(void) {
		return _nfields;
	}
	;
	int PGntuples(void) {
		return _ntuples;
	}
	;
	/*
	 * returns length of escaped string
	 * single quotes and E prefix (if needed)
	 * will be added.
	 */
	int PGescapeString(const char *inbuf, char *outbuf);
	/*
	 * returns length of escaped string
	 * double quotes will be added.
	 */
	int PGescapeName(const char *inbuf, char *outbuf);
	/*
	 * sends formatted query to backend
	 * returns negative value on error
	 * or zero on success
	 * Formatting sequences:
	 * %s - string literal (will be escaped with escapeString)
	 * %n - name (will be escaped with escapeName)
	 * %d - int (single quotes will be added)
	 * %l - long int (single quotes will be added)
	 * %% - % character
	 */
	int PGexecuteFormat(const char *format, ...);

private:
	int pqPacketSend(char pack_type, const char *buf, int buf_len);
	int pqGetc(char *);
	int pqGetInt4(int32_t *result);
	int pqGetInt2(int16_t *result);
	int pqGetnchar(char *s, int len);
	int pqSkipnchar(int len);
	int pqGets(char *s, int maxlen);
	int pqGetRowDescriptions(void);
	int pqGetRow(void);
	void setMsg(const char *, int);
	void setMsg_P(const char *, int);
	int pqGetNotice(int);
	int pqGetNotify(int32_t);
	char *_user;
	char *_passwd;
//	char *Buffer;
	char *_buffer;
//	int bufSize;
	int _bufSize;
	int bufPos;
	int writeMsgPart(const char *s, int len, int fine);
	int32_t writeFormattedQuery(int32_t length, const char *format, va_list va);
	int dataAvailable(void);
	int build_startup_packet(char *packet, const char *db, const char *charset);
	uint8_t conn_status;
	uint8_t attempts;
	/*
	 int32_t be_pid;
	 int32_t be_key;
	 */
	int16_t _nfields;
	int16_t _ntuples;
	uint32_t _formats;
	uint32_t _null;
	uint8_t _binary;
	uint8_t _flags;
	uint32_t _available;
	int result_status;
	// network stuff
	struct sockaddr_in DestAddr;
	int SockH = -1;
	int ipProtocol = 0;
	int AddrFamily = 0;
	int NetConnected=0;

};

#endif
