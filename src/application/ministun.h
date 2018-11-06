/*
 * ministun.h
 * Part of the ministun package
 *
 * STUN support code borrowed from Asterisk -- An open source telephony toolkit.
 * Copyright (C) 1999 - 2006, Digium, Inc.
 * Mark Spencer <markster@digium.com>
 * Standalone remake (c) 2009 Vladislav Grishenko <themiron@mail.ru>
 *
 * This software is licensed under the terms of the GNU General
 * Public License (GPL). Please see the file COPYING for details.
 *
 */

#define PACKAGE		"ministun"
#define VERSION		"0.1"

#define STUN_SERVER "stun.xten.com"
#define STUN_PORT 3478
#define STUN_COUNT 3
#define STUN_TIMEOUT 3

#ifndef __attribute__
#define __attribute__(a)
#endif


#ifdef WIN32
#pragma pack(push,1)
#endif
typedef struct {
    unsigned int id[4];
} __attribute__((packed)) stun_trans_id;
#ifdef WIN32
#pragma pack(pop)
#endif

#ifdef WIN32
#pragma pack(push,1)
#endif
struct stun_header {
    unsigned short msgtype;
    unsigned short msglen;
    stun_trans_id  id;
    unsigned char  ies[0];
} __attribute__((packed));
#ifdef WIN32
#pragma pack(pop)
#endif


#ifdef WIN32
#pragma pack(push,1)
#endif
struct stun_attr {
    unsigned short attr;
    unsigned short len;
    unsigned char  value[0];
} __attribute__((packed));
#ifdef WIN32
#pragma pack(pop)
#endif

/*
 * The format normally used for addresses carried by STUN messages.
 */
#ifdef WIN32
#pragma pack(push,1)
#endif
struct stun_addr {
    unsigned char  unused;
    unsigned char  family;
    unsigned short port;
    unsigned int   addr;
} __attribute__((packed));
#ifdef WIN32
#pragma pack(pop)
#endif


#define STUN_IGNORE		(0)
#define STUN_ACCEPT		(1)

/* STUN message types
 * 'BIND' refers to transactions used to determine the externally
 * visible addresses. 'SEC' refers to transactions used to establish
 * a session key for subsequent requests.
 * 'SEC' functionality is not supported here.
 */

#define STUN_BINDREQ	0x0001
#define STUN_BINDRESP	0x0101
#define STUN_BINDERR	0x0111
#define STUN_SECREQ	0x0002
#define STUN_SECRESP	0x0102
#define STUN_SECERR	0x0112

/* Basic attribute types in stun messages.
 * Messages can also contain custom attributes (codes above 0x7fff)
 */
#define STUN_MAPPED_ADDRESS	0x0001
#define STUN_RESPONSE_ADDRESS	0x0002
#define STUN_CHANGE_REQUEST	0x0003
#define STUN_SOURCE_ADDRESS	0x0004
#define STUN_CHANGED_ADDRESS	0x0005
#define STUN_USERNAME		0x0006
#define STUN_PASSWORD		0x0007
#define STUN_MESSAGE_INTEGRITY	0x0008
#define STUN_ERROR_CODE		0x0009
#define STUN_UNKNOWN_ATTRIBUTES	0x000a
#define STUN_REFLECTED_FROM	0x000b
