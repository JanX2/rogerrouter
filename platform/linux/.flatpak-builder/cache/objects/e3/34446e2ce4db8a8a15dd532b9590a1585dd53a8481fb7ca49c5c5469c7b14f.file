/*****************************************************************
 * CAPI 2.0 Library                                              *
 * All parts are distributed under the terms of GPL. See COPYING *
 *****************************************************************/

/**
 * \file capi_mod_standard.c
 * \brief Standard CAPI interface
 */

#include <sys/socket.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/capi.h>
#include <unistd.h>
#include "capi20.h"
#include "capi_mod.h"

#define CAPI_DEVICE_NAME 		"/dev/capi20"
#define CAPI_DEVICE_NAME_NEW	"/dev/isdn/capi20"

static capi_ioctl_struct    ioctl_data;

/**
 * \brief Check if standard interface is available
 * \return file descriptor of socket
 */
static int standardIsInstalled( void ) {
	int nHandle;

	if ( ( nHandle = open( CAPI_DEVICE_NAME, O_RDWR, 0666 ) ) < 0 && ( errno == ENOENT ) ) {
		nHandle = open( CAPI_DEVICE_NAME_NEW, O_RDWR, 0666 );
	}

	if ( nHandle >= 0 ) {
		if ( ioctl( nHandle, CAPI_INSTALLED, 0 ) != 0 ) {
			nHandle = -1;
		}
	}

	return nHandle;
}

/**
 * \brief Register standard capi device
 * \param nMaxB3Connection maximum b3 connection
 * \param nMaxB3Blks maximum b3 blocks
 * \param nMaxSizeB3 maximum b3 size
 * \param pnApplId pointer where to store the new application id
 * \return new file handle
 */
static unsigned standardRegister( unsigned nMaxB3Connection, unsigned nMaxB3Blks, unsigned nMaxSizeB3, unsigned *pnApplId ) {
	char anBuf[ PATH_MAX ];
	int nApplId;
	int nIndex;
	int nHandle;

	if ( ( nHandle = open( CAPI_DEVICE_NAME, O_RDWR | O_NONBLOCK, 0666 ) ) < 0 && ( errno == ENOENT ) ) {
		nHandle = open( CAPI_DEVICE_NAME_NEW, O_RDWR | O_NONBLOCK, 0666 );
	}

	if (nHandle < 0) {
		return nHandle;
	}

	ioctl_data.rparams.level3cnt = nMaxB3Connection;
	ioctl_data.rparams.datablkcnt = nMaxB3Blks;
	ioctl_data.rparams.datablklen = nMaxSizeB3;

	if ( ( nApplId = ioctl(nHandle, CAPI_REGISTER, &ioctl_data ) ) < 0 ) {
		if ( errno == EIO ) {
			if ( ioctl( nHandle, CAPI_GET_ERRCODE, &ioctl_data ) < 0 ) {
				close( nHandle );
				return -2;
			}
			close( nHandle );
			return -2;
		} else if ( errno == EINVAL ) {
			/* old kernel driver */
			close( nHandle );

			nHandle = -1;
			for ( nIndex = 0; nHandle < 0; nIndex++ ) {
				/*----- open pseudo-clone device -----*/
				snprintf( anBuf, sizeof( anBuf ), "/dev/capi20.%02d", nIndex );
				if ( ( nHandle = open( anBuf, O_RDWR | O_NONBLOCK, 0666 ) ) < 0 ) {
					switch ( errno ) {
						case EEXIST:
							break;
						default:
							return -3;
					}
				}
			}

			if ( nHandle < 0 ) {
				return -3;
			}

			ioctl_data.rparams.level3cnt = nMaxB3Connection;
			ioctl_data.rparams.datablkcnt = nMaxB3Blks;
			ioctl_data.rparams.datablklen = nMaxSizeB3;

			if ( ( nApplId = ioctl( nHandle, CAPI_REGISTER, &ioctl_data ) ) < 0 ) {
				if ( errno == EIO ) {
					if ( ioctl( nHandle, CAPI_GET_ERRCODE, &ioctl_data ) < 0 ) {
						close( nHandle );
						return -4;
					}

					close( nHandle );
					return -5;
				}
				close( nHandle );
				return -6;
			}
			nApplId = capi_alloc_applid(nHandle);
		}
	}

	*pnApplId = nApplId;

	return nHandle;
}

/**
 * \brief Put capi message
 * \param nHandle file handle
 * \param nApplId application id
 * \param pnMsg message pointer
 * \return error code
 */
static unsigned standardPutMessage( int nHandle, unsigned nApplId, unsigned char *pnMsg ) {
	int nLen = CAPIMSG_LEN( pnMsg );
	int nCommand = CAPIMSG_COMMAND( pnMsg );
	int nSubCommand = CAPIMSG_SUBCOMMAND( pnMsg );
	int nNum;

	nLen = capi_processMessage( pnMsg, nApplId, nCommand, nSubCommand, nLen );

	nNum = write( nHandle, pnMsg, nLen );
	if ( nNum != nLen ) {
		switch ( errno ) {
			case EFAULT:
			case EINVAL:
				return CapiIllCmdOrSubcmdOrMsgToSmall;
			case EBADF:
				return CapiIllAppNr;
			case EIO:
				if ( ioctl( nHandle, CAPI_GET_ERRCODE, &ioctl_data ) < 0 ) {
					return CapiMsgOSResourceErr;
				} else {
					return ioctl_data.errcode;
				}
	          default:
				return CapiMsgOSResourceErr;
		}
	}

    return CapiNoError;
}

/**
 * \brief Get capi message
 * \param nHandle file handle
 * \param nApplId application id
 * \param pnMsg message pointer
 * \return error code
 */
static unsigned standardGetMessage( int nHandle, unsigned nApplId, unsigned char **ppnBuffer ) {
	unsigned char *pnBuffer;
	unsigned nOffset;
	int nRet;
	size_t nBufSize;

	if ( ( *ppnBuffer = pnBuffer = ( unsigned char * ) capi_get_buffer( nApplId, &nBufSize, &nOffset ) ) == 0 ) {
		return CapiMsgOSResourceErr;
	}

	nRet = read( nHandle, pnBuffer, nBufSize );
	if ( nRet > 0 ) {
		/* workaround for old driver */
		CAPIMSG_SETAPPID( pnBuffer, nApplId );

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
 * \brief Get manufactor
 * \param nHandle file handle
 * \param nController controller number
 * \param pnBuffer where to store the information
 * \return pnBuffer or NULL on error
 */
static unsigned char *standardGetManufactor( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
    ioctl_data.contr = nController;

	if ( ioctl( nHandle, CAPI_GET_MANUFACTURER, &ioctl_data ) < 0 ) {
		return NULL;
	}

	memcpy( pnBuffer, ioctl_data.manufacturer, CAPI_MANUFACTURER_LEN );
	pnBuffer[ CAPI_MANUFACTURER_LEN - 1 ] = 0;

	return pnBuffer;
}

/**
 * \brief Get version
 * \param nHandle file handle
 * \param nController controller number
 * \param pnBuffer where to store the information
 * \return pnBuffer or NULL on error
 */
static unsigned char *standardGetVersion( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	ioctl_data.contr = nController;

	if ( ioctl( nHandle, CAPI_GET_VERSION, &ioctl_data ) < 0 ) {
		return NULL;
	}

	memcpy( pnBuffer, &ioctl_data.version, sizeof( capi_version ) );
	return pnBuffer;
}

/**
 * \brief Get serial number
 * \param nHandle file handle
 * \param nController controller number
 * \param pnBuffer where to store the information
 * \return pnBuffer or NULL on error
 */
static unsigned char *standardGetSerialNumber( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	ioctl_data.contr = nController;

	if ( ioctl( nHandle, CAPI_GET_SERIAL, &ioctl_data ) < 0 ) {
		return NULL;
	}

	memcpy( pnBuffer, &ioctl_data.serial, CAPI_SERIAL_LEN );
	pnBuffer[ CAPI_SERIAL_LEN - 1 ] = 0;

	return pnBuffer;
}

/**
 * \brief Get profile from standard
 * \param nHandle socket handle
 * \param nController controller
 * \param pnBuffer buffer
 * \return error code
 */
static unsigned standardGetProfile( int nHandle, unsigned nController, unsigned char *pnBuffer ) {
	ioctl_data.contr = nController;

	if ( ioctl( nHandle, CAPI_GET_PROFILE, &ioctl_data ) < 0 ) {
		if ( errno != EIO ) {
			return CapiMsgOSResourceErr;
		}
		if ( ioctl( nHandle, CAPI_GET_ERRCODE, &ioctl_data ) < 0 ) {
			return CapiMsgOSResourceErr;
		}

		return ( unsigned ) ioctl_data.errcode;
	}

	if ( nController ) {
		memcpy( pnBuffer, &ioctl_data.profile, sizeof( struct capi_profile ) );
	} else {
		memcpy( pnBuffer, &ioctl_data.profile.ncontroller, sizeof( ioctl_data.profile.ncontroller ) );
	}

	return CapiNoError;
}

/**
 * \brief Get flags
 * \param nApplId application id
 * \param pnFlags where to save flags
 * \return error code
 */
static int standardGetFlags( unsigned nApplId, unsigned *pnFlags ) {
	if ( ioctl(capi_applid2fd( nApplId ), CAPI_GET_FLAGS, pnFlags) < 0) {
		return CapiMsgOSResourceErr;
	}

	return CapiNoError;
}

/**
 * \brief Set flags
 * \param nApplId application id
 * \param nFlags flags to set
 * \return error code
 */
static int standardSetFlags( unsigned nApplId, unsigned nFlags ) {
	if ( ioctl(capi_applid2fd( nApplId ), CAPI_SET_FLAGS, &nFlags) < 0) {
		return CapiMsgOSResourceErr;
	}

	return CapiNoError;
}

/**
 * \brief Clear flags
 * \param nApplId application id
 * \param nFlags flags to clear
 * \return error code
 */
static int standardClearFlags( unsigned nApplId, unsigned nFlags ) {
	if ( ioctl(capi_applid2fd( nApplId ), CAPI_CLR_FLAGS, &nFlags) < 0) {
		return CapiMsgOSResourceErr;
	}

	return CapiNoError;
}

/**
 * \brief Get tty device name
 * \param nApplId application id
 * \param nNcci network control connection identifier
 * \param pnBuffer buffer pointer
 * \param nSize size of buffer
 * \return pnBuffer or NULL on error
 */
static char *standardGetTtyDeviceName( unsigned nApplId, unsigned nNcci, char *pnBuffer, size_t nSize ) {
	int nUnit;

	nUnit = ioctl(capi_applid2fd( nApplId ), CAPI_NCCI_GETUNIT, &nNcci);
	if ( nUnit < 0 ) {
		return NULL;
	}

	snprintf( pnBuffer, nSize, "/dev/capi/%d", nUnit );

	return pnBuffer;
}

/**
 * \brief Get raw device name
 * \param nApplId application id
 * \param nNcci network control connection identifier
 * \param pnBuffer buffer pointer
 * \param nSize size of buffer
 * \return pnBuffer or NULL on error
 */
static char *standardGetRawDeviceName( unsigned nApplId, unsigned nNcci, char *pnBuffer, size_t nSize ) {
	int nUnit;

	nUnit = ioctl(capi_applid2fd( nApplId ), CAPI_NCCI_GETUNIT, &nNcci);
	if ( nUnit < 0 ) {
		return NULL;
	}

	snprintf( pnBuffer, nSize, "/dev/capi/r%d", nUnit );

	return pnBuffer;
}

/**
 * \brief Get NCCI Open count
 * \param nApplId application id
 * \param nNcci network control connection identifier
 * \return error code
 */
static int standardGetNcciOpenCount( unsigned nApplId, unsigned nNcci ) {
	return ioctl(capi_applid2fd( nApplId ), CAPI_NCCI_OPENCOUNT, &nNcci);
}

/** Module operations structure */
static struct sModuleOperations sStandard = {
	standardIsInstalled,
	standardRegister,
	NULL,
	standardPutMessage,
	standardGetMessage,
	standardGetManufactor,
	standardGetVersion,
	standardGetSerialNumber,
	standardGetProfile,
	NULL,
	standardGetFlags,
	standardSetFlags,
	standardClearFlags,
	standardGetTtyDeviceName,
	standardGetRawDeviceName,
	standardGetNcciOpenCount
};

MODULE_INIT( "standard", &sStandard );
