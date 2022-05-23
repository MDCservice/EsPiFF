/*
 SimplePgSQL.c - Lightweight PostgreSQL connector for Arduino
 Copyright (C) Bohdan R. Rau 2016 <ethanak@polip.com>

 SimplePgSQL is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.

 SimplePgSQL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.

 You should have received a copy of the GNU Lesser General Public
 License along with SimplePgSQL.  If not, write to:
 The Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor
 Boston, MA  02110-1301, USA.
 */
#include <stdio.h>
//#include "esp_eth.h"
#include <string.h>
#include <sys/param.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_netif.h"
#include "lwip/err.h"
#include "lwip/sockets.h"


#include "SimplePgSQL.h"

#define AUTH_REQ_OK			0	/* User is authenticated  */
#define AUTH_REQ_PASSWORD	3	/* Password */
#define PGTAG "PGSQL"
static const char EM_OOM[] = "Out of mem";
static const char EM_READ[] = "Backend read err";
static const char EM_WRITE[] = "Backend write err";
static const char EM_CONN[] = "Cannot conn 2 srv";
static const char EM_SYNC[] = "Backend out of sync";
static const char EM_INTR[] = "Internal err";
static const char EM_UAUTH[] = "auth method !sptd";
static const char EM_BIN[] = "Bin fmt !sptd";
static const char EM_EXEC[] = "Previ exe !finished";
static const char EM_PASSWD[] = "Pwd req";
static const char EM_EMPTY[] = "empty qry";
static const char EM_FORMAT[] = "Illegal chr fmt";

PGconnection::PGconnection(const int flags, const unsigned char *Buffer, const int bufSize) {
	conn_status = CONNECTION_NEEDED;
	_passwd = NULL;
	_user = NULL;
	_buffer = (char *) Buffer;
	_bufSize = bufSize;
	bufPos = 0;
	result_status=0;
	_available=0;
	_nfields=0;
	_ntuples=0;
	_flags=0;
}

int PGconnection::PGsetDbLogin(const char *ServerIP, int ServerPort, const char *dbName, const char *dbUser, const char *dbPasswd, const char *dbCharset) {

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
	if (packetlen > _bufSize - 10) {
		setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
		conn_status = CONNECTION_BAD;
		return conn_status;
	}

	startpacket = _buffer + (_bufSize - (packetlen + 1));
	build_startup_packet(startpacket, dbName, dbCharset);
	if (pqPacketSend(0, startpacket, packetlen) < 0) {
		setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
		return conn_status = CONNECTION_BAD;
	}
	attempts = 0;
	return conn_status = CONNECTION_AWAITING_RESPONSE;
}

void PGconnection::PGclose(void) {
	if (NetConnected) {
		pqPacketSend('X', NULL, 0);
//		client->stop();
		shutdown(SockH, 0);
		close(SockH);
	}
	if (_user) {
		free(_user);
		_user = _passwd = NULL;
	}
	NetConnected = 0;
	conn_status = CONNECTION_NEEDED;
}

int PGconnection::PGstatus(void) {
	char bereq;
	char rc;
	int32_t msgLen;
	int32_t areq;
	char * pwd = _passwd;

	switch (conn_status) {
	case CONNECTION_NEEDED:
	case CONNECTION_OK:
	case CONNECTION_BAD:

		return conn_status;

	case CONNECTION_AWAITING_RESPONSE:
		if (dataAvailable() == 0) return conn_status;
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
				_user = _passwd = NULL;
			}
			result_status = PG_RSTAT_READY;
			return conn_status = CONNECTION_AUTH_OK;
		}
		if (areq != AUTH_REQ_PASSWORD) {
			setMsg_P(EM_UAUTH, PG_RSTAT_HAVE_ERROR);
			return conn_status = CONNECTION_BAD;
		}
		if (!_passwd || !*_passwd) {
			setMsg_P(EM_PASSWD, PG_RSTAT_HAVE_ERROR);
			return conn_status = CONNECTION_BAD;
		}
		pwd = _passwd;
		rc = pqPacketSend('p', pwd, strlen(pwd) + 1);
		if (rc) {
			goto write_error;
		}
		return conn_status;

	case CONNECTION_AUTH_OK:
		for (;;) {
			if (dataAvailable() == 0) return conn_status;
			if (pqGetc(&bereq))
				goto read_error;
			if (pqGetInt4(&msgLen))
				goto read_error;
			msgLen -= 4;
			if (bereq == 'A' || bereq == 'N' || bereq == 'S' || bereq == 'K') {
				if (pqSkipnchar(msgLen))
					goto read_error;
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
		break;
	default:
		setMsg_P(EM_INTR, PG_RSTAT_HAVE_ERROR);
		return conn_status = CONNECTION_BAD;
	}
	read_error: setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
	return conn_status = CONNECTION_BAD;
	write_error: setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
	return conn_status = CONNECTION_BAD;
}

int PGconnection::PGexecute(const char *query) {
	/*
	if (!(result_status & PG_RSTAT_READY)) {
		setMsg_P(EM_EXEC, PG_RSTAT_HAVE_ERROR);
		return -1;
	}
	*/
	int len = strlen(query);
	if (pqPacketSend('Q', query, len + 1)) {
		setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
		conn_status = CONNECTION_BAD;
		return -1;
	}
	result_status = PG_RSTAT_COMMAND_SENT;
	return 0;
}

int PGconnection::PGescapeName(const char *inbuf, char *outbuf) {
	const char *c;
	int l = 2;
	for (c = inbuf; *c; c++) {
		l++;
		if (*c == '\\' || *c == '"')
			l++;
	}
	if (outbuf) {
		*outbuf++ = '"';
		for (c = inbuf; *c; c++) {
			*outbuf++ = *c;
			if (*c == '\\' || *c == '"')
				*outbuf++ = *c;
		}
		*outbuf++ = '"';
	}
	return l;
}

int PGconnection::PGescapeString(const char *inbuf, char *outbuf) {
	const char *c;
	int e = 0, l;
	for (c = inbuf; *c; c++) {
		if (*c == '\\' || *c == '\'')
			e++;
	}
	l = e + (c - inbuf) + (e ? 4 : 2);
	if (outbuf) {
		if (e) {
			*outbuf++ = ' ';
			*outbuf++ = 'E';
		}
		*outbuf++ = '\'';
		for (c = inbuf; *c; c++) {
			*outbuf++ = *c;
			if (*c == '\\' || *c == '\'')
				*outbuf++ = *c;
		}
		*outbuf++ = '\'';
	}
	return l;
}

char * PGconnection::PGgetValue(int nr) {
	int i;
	if (_null & (1 << nr)) return NULL;
	char *c = _buffer;
	if (nr < 0 || nr >= _nfields) return NULL;
	for (i = 0; i < nr; i++) {
		if (_null & (1 << i)) continue;
		c += strlen(c) + 1;
	}
	return c;
}

char *PGconnection::PGgetColumn(int n) {
	char *c;
	int i;
	if (!(result_status & PG_RSTAT_HAVE_COLUMNS)) return NULL;
	if (n < 0 || n >= _nfields)	return NULL;
	for (c = _buffer, i = 0; i < n; i++) {
		c += strlen(c) + 1;
	}
	return c;
}

char *PGconnection::PGgetMessage(void) {
	if (!(result_status & PG_RSTAT_HAVE_MESSAGE))
		return NULL;
	return _buffer;
}

void dumpbuffer(char *b,int l) {
	int i;
	unsigned int v;
	for (i=0;i<l;i++) {
		if (i%8 == 0) {
			printf("\n%04X    ",i);
		}
		v=*b;
		printf("%02X  ",v);
		if (v>31) printf(" %c  ",v);
		else printf("    ");
		b++;
	}
	printf("\n");
}

int PGconnection::PGgetData(void) {
	char id;
	int32_t msgLen;
	int rc;
	char *c;
	int r=0;
	r=dataAvailable();
//	printf("getData:avail:%d\n",r);fflush(stdout);
	if (r==0) return 0;

	if (pqGetc(&id)) goto read_error;
	if (pqGetInt4(&msgLen))	goto read_error;
//	printf("MSG ID: %c Len:%d   (avail:%d)\n",id,msgLen,r);fflush(stdout);
	msgLen -= 4;
	switch (id) {
	case 'T':
		if ((rc = pqGetRowDescriptions())) {
			if (rc == -2)
				setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
			else if (rc == -3)
				setMsg_P(EM_BIN, PG_RSTAT_HAVE_ERROR);
			goto read_error;
		}
		if (_flags & PG_FLAG_IGNORE_COLUMNS) {
			result_status &= ~PG_RSTAT_HAVE_MASK;
			return 0;
		}
		return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_COLUMNS;

	case 'E':
		if (pqGetNotice(PG_RSTAT_HAVE_ERROR))
			goto read_error;
		return result_status;

	case 'N':
		if (_flags & PG_FLAG_IGNORE_NOTICES) {
			if (pqSkipnchar(msgLen))
				goto read_error;
			return 0;
		}
		if (pqGetNotice(PG_RSTAT_HAVE_NOTICE))
			goto read_error;
		return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_NOTICE;

	case 'A':
		if (_flags & PG_FLAG_IGNORE_NOTICES) {
			if (pqSkipnchar(msgLen))
				goto read_error;
			return 0;
		}
		if (pqGetNotify(msgLen))
			goto read_error;
		return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_NOTICE;

	case 'Z':
		if (pqSkipnchar(msgLen))
			goto read_error;
		result_status = (result_status & PG_RSTAT_HAVE_SUMMARY) | PG_RSTAT_READY;
		return PG_RSTAT_READY;

	case 'S':
	case 'K':
		if (pqSkipnchar(msgLen))
			goto read_error;
		return 0;

	case 'C':
		if (msgLen > _bufSize - 1)
			goto oom;
		if (pqGetnchar(_buffer, msgLen))
			goto read_error;
		_buffer[msgLen] = 0;
		_ntuples = 0;
		result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_SUMMARY;
		for (c = _buffer; *c && !isdigit(*c); c++)
			;
		if (!*c)
			return result_status;
		if (strncmp(_buffer, "SELECT ", 7)) {
			for (; *c && isdigit(*c); c++)
				;
			for (; *c && !isdigit(*c); c++)
				;
		}
		if (*c)
			_ntuples = strtol(c, NULL, 10);
		return result_status;

	case 'D':
		if ((rc = pqGetRow())) {
			if (rc == -2)
				setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);
			else if (rc == -3)
				setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
			goto read_error;
		}
		if (_flags & PG_FLAG_IGNORE_COLUMNS) {
			result_status &= ~PG_RSTAT_HAVE_MASK;
			return 0;
		}

		return result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | PG_RSTAT_HAVE_ROW;

	case 'I':
		if (pqSkipnchar(msgLen))
			goto read_error;
		setMsg_P(EM_EMPTY, PG_RSTAT_HAVE_ERROR);
		return result_status;

	default:
		setMsg_P(EM_SYNC, PG_RSTAT_HAVE_ERROR);
		conn_status = CONNECTION_BAD;
		return -1;
	}

	oom: setMsg_P(EM_OOM, PG_RSTAT_HAVE_ERROR);

	read_error: if (!(result_status & PG_RSTAT_HAVE_ERROR)) {
		printf("READERROR!\n");fflush(stdout);
		setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
	}
	conn_status = CONNECTION_BAD;
	return -1;
}

int PGconnection::PGexecuteFormat(const char *format, ...) {
	int32_t msgLen;
	va_list va;
	va_start(va, format);
	msgLen = writeFormattedQuery(0, format, va);
	va_end(va);
	if (msgLen < 0)
		return -1;
	va_start(va, format);
	msgLen = writeFormattedQuery(msgLen, format, va);
	va_end(va);
	if (msgLen) {
		return -1;
	}
	result_status = PG_RSTAT_COMMAND_SENT;
	return 0;
}

int PGconnection::build_startup_packet(char *packet, const char *dbName, const char *dbCharset) {
	int packet_len = 4;
	if (packet) {
		memcpy(packet, "\0\003\0\0", 4);
	}
#define ADD_STARTUP_OPTION(optname, optval) \
  do { \
    if (packet) \
      strcpy(packet + packet_len, (char *)optname); \
    packet_len += strlen((char *)optname) + 1; \
    if (packet) \
      strcpy(packet + packet_len, (char *)optval); \
    packet_len += strlen((char *)optval) + 1; \
  } while(0)

#define ADD_STARTUP_OPTION_P(optname, optval) \
  do { \
    if (packet) \
      strcpy(packet + packet_len, (char *)optname); \
    packet_len += strlen((char *)optname) + 1; \
    if (packet) \
      strcpy(packet + packet_len, (char *)optval); \
    packet_len += strlen((char *)optval) + 1; \
  } while(0)

	if (_user && _user[0])
		ADD_STARTUP_OPTION("user", _user);
	if (dbName && dbName[0])
		ADD_STARTUP_OPTION("database", dbName);
	if (dbCharset && dbCharset[0])
		ADD_STARTUP_OPTION("client_encoding", dbCharset);
	ADD_STARTUP_OPTION_P("application_name", "Scaladis");
#undef ADD_STARTUP_OPTION
	if (packet)
		packet[packet_len] = '\0';
	packet_len++;

	return packet_len;
}

int PGconnection::pqPacketSend(char pack_type, const char *buf, int buf_len) {
	char *start = _buffer;
	int l = _bufSize - 4;
//	int n;
	if (pack_type) {
		*start++ = pack_type;
		l--;
	}
	*start++ = ((buf_len + 4) >> 24) & 0xff;
	*start++ = ((buf_len + 4) >> 16) & 0xff;
	*start++ = ((buf_len + 4) >> 8) & 0xff;
	*start++ = (buf_len + 4) & 0xff;

	if (buf) {
		if (buf_len <= l) {
			memcpy(start, buf, buf_len);
			start += buf_len;
			buf_len = 0;
		} else {
			memcpy(start, buf, l);
			start += l;
			buf_len -= l;
			buf += l;
		}
	}
	int err = send(SockH, _buffer, start - _buffer, 0);
	if (err < 0) {
		ESP_LOGE(PGTAG, "Send Error occurred during sending: errno %d", errno);
		return err;
	}
	if (buf && buf_len) {
		err = send(SockH, (const char *) buf, (size_t) buf_len, 0);
		if (err < 0) {
			ESP_LOGE(PGTAG, "Send2 Error occurred during sending: errno %d", errno);
			return err;

		}
	}

	return 0;
}

int PGconnection::pqGetc(char *buf) {
	int i=0;
//	for (i = 0; !client->available() && i < 10; i++) {
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
}

int PGconnection::pqGetInt4(int32_t *result) {
	uint32_t tmp4 = 0;
	uint8_t tmp, i;
	int rt=0;
	for (i = 0; i < 4; i++) {
		rt=pqGetc((char *) &tmp);
		if (rt) return -1;
		tmp4 = (tmp4 << 8) | tmp;
	}
	*result = tmp4;
	return 0;
}

int PGconnection::pqGetInt2(int16_t *result) {
	uint16_t tmp2 = 0;
	uint8_t tmp, i;
	for (i = 0; i < 2; i++) {
		if (pqGetc((char *) &tmp)) return -1;
		tmp2 = (tmp2 << 8) | tmp;
	}
	*result = tmp2;
	return 0;
}

int PGconnection::pqGetnchar(char *s, int len) {
	while (len-- > 0) {
		if (pqGetc(s++)) return -1;
	}
	return 0;
}

int PGconnection::pqGets(char *s, int maxlen) {
	int len;
	char z;
	for (len = 0; len < maxlen; len++) {
		if (pqGetc(&z))	return -1;
		if (s) *s++ = z;
		if (!z)	return len + 1;
	}
	return -(len + 1);
}

int PGconnection::pqSkipnchar(int len) {
	char dummy;
	int i;
	for (i=0;i<len;i++) read(SockH,&dummy,1);
	/*
	while (len-- > 0) {
		if (pqGetc(&dummy))
			return -1;
	}
	*/
	return 0;
}

int PGconnection::pqGetRow(void) {
	int i;
	int bufpos = 0;
	int32_t len;
	int16_t cols;

	_null = 0;
	if (pqGetInt2(&cols)) return -1;
	if (cols != _nfields) return -3;

	for (i = 0; i < _nfields; i++) {
		if (pqGetInt4(&len)) return -1;
		if (len < 0) {
			_null |= 1 << i;
			continue;
		}
		if (bufpos + len + 1 > _bufSize) {
			return -2;
		}
		if (pqGetnchar(_buffer + bufpos, len))
			return -1;
		bufpos += len;
		_buffer[bufpos++] = 0;
	}
	return 0;
}

int PGconnection::pqGetRowDescriptions(void) {
	int i;
	int16_t format;
	int rc;
	int bufpos;
	if (pqGetInt2(&_nfields))
		return -1;
	if (_nfields > PG_MAX_FIELDS)
		return -2; // implementation limit
	_formats = 0;
	bufpos = 0;

	for (i = 0; i < _nfields; i++) {
		if (!(_flags & PG_FLAG_IGNORE_COLUMNS)) {
			if (bufpos >= _bufSize - 1)
				return -2;
			rc = pqGets(_buffer + bufpos, _bufSize - bufpos);
			if (rc < 0)
				return -1;
			bufpos += rc;
		} else {
			if (pqGets(NULL, 8192) < 0) {
				return -1;
			}
		}
		if (pqSkipnchar(16))
			return -1;
		if (pqGetInt2(&format))
			return -1;
		format = format ? 1 : 0;
		_formats |= format << i;
	}
	if (_formats)
		return -3;
	return 0;
}

void PGconnection::setMsg(const char *s, int type) {
	strcpy(_buffer, s);
	result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
}

void PGconnection::setMsg_P(const char *s, int type) {
	strcpy(_buffer, s);
	result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
}

int PGconnection::pqGetNotice(int type) {
	int bufpos = 0;
	char id;
	int rc;
	for (;;) {
		if (pqGetc(&id)) goto read_error;
		if (!id)
			break;
		if (id == 'S' || id == 'M') {
			if (bufpos && bufpos < _bufSize - 1)
				_buffer[bufpos++] = ':';
			rc = pqGets(_buffer + bufpos, _bufSize - bufpos);
			if (rc < 0)
				goto read_error;
			bufpos += rc - 1;
		} else {
			rc = pqGets(NULL, 8192);
			if (rc < 0)	goto read_error;
		}
	}
	_buffer[bufpos] = 0;
	result_status = (result_status & ~PG_RSTAT_HAVE_MASK) | type;
	return 0;

	read_error: if (!bufpos)
		setMsg_P(EM_READ, PG_RSTAT_HAVE_ERROR);
	return -1;
}

int PGconnection::pqGetNotify(int32_t msgLen) {
	int32_t pid;
	int bufpos, i;
	if (pqGetInt4(&pid))
		return -1;
	msgLen -= 4;
	bufpos = sprintf(_buffer, "%d:", pid);
	if (msgLen > _bufSize - (bufpos + 1)) {
		if (pqGetnchar(_buffer + bufpos, _bufSize - (bufpos + 1)))
			return -1;
		msgLen -= _bufSize - (bufpos + 1);
		if (pqSkipnchar(msgLen))
			return -1;
		_buffer[msgLen = _bufSize - 1] = 0;

	} else {
		if (pqGetnchar(_buffer + bufpos, msgLen))
			return -1;
		_buffer[bufpos + msgLen] = 0;
		msgLen += bufpos;
	}
	for (i = 0; i < msgLen; i++)
		if (!_buffer[i])
			_buffer[i] = ':';
	return 0;
}

int PGconnection::dataAvailable() {
	int res=0;
//	if (_available) return _available;
	ioctl(SockH,FIONREAD,&res);
	return res;
}

int PGconnection::writeMsgPart(const char *s, int len, int fine) {
	while (len > 0) {
		int n = len;
		if (n > _bufSize - bufPos)
			n = _bufSize - bufPos;
		memcpy(_buffer + bufPos, s, n);
		bufPos += n;
		s += n;
		len -= n;
		if (bufPos >= _bufSize) {
//			if (client->write((uint8_t *) Buffer, bufPos) != (size_t) bufPos) return -1;
			int err = send(SockH, _buffer, bufPos, 0);
			if (err < 0)
				return -1;
			bufPos = 0;
		}
	}
	if (bufPos && fine) {
//		if (client->write((uint8_t *) Buffer, bufPos) != (size_t) bufPos) return -1;
		int err = send(SockH, _buffer, bufPos, 0);
		if (err < 0)
			return -1;

		bufPos = 0;
	}

	return 0;
}

int32_t PGconnection::writeFormattedQuery(int32_t length, const char *format, va_list va) {
	int32_t msgLen = 0;
	const char *percent;
	int blen, rc;
#define LBUFLEN 32
	char buf[LBUFLEN], znak;
	if (length) {
		length += 4;
		bufPos = 0;
		_buffer[bufPos++] = 'Q';
		_buffer[bufPos++] = (length >> 24) & 0xff;
		_buffer[bufPos++] = (length >> 16) & 0xff;
		_buffer[bufPos++] = (length >> 8) & 0xff;
		_buffer[bufPos++] = (length) & 0xff;
	}
	for (;;) {
		percent = strchr(format, '%');
		if (!percent)
			break;
		znak = percent[1];
		if (!length) {
			msgLen += (percent - format);
		} else {
			rc = writeMsgPart(format, percent - format, false);
			if (rc)
				goto write_error;
		}
		format = percent + 2;
		if (znak == 's' || znak == 'n') {
			char *str = va_arg(va, char *);
			blen = (znak == 's') ? PGescapeString(str, NULL) : PGescapeName(str, NULL);
			if (!length) {
				msgLen += blen;
			} else {
				if (bufPos + blen > _bufSize) {
					rc = writeMsgPart(NULL, 0, true);
					if (rc)
						goto write_error;
				}
			}
			if (znak == 's') {
				PGescapeString(str, _buffer + bufPos);
			} else {
				PGescapeName(str, _buffer + bufPos);
			}
			bufPos += blen;
			continue;
		}
		if (znak == 'l' || znak == 'd') {
			if (znak == 'l') {
				long n = va_arg(va, long);
				blen = snprintf(buf, LBUFLEN, "'%ld'", n);
			} else {
				int n = va_arg(va, int);
				blen = snprintf(buf, LBUFLEN, "'%d'", n);
			}
			if (length) {
				rc = writeMsgPart(buf, blen, false);
				if (rc)
					goto write_error;
			} else {
				msgLen += blen;
			}
		}
		setMsg_P(EM_FORMAT, PG_RSTAT_HAVE_ERROR);
		return -1;
	}
	blen = strlen(format);
	if (length) {
		rc = writeMsgPart(format, blen, false);
		if (!rc) {
			rc = writeMsgPart("\0", 1, true);
		}
		if (rc)
			goto write_error;
	} else {
		msgLen += blen + 1;
	}
	return msgLen;

	write_error: setMsg_P(EM_WRITE, PG_RSTAT_HAVE_ERROR);
	conn_status = CONNECTION_BAD;
	return -1;
}
