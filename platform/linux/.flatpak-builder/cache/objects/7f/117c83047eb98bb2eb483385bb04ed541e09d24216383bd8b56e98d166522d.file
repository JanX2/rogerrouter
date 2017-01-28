/*
 * capi_mod.h
 *
 * Author	Jan-Michael Brummer
 * Author	Marco Zissen
 *
 * Copyright 2008  by Jan-Michael Brummer, Marco Zissen
 *
 * Author	Karsten Keil <kkeil@linux-pingi.de>
 *
 * Copyright 2011  by Karsten Keil <kkeil@linux-pingi.de>
 *
 * This code is free software; you can redistribute it and/or modify
 * it under the terms of the GNU LESSER GENERAL PUBLIC LICENSE
 * version 2.1 as published by the Free Software Foundation.
 *
 * This code is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU LESSER GENERAL PUBLIC LICENSE for more details.
 *
 */

#ifndef CAPI_MOD_H
#define CAPI_MOD_H

//#include <capi20.h>

/* define ulong if not defined */
#ifndef C_ULONG_DEFINED
typedef unsigned long ulong;
#endif

struct sModuleOperations {
	int ( *IsInstalled )( void );
	unsigned ( *Register )( unsigned nMaxB3Connection, unsigned nMaxB3Blks, unsigned nMaxSizeB3, unsigned *pnApplId );
	unsigned ( *Release )( int nHandle, int nApplId );
	unsigned ( *PutMessage )( int nHandle, unsigned nApplId, unsigned char *pnMessage );
	unsigned ( *GetMessage )( int nHandle, unsigned nApplId, unsigned char **ppnBuffer );
	unsigned char *( *GetManufactor )( int nHandle, unsigned nController, unsigned char *pnBuffer );
	unsigned char *( *GetVersion )( int nHandle, unsigned nController, unsigned char *pnBuffer );
	unsigned char *( *GetSerialNumber )( int nHandle, unsigned nController, unsigned char *pnBuffer );
	unsigned ( *GetProfile )( int nHandle, unsigned nController, unsigned char *pnBuffer );
	unsigned ( *waitformessage)(int nHandle, unsigned ApplID, struct timeval *TimeOut);
	/* The following parts are only used on the standard capi device */
	int ( *GetFlags )( unsigned nApplId, unsigned *pnFlagsPtr );
	int ( *SetFlags )( unsigned nApplId, unsigned nFlags );
	int ( *ClearFlags )( unsigned nApplId, unsigned nFlags );
	char *( *GetTtyDeviceName )( unsigned nApplId, unsigned nNcci, char *pnBuffer, size_t nSize );
	char *( *GetRawDeviceName )( unsigned nApplId, unsigned nNcci, char *pnBuffer, size_t nSize );
	int ( *GetNcciOpenCount )( unsigned nApplId, unsigned nNcci );
} __attribute__ ((packed));

struct sModule {
	char *pnName;
	int nVersion;
	void *pHandle;
	struct sModuleOperations *psOperations;
} __attribute__ ((packed));

#define MODULE_LOADER_VERSION		2

#define MODULE_INIT( NAME, OPS )\
	int InitModule( struct sModule *psModule ) {\
		psModule -> pnName = strdup( NAME );\
		psModule -> nVersion = MODULE_LOADER_VERSION;\
		psModule -> psOperations = OPS;\
		return 0;\
	}

#endif
