/*
 * ministun.c
 * Part of the ministun package
 *
 * STUN support code borrowed from Asterisk -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2006, Digium, Inc.
 * Mark Spencer <markster@digium.com>
 * Standalone remake (c) 2009 Vladislav Grishenko <themiron@mail.ru>
 *
 * See http://www.asterisk.org for more information about
 * the Asterisk project. Please do not directly contact
 * any of the maintainers of this project for assistance;
 * the project provides a web site, mailing lists and IRC
 * channels for your use.
 *
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2. See the LICENSE file
 * at the top of the source tree.
 * 
 * This code provides some support for doing STUN transactions.
 * Eventually it should be moved elsewhere as other protocols
 * than RTP can benefit from it - e.g. SIP.
 * STUN is described in RFC3489 and it is based on the exchange
 * of UDP packets between a client and one or more servers to
 * determine the externally visible address (and port) of the client
 * once it has gone through the NAT boxes that connect it to the
 * outside.
 * The simplest request packet is just the header defined in
 * struct stun_header, and from the response we may just look at
 * one attribute, STUN_MAPPED_ADDRESS, that we find in the response.
 * By doing more transactions with different server addresses we
 * may determine more about the behaviour of the NAT boxes, of
 * course - the details are in the RFC.
 *
 * All STUN packets start with a simple header made of a type,
 * length (excluding the header) and a 16-byte random transaction id.
 * Following the header we may have zero or more attributes, each
 * structured as a type, length and a value (whose format depends
 * on the type, but often contains addresses).
 * Of course all fields are in network format.
 */

#if 0
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#ifndef uint64_t

#include <stdint.h>

#ifdef WIN32
#include <winsock2.h>
#pragma comment(lib, "Ws2_32.lib")
#ifndef socklen_t

#ifndef int32_t
typedef __int32 int32_t;
#endif
#ifndef uint32_t
typedef unsigned __int32 uint32_t;
#endif

#define socklen_t uint32_t
#endif

#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

#include <time.h>
#include <errno.h>

#endif		// #ifndef uint64_t



#include "ministun.h"

#undef STUN_BINDREQ_PROCESS

char *stunserver = STUN_SERVER;
int stunport = STUN_PORT;
int stuncount = STUN_COUNT;
int stundebug = 0;

/* helper function to print message names */
static const char *stun_msg2str(int msg)
{
	switch (msg) {
	case STUN_BINDREQ:
		return "Binding Request";
	case STUN_BINDRESP:
		return "Binding Response";
	case STUN_BINDERR:
		return "Binding Error Response";
	case STUN_SECREQ:
		return "Shared Secret Request";
	case STUN_SECRESP:
		return "Shared Secret Response";
	case STUN_SECERR:
		return "Shared Secret Error Response";
	}
	return "Non-RFC3489 Message";
}

/* helper function to print attribute names */
static const char *stun_attr2str(int msg)
{
	switch (msg) {
	case STUN_MAPPED_ADDRESS:
		return "Mapped Address";
	case STUN_RESPONSE_ADDRESS:
		return "Response Address";
	case STUN_CHANGE_REQUEST:
		return "Change Request";
	case STUN_SOURCE_ADDRESS:
		return "Source Address";
	case STUN_CHANGED_ADDRESS:
		return "Changed Address";
	case STUN_USERNAME:
		return "Username";
	case STUN_PASSWORD:
		return "Password";
	case STUN_MESSAGE_INTEGRITY:
		return "Message Integrity";
	case STUN_ERROR_CODE:
		return "Error Code";
	case STUN_UNKNOWN_ATTRIBUTES:
		return "Unknown Attributes";
	case STUN_REFLECTED_FROM:
		return "Reflected From";
	}
	return "Non-RFC3489 Attribute";
}

/* here we store credentials extracted from a message */
struct stun_state {
	const char *username;
	const char *password;
};

static int stun_process_attr(struct stun_state *state, struct stun_attr *attr)
{
	if (stundebug)
		fprintf(stderr, "Found STUN Attribute %s (%04x), length %d\n",
			    stun_attr2str(ntohs(attr->attr)), ntohs(attr->attr), ntohs(attr->len));
	switch (ntohs(attr->attr)) {
#ifdef STUN_BINDREQ_PROCESS
	case STUN_USERNAME:
		state->username = (const char *) (attr->value);
		break;
	case STUN_PASSWORD:
		state->password = (const char *) (attr->value);
		break;
#endif
	case STUN_MAPPED_ADDRESS:
		break;
	default:
		if (stundebug)
			fprintf(stderr, "Ignoring STUN attribute %s (%04x), length %d\n", 
				    stun_attr2str(ntohs(attr->attr)), ntohs(attr->attr), ntohs(attr->len));
	}
	return 0;
}

#ifdef STUN_BINDREQ_PROCESS
/* append a string to an STUN message */
static void append_attr_string(struct stun_attr **attr, int attrval, const char *s, int *len, int *left)
{
	int size = sizeof(**attr) + strlen(s);
	if (*left > size) {
		(*attr)->attr = htons(attrval);
		(*attr)->len = htons(strlen(s));
		memcpy((*attr)->value, s, strlen(s));
		(*attr) = (struct stun_attr *)((*attr)->value + strlen(s));
		*len += size;
		*left -= size;
	}
}

/* append an address to an STUN message */
static void append_attr_address(struct stun_attr **attr, int attrval, struct sockaddr_in *sock_in, int *len, int *left)
{
	int size = sizeof(**attr) + 8;
	struct stun_addr *addr;
	if (*left > size) {
		(*attr)->attr = htons(attrval);
		(*attr)->len = htons(8);
		addr = (struct stun_addr *)((*attr)->value);
		addr->unused = 0;
		addr->family = 0x01;
		addr->port = sock_in->sin_port;
		addr->addr = sock_in->sin_addr.s_addr;
		(*attr) = (struct stun_attr *)((*attr)->value + 8);
		*len += size;
		*left -= size;
	}
}
#endif

/* wrapper to send an STUN message */
static int stun_send(int s, struct sockaddr_in *dst, struct stun_header *resp)
{
	return sendto(s, resp, ntohs(resp->msglen) + sizeof(*resp), 0,
		      (struct sockaddr *)dst, sizeof(*dst));
}

/* helper function to generate a random request id */
static void stun_req_id(struct stun_header *req)
{
	int x;
#ifdef WIN32
    srand(time(0));
#else
	srandom(time(0));
#endif
	for (x = 0; x < 4; x++)
#ifdef WIN32
        req->id.id[x] = rand();
#else
        req->id.id[x] = random();
#endif
}

/* callback type to be invoked on stun responses. */
typedef int (stun_cb_f)(struct stun_attr *attr, void *arg);

/* handle an incoming STUN message.
 *
 * Do some basic sanity checks on packet size and content,
 * try to extract a bit of information, and possibly reply.
 * At the moment this only processes BIND requests, and returns
 * the externally visible address of the request.
 * If a callback is specified, invoke it with the attribute.
 */
static int stun_handle_packet(int s, struct sockaddr_in *src,
	unsigned char *data, size_t len, stun_cb_f *stun_cb, void *arg)
{
	struct stun_header *hdr = (struct stun_header *)data;
	struct stun_attr *attr;
	struct stun_state st;
	int ret = STUN_IGNORE;
	int x;

	/* On entry, 'len' is the length of the udp payload. After the
	 * initial checks it becomes the size of unprocessed options,
	 * while 'data' is advanced accordingly.
	 */
	if (len < sizeof(struct stun_header)) {
		fprintf(stderr, "Runt STUN packet (only %d, wanting at least %d)\n", (int) len, (int) sizeof(struct stun_header));
		return -1;
	}
	len -= sizeof(struct stun_header);
	data += sizeof(struct stun_header);
	x = ntohs(hdr->msglen);	/* len as advertised in the message */
	if (stundebug)
		fprintf(stderr, "STUN Packet, msg %s (%04x), length: %d\n", stun_msg2str(ntohs(hdr->msgtype)), ntohs(hdr->msgtype), x);
	if (x > len) {
		fprintf(stderr, "Scrambled STUN packet length (got %d, expecting %d)\n", x, (int)len);
	} else
		len = x;
    memset(&st, 0, sizeof(st));   //bzero(&st, sizeof(st));
	while (len) {
		if (len < sizeof(struct stun_attr)) {
			fprintf(stderr, "Runt Attribute (got %d, expecting %d)\n", (int)len, (int) sizeof(struct stun_attr));
			break;
		}
		attr = (struct stun_attr *)data;
		/* compute total attribute length */
		x = ntohs(attr->len) + sizeof(struct stun_attr);
		if (x > len) {
			fprintf(stderr, "Inconsistent Attribute (length %d exceeds remaining msg len %d)\n", x, (int)len);
			break;
		}
		if (stun_cb)
			stun_cb(attr, arg);
		if (stun_process_attr(&st, attr)) {
			fprintf(stderr, "Failed to handle attribute %s (%04x)\n", stun_attr2str(ntohs(attr->attr)), ntohs(attr->attr));
			break;
		}
		/* Clear attribute id: in case previous entry was a string,
		 * this will act as the terminator for the string.
		 */
		attr->attr = 0;
		data += x;
		len -= x;
	}
	/* Null terminate any string.
	 * XXX NOTE, we write past the size of the buffer passed by the
	 * caller, so this is potentially dangerous. The only thing that
	 * saves us is that usually we read the incoming message in a
	 * much larger buffer
	 */
	*data = '\0';

	/* Now prepare to generate a reply, which at the moment is done
	 * only for properly formed (len == 0) STUN_BINDREQ messages.
	 */

#ifdef STUN_BINDREQ_PROCESS
	if (len == 0) {
		unsigned char respdata[1024];
		struct stun_header *resp = (struct stun_header *)respdata;
		int resplen = 0;	// len excluding header
		int respleft = sizeof(respdata) - sizeof(struct stun_header);

		resp->id = hdr->id;
		resp->msgtype = 0;
		resp->msglen = 0;
		attr = (struct stun_attr *)resp->ies;
		switch (ntohs(hdr->msgtype)) {
		case STUN_BINDREQ:
			if (stundebug)
				fprintf(stderr, "STUN Bind Request, username: %s\n", 
					    st.username ? st.username : "<none>");
			if (st.username)
				append_attr_string(&attr, STUN_USERNAME, st.username, &resplen, &respleft);
			append_attr_address(&attr, STUN_MAPPED_ADDRESS, src, &resplen, &respleft);
			resp->msglen = htons(resplen);
			resp->msgtype = htons(STUN_BINDRESP);
			stun_send(s, src, resp);
			ret = STUN_ACCEPT;
			break;
		default:
			if (stundebug)
				fprintf(stderr, "Dunno what to do with STUN message %04x (%s)\n",
					ntohs(hdr->msgtype), stun_msg2str(ntohs(hdr->msgtype)));
			break;
		}
	}
#endif
	return ret;
}

/* Extract the STUN_MAPPED_ADDRESS from the stun response.
 * This is used as a callback for stun_handle_response
 * when called from stun_request.
 */
static int stun_get_mapped(struct stun_attr *attr, void *arg)
{
	struct stun_addr *addr = (struct stun_addr *)(attr + 1);
	struct sockaddr_in *sa = (struct sockaddr_in *)arg;

	if (ntohs(attr->attr) != STUN_MAPPED_ADDRESS || ntohs(attr->len) != 8)
		return 1;	/* not us. */
	sa->sin_port = addr->port;
	sa->sin_addr.s_addr = addr->addr;
	return 0;
}

/* Generic STUN request
 * Send a generic stun request to the server specified,
 * possibly waiting for a reply and filling the 'reply' field with
 * the externally visible address. Note that in this case the request
 * will be blocking.
 * (Note, the interface may change slightly in the future).
 *
 * \param s the socket used to send the request
 * \param dst the address of the STUN server
 * \param username if non null, add the username in the request
 * \param answer if non null, the function waits for a response and
 *    puts here the externally visible address.
 * \return 0 on success, other values on error.
 */
int stun_request(int s, struct sockaddr_in *dst,
	const char *username, struct sockaddr_in *answer)
{
	struct stun_header *req;
	unsigned char reqdata[1024];
	int reqlen, reqleft;
	struct stun_attr *attr;
	int res = 0;
	int retry;

	req = (struct stun_header *)reqdata;
	stun_req_id(req);
	reqlen = 0;
	reqleft = sizeof(reqdata) - sizeof(struct stun_header);
	req->msgtype = 0;
	req->msglen = 0;
	attr = (struct stun_attr *)req->ies;
#ifdef STUN_BINDREQ_PROCESS
	if (username)
		append_attr_string(&attr, STUN_USERNAME, username, &reqlen, &reqleft);
#endif
	req->msglen = htons(reqlen);
	req->msgtype = htons(STUN_BINDREQ);
	for (retry = 0; retry < stuncount; retry++) {
		/* send request, possibly wait for reply */
		unsigned char reply_buf[1024];
		fd_set rfds;
		struct timeval to = { STUN_TIMEOUT, 0 };
		struct sockaddr_in src;
		socklen_t srclen;

		res = stun_send(s, dst, req);
		if (res < 0) {
			fprintf(stderr, "Request send #%d failed error %d, retry\n",
				retry, res);
			continue;
		}
		if (answer == NULL)
			break;
		FD_ZERO(&rfds);
		FD_SET(s, &rfds);
		res = select(s + 1, &rfds, NULL, NULL, &to);
		if (res <= 0) {	/* timeout or error */
			fprintf(stderr, "Response read timeout #%d failed error %d, retry\n",
				retry, res);
			continue;
		}
        memset(&src, 0, sizeof(src));     //bzero(&src, sizeof(src));
		srclen = sizeof(src);
		/* XXX pass -1 in the size, because stun_handle_packet might
		 * write past the end of the buffer.
		 */
		res = recvfrom(s, reply_buf, sizeof(reply_buf) - 1,
			0, (struct sockaddr *)&src, &srclen);
		if (res <= 0) {
			fprintf(stderr, "Response read #%d failed error %d, retry\n",
				retry, res);
			continue;
		}
        memset(&answer, 0, sizeof(struct sockaddr_in));     //bzero(answer, sizeof(struct sockaddr_in));
		stun_handle_packet(s, &src, reply_buf, res, stun_get_mapped, answer);
		return 0;
	}
	return -1;
}

static void usage(char *name)
{
	fprintf(stderr, "Minimalistic STUN client ver.%s\n", VERSION);
	fprintf(stderr, "Usage: %s [-p port] [-c count] [-d] stun_server\n", PACKAGE);
}

/*
int main(int argc, char *argv[])
{
	int sock, opt, res;
	struct sockaddr_in server,client,mapped;
	struct hostent *hostinfo;

	while ((opt = getopt(argc, argv, "p:c:t:dh")) != -1) {
		switch (opt) {
			case 'p':
				stunport = atoi(optarg);
				break;
			case 'c':
				stuncount = atoi(optarg);
				break;
			case 'd':
				stundebug++;
				break;
			default:
				usage(argv[0]);
				return -1;
			}
	}

	if (optind < argc)
		stunserver = argv[optind];

	hostinfo = gethostbyname(stunserver);
	if (!hostinfo) {
		fprintf(stderr, "Error resolving host %s\n", stunserver);
		return -1;
	}
	bzero(&server, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr = *(struct in_addr*) hostinfo->h_addr;
	server.sin_port = htons(stunport);

	sock = socket(AF_INET, SOCK_DGRAM, 0);
	if( sock < 0 ) {
		fprintf(stderr, "Error creating socket\n");
		return -1;
	}

	bzero(&client, sizeof(client));
	client.sin_family = AF_INET;
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	client.sin_port = 0;
        if (bind(sock, (struct sockaddr*)&client, sizeof(client)) < 0) {
		fprintf(stderr, "Error bind to socket\n");
		close(sock);
		return -1;
	}
	res = stun_request(sock, &server, NULL, &mapped);
	if (!res && (mapped.sin_addr.s_addr != htonl(INADDR_ANY)))
		printf("%s\n",inet_ntoa(mapped.sin_addr));

	close(sock);
	return res;
}
*/



