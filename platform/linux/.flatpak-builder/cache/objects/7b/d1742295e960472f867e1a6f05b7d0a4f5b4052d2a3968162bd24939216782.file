/*****************************************************************
 * CAPI 2.0 Library                                              *
 * All parts are distributed under the terms of GPL. See COPYING *
 *****************************************************************/

/**
 * \file capi_mod_rcapi.c
 * \brief Melware remote capi implementation
 */

#ifndef __WIN32__
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#else
#include <winsock2.h>

#define EMSGSIZE WSAEMSGSIZE

typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned int u_int32_t;
typedef unsigned long u_int64_t;
#endif

#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include "capi_defs.h"
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include "capi20.h"
#include "capi_mod.h"
#include "capi_mod_fritzbox.h"

/* REMOTE-CAPI commands */
#define RCAPI_REGISTER_REQ                      CAPICMD(0xf2, 0xff)
#define RCAPI_REGISTER_CONF                     CAPICMD(0xf3, 0xff)
#define RCAPI_GET_MANUFACTURER_REQ              CAPICMD(0xfa, 0xff)
#define RCAPI_GET_MANUFACTURER_CONF             CAPICMD(0xfb, 0xff)
#define RCAPI_GET_SERIAL_NUMBER_REQ             CAPICMD(0xfe, 0xff)
#define RCAPI_GET_SERIAL_NUMBER_CONF            CAPICMD(0xff, 0xff)
#define RCAPI_GET_VERSION_REQ                   CAPICMD(0xfc, 0xff)
#define RCAPI_GET_VERSION_CONF                  CAPICMD(0xfd, 0xff)
#define RCAPI_GET_PROFILE_REQ                   CAPICMD(0xe0, 0xff)
#define RCAPI_GET_PROFILE_CONF                  CAPICMD(0xe1, 0xff)
#define RCAPI_AUTH_USER_REQ                     CAPICMD(0xff, 0x00)
#define RCAPI_AUTH_USER_CONF                    CAPICMD(0xff, 0x01)

/* add a fixed trace file for now */
static char *rcapiTraceFile = "/tmp/rcapiTraceFile";
static char *getTraceFile(void)
{
	return rcapiTraceFile;
}

/**
 * \brief Create a socket to hostname:port
 * \return socket number
 */
static int rcapiOpenSocket( void ) {
	struct hostent *psHostInfo;
	struct sockaddr_in sAddr;
	int nHandle;

	/* Create new socket */
	nHandle = socket( PF_INET, SOCK_STREAM, 0 );
	if ( nHandle == -1 ) {
		return nHandle;
	}

	sAddr.sin_family = PF_INET;
	sAddr.sin_port = htons( capi20ext_get_port() );
	/* Retrieve hostname information */
	psHostInfo = gethostbyname( capi20ext_get_host() );
	if ( psHostInfo != NULL ) {
		sAddr.sin_addr = *( struct in_addr * ) psHostInfo -> h_addr;

		/* Connect socket to address */
		if ( !connect( nHandle, ( struct sockaddr * ) &sAddr, sizeof( sAddr ) ) ) {
			/* no errors, return handle */
			return nHandle;
		}
	}

	close( nHandle );
	return -1;
}

/**
 * \brief Read data from socket
 * \param nHandle socket handle
 * \param pnBuffer buffer pointer
 * \param nLen length of buffer
 * \return read len
 */
static int rcapiReadSocket( int nHandle, unsigned char *pnBuffer, int nLen ) {
	unsigned char anTemp[ 4096 ], *pnPtr;
	int nTotLen, nReadLen, nActLen, nOrigLen;
	time_t nTime;

	/* try to read the header, 2 bytes */
	if( recv( nHandle, ( char * ) anTemp, 2, 0 ) == 2 ) {
		pnPtr = anTemp;
		/* Get message len */
		nTotLen = nOrigLen = ( int ) get_netword( &pnPtr ) - 2;
		nTime = time( NULL );
		nReadLen = 0;

		/* read message */
		while ( ( ( nActLen = recv( nHandle, ( char * ) &anTemp[ nReadLen ], nTotLen, 0 ) ) < nTotLen) && ( time( NULL ) < ( nTime + 5 ) ) ) {
			if ( nActLen > 0 ) {
				nTotLen -= nActLen;
				nReadLen += nActLen;
			}
			nActLen = 0;
		}
		if ( nActLen > 0 ) {
			nReadLen += nActLen;
		}

		if ( nReadLen != nOrigLen ) {
			return 0;
		}

		if ( !nLen ) {
			nLen = nOrigLen;
		}
		nOrigLen = ( nLen < nReadLen ) ? nLen : nReadLen;
		/* copy informations to buffer, cut to nLen if needed */
		memcpy( pnBuffer, anTemp, nOrigLen );

		return nOrigLen;
	}

	/* could not read header, return 0 */
	return 0;
}

/**
 * \brief Send message to socket and wait for response
 * \param nHandle socket handle
 * \param pnBuffer data buffer pointer
 * \param nLen number of bytes to write from pnBuffer
 * \param nConf current configuration id
 * \return number of bytes read
 */
static int rcapiRemoteCommand( int nHandle, unsigned char *pnBuffer, int nLen, int nConf ) {
	unsigned char *pnPtr;

	/* write message to socket */
	if ( send( nHandle, ( char * ) pnBuffer, nLen, 0 ) < nLen ) {
		return 0;
	}

	/* read data */
	if ( ( nLen = rcapiReadSocket( nHandle, pnBuffer, 0 ) ) < 1 ) {
		return 0;
	}

	pnPtr = pnBuffer + 4;

	/* check if'we got the current configuration */
	if ( get_netword( &pnPtr ) == nConf ) {
		memmove( pnBuffer, pnBuffer + 8, nLen - 8 );
		return nLen - 8;
	}

	return 0;
}

/**
 * \brief Add standard rcapi header to buffer pointer
 * \param ppnPtr data buffer pointer
 * \param nLen length of message
 * \param nCmd command id
 * \param nCtrl controller
 */
static void rcapiSetHeader( unsigned char **ppnPtr, int nLen, _cword nCmd, _cdword nCtrl ) {
	/* length of message */
	put_netword( ppnPtr, nLen );
	put_word( ppnPtr, 0 );
	put_word( ppnPtr, 0 );
	/* command id */
	put_netword( ppnPtr, nCmd );
	put_word( ppnPtr, 0 );
	/* controller id */
	put_dword( ppnPtr, nCtrl );
}

/**
 * \brief Debug purpose, write capi data to file
 * \param nSend send
 * \param pnBuffer data buffer
 * \param nLength length of buffer
 * \param nDataMsg data message len
 */
void rcapiWriteCapiTrace(int nSend, unsigned char *pnBuffer, int nLength, int nDataMsg ) {
	int nHandle;
	_cdword nTime;
	unsigned char anHeader[ 7 ];
	char *pnTraceFile = getTraceFile();

	if ( strlen( pnTraceFile ) <= 0 ) {
		return;
	}

	if ( capi20ext_get_tracelevel() < ( nDataMsg + 1 ) ) {
		return;
	}

	nHandle = open( pnTraceFile, O_WRONLY | O_CREAT | O_APPEND, 0644 );
	if ( nHandle >= 0 ) {
		nTime = ( _cdword ) time( NULL );
		capimsg_setu16( anHeader, 0, nLength + sizeof( anHeader ) );
		capimsg_setu32( anHeader, 2, nTime );
		anHeader[ 6 ] = ( nSend ) ? 0x80 : 0x81;
		close( nHandle );
	}
}

/**
 * \brief Check if rcapi interface is available
 * \return file descriptor of socket, or error code
 */
static int rcapiIsInstalled( void ) {
	int nHandle;

	if ( strlen( capi20ext_get_host() ) <= 0 || capi20ext_get_port() == -1 ) {
		return -1;
	}

	/* we check if we can open a new socket to hostname:port */
	nHandle = rcapiOpenSocket();

	/* return new handle or error code */
	return nHandle;
}

/**
 * \brief Register at rcapi
 * \param nMaxB3Connection maximum b3 connection
 * \param nMaxB3Blks maximum b3 blocks
 * \param nMaxSizeB3 maximum b3 size
 * \param pnApplId pointer where we store the new application id
 * \return new socket handle
 */
static unsigned rcapiRegister( unsigned nMaxB3Connection, unsigned nMaxB3Blks, unsigned nMaxSizeB3, unsigned *pnApplId ) {
	unsigned char anBuf[ 100 ], *pnPtr = anBuf;
	int nSock, nErrCode;;

	/* Safety: Set application id to invalid (-1) */
	*pnApplId = -1;

	/* open a new socket for communication */
	nSock = rcapiOpenSocket();
	if ( nSock < 0 ) {
		return nSock;
	}

	rcapiSetHeader( &pnPtr, 23, RCAPI_REGISTER_REQ, 0 );
	/* Fake size */
	put_word( &pnPtr, 2048 );
	put_word( &pnPtr, nMaxB3Connection );
	put_word( &pnPtr, nMaxB3Blks );
	put_word( &pnPtr, nMaxSizeB3 );
	/* CAPI version */
	put_word( &pnPtr, 2 );

	/* Send message to socket and wait for answer */
	if ( !( rcapiRemoteCommand( nSock, anBuf, 23, RCAPI_REGISTER_CONF ) ) ) {
		close( nSock );
		return -2;
	}

	pnPtr = anBuf;
	nErrCode = get_word( &pnPtr );
	if ( nErrCode == CapiNoError ) {
		/* No error, allocate new applid and set it to pnApplId */
		*pnApplId = capi_alloc_applid(nSock);
	} else {
		/* wuhh, error occured, close socket and return -1 */
		close( nSock );
		nSock = -1;
	}

	return nSock;
}

/**
 * \brief Put capi message to rcapi
 * \param nSock socket handle
 * \param nApplId application id
 * \param pnMsg message pointer
 * \return error code
 */
static unsigned rcapiPutMessage( int nSock, unsigned nApplId, unsigned char *pnMsg ) {
	unsigned char anSendBuffer[ SEND_BUFSIZ ], *pnSendBuffer = anSendBuffer;
	int nLen = CAPIMSG_LEN( pnMsg );
	int nCommand = CAPIMSG_COMMAND( pnMsg );
	int nSubCommand = CAPIMSG_SUBCOMMAND( pnMsg );
	int nNum;

	nLen = capi_processMessage( pnMsg, nApplId, nCommand, nSubCommand, nLen );
	nLen += 2;

	put_netword( &pnSendBuffer, nLen );
	memcpy( pnSendBuffer, pnMsg, nLen );

	/* write data to socket */
	nNum = send( nSock, ( char * ) anSendBuffer, nLen, 0 );
    if ( nNum != nLen ) {
	return CapiMsgOSResourceErr;
	}

    return CapiNoError;

}

/**
 * \brief Get message from rcapi
 * \param nSock socket handle
 * \param nApplId application id
 * \param ppnBuffer pointer to data buffer pointer (where we store the data)
 * \return error code
 */
static unsigned rcapiGetMessage( int nSock, unsigned nApplId, unsigned char **ppnBuffer ) {
	unsigned char *pnBuffer;
	unsigned nOffset;
	size_t nBufSize;
	int nRet;

	/* try to get a new buffer from queue */
	if ( ( *ppnBuffer = pnBuffer = ( unsigned char * ) capi_get_buffer( nApplId, &nBufSize, &nOffset ) ) == 0 ) {
		return CapiMsgOSResourceErr;
	}

	/* Get message */
	nRet = rcapiReadSocket( nSock, pnBuffer, nBufSize );

	if ( nRet > 0 ) {
		/* workaround for old driver */
		CAPIMSG_SETAPPID( pnBuffer, nApplId );

		/* DATA_B3? Then set buffer address */
		if ( ( CAPIMSG_COMMAND( pnBuffer ) == CAPI_DATA_B3 ) && ( CAPIMSG_SUBCOMMAND( pnBuffer ) == CAPI_IND ) ) {
			capi_save_datahandle( nApplId, nOffset, CAPIMSG_U16( pnBuffer, 18 ), CAPIMSG_U32( pnBuffer, 8 ) );
			/* patch datahandle */
			capimsg_setu16( pnBuffer, 18, nOffset );

#if SIZEOF_VOID_P == 4
			u_int32_t nData = ( u_int32_t ) pnBuffer + CAPIMSG_LEN( pnBuffer );
			pnBuffer[ 12 ] = nData & 0xFF;
			pnBuffer[ 13 ] = ( nData >> 8 ) & 0xFF;
			pnBuffer[ 14 ] = ( nData >> 16 ) & 0xFF;
			pnBuffer[ 15 ] = ( nData >> 24 ) & 0xFF;
#else
			u_int64_t nData;

			if ( CAPIMSG_LEN( pnBuffer ) < 30 ) {
				memmove( pnBuffer + 30, pnBuffer + CAPIMSG_LEN( pnBuffer ), CAPIMSG_DATALEN( pnBuffer ) );
				pnBuffer[ 0 ] = 30;
				pnBuffer[ 1 ] = 0;
			}
			nData = ( ( ( ulong ) pnBuffer ) + CAPIMSG_LEN( pnBuffer ) );
			pnBuffer[ 12 ] = pnBuffer[ 13 ] = pnBuffer[ 14 ] = pnBuffer[ 15 ] = 0;
			pnBuffer[ 22 ] = nData & 0xFF;
			pnBuffer[ 23 ] = ( nData >> 8 ) & 0xFF;
			pnBuffer[ 24 ] = ( nData >> 16 ) & 0xFF;
			pnBuffer[ 25 ] = ( nData >> 24 ) & 0xFF;
			pnBuffer[ 26 ] = ( nData >> 32 ) & 0xFF;
			pnBuffer[ 27 ] = ( nData >> 40 ) & 0xFF;
			pnBuffer[ 28 ] = ( nData >> 48 ) & 0xFF;
			pnBuffer[ 29 ] = ( nData >> 56 ) & 0xFF;
#endif

			/* keep buffer */
			return CapiNoError;
		}

		/* buffer is not needed, return it */
		capi_return_buffer( nApplId, nOffset );

		if ( ( CAPIMSG_COMMAND( pnBuffer ) == CAPI_DISCONNECT) && ( CAPIMSG_SUBCOMMAND( pnBuffer ) == CAPI_IND ) ) {
			cleanup_buffers_for_plci( nApplId, CAPIMSG_U32( pnBuffer, 8 ) );
		}

		return CapiNoError;
	}

	capi_return_buffer( nApplId, nOffset );

	if ( nRet == 0 ) {
		return CapiReceiveQueueEmpty;
	}

	switch ( errno ) {
		case EMSGSIZE:
			nRet = CapiIllCmdOrSubcmdOrMsgToSmall;
			break;
		case EAGAIN:
			nRet = CapiReceiveQueueEmpty;
			break;
		default:
			nRet = CapiMsgOSResourceErr;
			break;
	}

	return nRet;
}

/**
 * \brief Get manufactor informations
 * \param nHandle socket handle
 * \param nController controller id
 * \param pnBuffer buffer pointer we write our informations to
 * \return pnBuffer
 */
static unsigned char *rcapiGetManufactor( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	unsigned char anBuf[ 100 ];
	unsigned char *pnPtr = anBuf;

	rcapiSetHeader( &pnPtr, 14, RCAPI_GET_MANUFACTURER_REQ, nController );
	if ( !( rcapiRemoteCommand( nHandle, anBuf, 14, RCAPI_GET_MANUFACTURER_CONF ) ) ) {
		return 0;
	}

	memcpy( pnBuffer, anBuf + 1, CAPI_MANUFACTURER_LEN );
	pnBuffer[ CAPI_MANUFACTURER_LEN - 1 ] = 0;

	return pnBuffer;
}

/**
 * \brief Get version informations
 * \param nHandle socket handle
 * \param nController controller id
 * \param pnBuffer buffer pointer we write our informations to
 * \return pnBuffer
 */
static unsigned char *rcapiGetVersion( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	unsigned char anBuf[ 100] ;
	unsigned char *pnPtr = anBuf;

	rcapiSetHeader( &pnPtr, 14, RCAPI_GET_VERSION_REQ, nController );
	if ( !( rcapiRemoteCommand( nHandle, anBuf, 14, RCAPI_GET_VERSION_CONF ) ) ) {
		return 0;
	}

	memcpy( pnBuffer, anBuf + 1, sizeof( capi_version ) );

	return pnBuffer;
}

/**
 * \brief Get serial number informations
 * \param nHandle socket handle
 * \param nController controller id
 * \param pnBuffer buffer pointer we write our informations to
 * \return pnBuffer
 */
static unsigned char *rcapiGetSerialNumber( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	unsigned char anBuf[ 100 ];
	unsigned char *pnPtr = anBuf;

	rcapiSetHeader( &pnPtr, 14, RCAPI_GET_SERIAL_NUMBER_REQ, nController );
	if ( !( rcapiRemoteCommand( nHandle, anBuf, 14, RCAPI_GET_SERIAL_NUMBER_CONF ) ) ) {
		return 0;
	}

	memcpy( pnBuffer, anBuf + 1, CAPI_SERIAL_LEN );
	pnBuffer[ CAPI_SERIAL_LEN - 1 ] = 0;

	return pnBuffer;
}

/**
 * \brief Get profile from fritzbox
 * \param nHandle socket handle
 * \param nControllerId controller
 * \param pnBuf buffer
 * \return error code
 */
static unsigned rcapiGetProfile( int nHandle, unsigned nController, unsigned char *pnBuf ) {
	unsigned char anBuf[ 100] ;
	unsigned char *pnPtr = anBuf;

	rcapiSetHeader( &pnPtr, 14, RCAPI_GET_PROFILE_REQ, nController );
	if ( !( rcapiRemoteCommand( nHandle, anBuf, 14, RCAPI_GET_PROFILE_CONF ) ) ) {
		return CapiMsgOSResourceErr;
	}

	if( *( unsigned short * ) anBuf == CapiNoError) {
		memcpy( pnBuf, anBuf + 2, ( nController ) ? sizeof( struct capi_profile ) : 2 );
	}

	return *( unsigned short * ) anBuf;
}

/** Module operations structure */
static struct sModuleOperations sRemoteCapi = {
	rcapiIsInstalled,
	rcapiRegister,
	NULL,
	rcapiPutMessage,
	rcapiGetMessage,
	rcapiGetManufactor,
	rcapiGetVersion,
	rcapiGetSerialNumber,
	rcapiGetProfile,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL
};

MODULE_INIT( "rcapi", &sRemoteCapi );
