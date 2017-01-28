/*
 * CAPI 2.0 library
 *
 * 2002-03-27 - Added remote capi features.
 *              Armin Schindler <armin@melware.de>
 *
 * This program is free software and may be modified and
 * distributed under the terms of the GNU Public License.
 *
 */

#ifndef __WIN32__
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#else
#include <winsock2.h>
typedef unsigned int u_int32_t;
typedef unsigned long u_int64_t;
#endif

#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <errno.h>
#include <limits.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdarg.h>
#define _LINUX_LIST_H
#include "capi_defs.h"

#include <sys/stat.h>
#include <semaphore.h>
#ifdef HAVE_BYTESWAP_H
#include <byteswap.h>
#else
#include "compat/byteswap.h"
#endif



#include "capi20.h"
#include "capi_mod.h"
#include "capi_debug.h"

/*
 * We will use shared memory to allow unique application IDs in the system
 *
 */
#define CAPI20_SHARED_MEM_VERSION	0x01000010
#define CAPI20_SHARED_MEM_NAME		"/CAPI20_shared_memory"
#define CAPI20_SEMAPHORE_NAME		"/CAPI20_shared_sem"
#define CAPI20_SHARED_MEM_MODE		(S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH)

#define MAX_APPL 1024

#ifdef _BIG_ENDIAN
#define dword2ptr(x) (void *) bswap_32(x)
#define qword2ptr(x) (void *) bswap_64(x)
#else
#define dword2ptr(x) x
#define qword2ptr(x) x
#endif

#if __linux__
static sem_t *capi_sem;
static int shm_mem_fd = -1;
#endif

static struct shr_applids {
	unsigned int	init_done :1;
	unsigned int	usage;
	unsigned int	max_id;
	struct {
		unsigned int		inuse:1;
		int			fd;
		pid_t			pid;
		unsigned long long	para;
	} id[MAX_APPL];
	unsigned int	reserved[16 * MAX_APPL];
} *appids;

#define CAPI20_SHARED_MEM_SIZE	sizeof(*appids)

static pid_t my_pid;

static int                  capi_fd = -1;

static char *globalconfigfilename = "/etc/capi20.conf";
static char *userconfigfilename = ".capi20rc";

static short port = -1;
static char driver[1024] = "";
static char hostname[1024] = "";

static int tracelevel=0xff;
static char *tracefile;

static void lock_capi_shared(void)
{
#ifdef __linux__
	while (sem_wait(capi_sem)) {
		if (errno != EINTR) {
			fprintf(stderr, "sem_wait() returned error %d - %s\n", errno, strerror(errno));
		}
	}
#endif
}

static void unlock_capi_shared(void)
{
#ifdef __linux__
	if (sem_post(capi_sem))
		fprintf(stderr, "sem_post() returned error %d - %s\n", errno, strerror(errno));
#endif
}


static int stderr_vprint(const char *file, int line, const char *func, const char *fmt, va_list va)
{
	int ret;
	fprintf(stderr, "%20s:%4d %22s():", file, line, func);
	ret = vfprintf(stderr, fmt,  va);
	fflush(stderr);
	return ret;
}


static capi_debug_t dbg_vprintf = stderr_vprint;

int _capi_dprintf(const char *file, int line, const char *func, const char *fmt, ...)
{
	va_list args;
	int ret;

	va_start(args, fmt);
	ret = dbg_vprintf(file, line, func, fmt, args);
	va_end(args);
	return ret;
}

void register_dbg_vprintf(capi_debug_t fn)
{
	dbg_vprintf = fn;
}

/**
 * \brief CapiDebug output functions
 * \param nLevel debug level of following message
 * \param pnFormat formatted string
 */
void CapiDebug(int nLevel, const char *pnFormat, ...) {
	if (nLevel <= tracelevel) {
		va_list pArgs;

		va_start(pArgs, pnFormat);
		dbg_vprintf(__FILE__, __LINE__, __PRETTY_FUNCTION__, pnFormat, pArgs);
		va_end(pArgs);
	}
}

void capi_dump_shared(void)
{
	int i, val, ret;

	capi_dprintf("MapAddress: %p\n", appids);
	capi_dprintf("MapSize:    %zd\n", CAPI20_SHARED_MEM_SIZE);
#ifdef __linux__
	if (capi_sem) {
		ret = sem_getvalue(capi_sem, &val);
	} else {
		ret = 0;
		val = 9999999;
	}
#else
	ret = 0;
	val = 9999999;
#endif
	capi_dprintf("Semaphore: %d (ret=%d)\n", val, ret);
	if (appids) {
		capi_dprintf("Shared memory %s\n", appids->init_done ? "initialized" : "not initialized");
		capi_dprintf("Usage count: %d\n", appids->usage);
		capi_dprintf("Max used Id: %d\n", appids->max_id);
		for (i = 1; i <= appids->max_id; i++) {
			if (i == MAX_APPL)
				break;
			capi_dprintf("AppId:%4d: fd: %d pid: %d %s\n", i, appids->id[i].fd, appids->id[i].pid,
				appids->id[i].inuse ? "(used)" : "(not used)");
		}
	} else {
		capi_dprintf("Shared memory not available\n");
	}
	capi_dprintf("End of dump\n");
}

/**
 * \brief Get byte from data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \return byte from buffer
 */
unsigned short get_byte( unsigned char **ppnPtr ) {
	*ppnPtr += 1;

	return ( ( unsigned char ) *( *ppnPtr - 1 ) );
}

/**
 * \brief Get word from data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \return word from buffer
 */
unsigned short get_word( unsigned char **ppnPtr ) {
	return ( ( get_byte( ppnPtr ) ) | get_byte( ppnPtr ) << 8 );
}

/**
 * \brief Get netword (lo/hi switch) from data buffer and increase
 * \param ppnPtr data buffer pointer
 * \brief buffer pointer
 * \return networdword from buffer
 */
unsigned short get_netword( unsigned char **ppnPtr ) {
	return ( ( get_byte( ppnPtr ) << 8 ) | get_byte( ppnPtr ) );
}

/**
 * \brief Put byte to data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \param nVal byte number
 * \return byte from buffer
 */
unsigned char *put_byte( unsigned char **ppnPtr, _cbyte nVal ) {
	**ppnPtr = nVal;
	*ppnPtr += 1;

	return *ppnPtr;
}

/**
 * \brief Put word to data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \param nVal word number
 * \return word from buffer
 */
unsigned char *put_word( unsigned char **ppnPtr, _cword nVal ) {
	put_byte( ppnPtr, nVal & 0xFF );
	put_byte( ppnPtr, ( nVal & 0xFF00 ) >> 8 );

	return *ppnPtr;
}

/**
 * \brief Put dword to data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \param nVal dword number
 * \return dword from buffer
 */
unsigned char *put_dword( unsigned char **ppnPtr, _cdword nVal ) {
	put_byte( ppnPtr, nVal & 0xFF );
	put_byte( ppnPtr, ( nVal & 0xFF00 ) >> 8 );
	put_byte( ppnPtr, ( nVal & 0xFF0000 ) >> 16 );
	put_byte( ppnPtr, ( nVal & 0xFF000000 ) >> 24 );

	return *ppnPtr;
}

/**
 * \brief Put netword to data buffer and increase buffer pointer
 * \param ppnPtr data buffer pointer
 * \param nVal netword number
 * \return netword from buffer
 */
unsigned char *put_netword( unsigned char **ppnPtr, _cword nVal ) {
	put_byte( ppnPtr, ( nVal & 0xFF00 ) >> 8 );
	put_byte( ppnPtr, nVal & 0xFF );

	return *ppnPtr;
}

static char *skip_whitespace( char *s ) {
	while ( *s && isspace( *s ) ) {
		s++;
	}

	return s;
}

static char *skip_nonwhitespace(char *s)
{
	while (*s && !isspace(*s)) s++;
		return s;
}

/*
 * read config file
 */

static int read_config(void)
{
	FILE *fp = NULL;
	char *s, *t;
	char buf[1024];

	if ((s = getenv("HOME")) != NULL) {
		strcpy(buf, s);
		strcat(buf, "/");
		strcat(buf, userconfigfilename);
		fp = fopen(buf, "r");
	}
	if ((!fp) && ((fp = fopen(globalconfigfilename, "r")) == NULL))
			return(0);

	while(fgets(buf, sizeof(buf), fp)) {
		buf[strlen(buf)-1] = 0;
		s = skip_whitespace(buf);
		if (*s == 0 || *s == '#')
					continue;
		if (!(strncmp(s, "REMOTE", 6))) {
			s = skip_nonwhitespace(s);

			t = skip_whitespace(s);
			s = skip_nonwhitespace(t);
			if (*s) *s++ = 0;
			strncpy(driver, t, (sizeof(driver) - 1));

			t = skip_whitespace(s);
			s = skip_nonwhitespace(t);
			if (*s) *s++ = 0;
			strncpy(hostname, t, (sizeof(hostname) - 1));

			t = skip_whitespace(s);
			s = skip_nonwhitespace(t);
			if (*s) *s++ = 0;
			port = strtol(t, NULL, 10);
			if (!port)
					port = 2662;
			continue;
		} else if (!(strncmp(s, "TRACELEVEL", 10))) {
			t = skip_nonwhitespace(s);
			s = skip_whitespace(t);
			tracelevel = (int)strtol(s, NULL, 10);
			continue;
		} else if (!(strncmp(s, "TRACEFILE", 9))) {
			t = skip_nonwhitespace(s);
			s = skip_whitespace(t);
			t = skip_nonwhitespace(s);
			if (*t) *t++ = 0;
			tracefile = strdup(s);
			continue;
		}
	}
	fclose(fp);
	return(1);
}

int capi20ext_get_port( void ) {
	return port;
}

void capi20ext_set_port( int nPortNumber ) {
	port = nPortNumber;
}

char *capi20ext_get_host( void ) {
	return hostname;
}

void capi20ext_set_host( char *pnHostName ) {
	snprintf( hostname, sizeof( hostname ), "%s", pnHostName );
}

char *capi20ext_get_driver( void ) {
	return driver;
}

void capi20ext_set_driver( char *pnDriver ) {
	snprintf( driver, sizeof( driver ), "%s", pnDriver );
}

int capi20ext_get_tracelevel() {
	return tracelevel;
}

void capi20ext_set_tracelevel(int level) {
	tracelevel = level;
}

/*
 * managment of application ids
 */


int capi_remember_applid(unsigned applid, int fd)
{
	if (applid >= MAX_APPL)
		return -1;
	lock_capi_shared();
	appids->id[applid].fd = fd;
	appids->id[applid].inuse = 1;
	unlock_capi_shared();
	return 0;
}

unsigned capi_alloc_applid(int fd)
{
	unsigned applid;

	lock_capi_shared();
	for (applid = 1; applid < MAX_APPL; applid++) {
		if (appids->id[applid].inuse == 0) {
			appids->id[applid].inuse = 1;
			appids->id[applid].fd = fd;
			appids->id[applid].pid = my_pid;
			if (appids->max_id < applid)
				appids->max_id = applid;
			break;
		}
	}
	if (applid == MAX_APPL)
		applid = 0;
	unlock_capi_shared();
	return applid;
}

void capi_freeapplid(unsigned applid)
{
	if (applid < MAX_APPL) {
		lock_capi_shared();
		appids->id[applid].fd = -1;
		appids->id[applid].inuse = 0;
		appids->id[applid].pid = 0;
		if (appids->max_id == applid) {
			for (applid = appids->max_id; applid > 0; applid--) {
				if (appids->id[applid].inuse)
					break;
			}
			appids->max_id = applid;
		}
		unlock_capi_shared();
	}
}

int capi_validapplid(unsigned applid)
{
	/* no need to lock here */
	return ((applid > 0) && (applid < MAX_APPL) && (appids->id[applid].inuse));
}

int capi_applid2fd(unsigned applid)
{
	/* no need to lock here */
	if (applid < MAX_APPL)
		return appids->id[applid].fd;

	return -1;
}

/*
 * buffer management
 */

struct recvbuffer {
	struct recvbuffer *next;
	unsigned int  datahandle;
	unsigned int  used;
	unsigned int  ncci;
	unsigned char *buf; /* 128 + MaxSizeB3 */
};

struct applinfo {
	unsigned  maxbufs;
	unsigned  nbufs;
	size_t    recvbuffersize;
	struct recvbuffer *buffers;
	struct recvbuffer *firstfree;
	struct recvbuffer *lastfree;
	unsigned char *bufferstart;
};

static struct applinfo *alloc_buffers(
	unsigned MaxB3Connection,
	unsigned MaxB3Blks,
	unsigned MaxSizeB3)
{
	struct applinfo *ap;
	unsigned nbufs = 2 + MaxB3Connection * (MaxB3Blks + 1);
	size_t recvbuffersize = 128 + MaxSizeB3;
	unsigned i;
	size_t size;

	if (recvbuffersize < 2048)
		recvbuffersize = 2048;

	size = sizeof(struct applinfo);
	size += sizeof(struct recvbuffer) * nbufs;
	size += recvbuffersize * nbufs;

	ap = (struct applinfo *)malloc(size);
	if (ap == 0)
		return 0;

	memset(ap, 0, size);
	ap->maxbufs = nbufs;
	ap->recvbuffersize = recvbuffersize;
	ap->buffers = (struct recvbuffer *)(ap+1);
	ap->firstfree = ap->buffers;
	ap->bufferstart = (unsigned char *)(ap->buffers+nbufs);
	for (i = 0; i < ap->maxbufs; i++) {
		ap->buffers[i].next = &ap->buffers[i+1];
		ap->buffers[i].used = 0;
		ap->buffers[i].ncci = 0;
		ap->buffers[i].buf = ap->bufferstart+(recvbuffersize*i);
	}
	ap->lastfree = &ap->buffers[ap->maxbufs-1];
	ap->lastfree->next = 0;
	return ap;
}

static void free_buffers(struct applinfo *ap)
{
	free(ap);
}

static struct applinfo *applinfo[MAX_APPL];

unsigned char *capi_get_buffer(unsigned applid, size_t *sizep, unsigned *handle)
{
	struct applinfo *ap;
	struct recvbuffer *buf;

	assert(capi_validapplid(applid));
	ap = applinfo[applid];
	if ((buf = ap->firstfree) == 0)
		return 0;

	ap->firstfree = buf->next;
	buf->next = 0;
	buf->used = 1;
	ap->nbufs++;
	*sizep = ap->recvbuffersize;
	*handle  = (buf->buf-ap->bufferstart)/ap->recvbuffersize;

	return buf->buf;
}

void capi_save_datahandle(unsigned applid, unsigned offset, unsigned datahandle, unsigned ncci)
{
	struct applinfo *ap;
	struct recvbuffer *buf;

	assert(capi_validapplid(applid));
	ap = applinfo[applid];
	assert(offset < ap->maxbufs);
	buf = ap->buffers+offset;
	buf->datahandle = datahandle;
	buf->ncci = ncci;
}

unsigned capi_return_buffer(unsigned applid, unsigned offset)
{
	struct applinfo *ap;
	struct recvbuffer *buf;

	assert(capi_validapplid(applid));
	ap = applinfo[applid];
	assert(offset < ap->maxbufs);
	buf = ap->buffers+offset;
	assert(buf->used == 1);
	assert(buf->next == 0);

	if (ap->lastfree) {
		ap->lastfree->next = buf;
		ap->lastfree = buf;
	} else {
		ap->firstfree = ap->lastfree = buf;
	}
	buf->used = 0;
	buf->ncci = 0;
	assert(ap->nbufs-- > 0);

	return buf->datahandle;
}

void cleanup_buffers_for_ncci(unsigned applid, unsigned ncci)
{
	struct applinfo *ap;
	unsigned i;

	assert(capi_validapplid(applid));
	ap = applinfo[applid];

	for (i = 0; i < ap->maxbufs; i++) {
		if (ap->buffers[i].used) {
			assert(ap->buffers[i].ncci != 0);
			if (ap->buffers[i].ncci == ncci) {
				capi_return_buffer(applid, i);
			}
		}
	}
}

void cleanup_buffers_for_plci(unsigned applid, unsigned plci)
{
	struct applinfo *ap;
	unsigned i;

	assert(capi_validapplid(applid));
	ap = applinfo[applid];

	for (i = 0; i < ap->maxbufs; i++) {
		if (ap->buffers[i].used) {
			assert(ap->buffers[i].ncci != 0);
			if ((ap->buffers[i].ncci & 0xffff) == plci) {
				capi_return_buffer(applid, i);
			}
		}
	}
}

/*
 * CAPI2.0 functions
 */

/** Simple single-linked list for modules */
struct sList {
	struct sModule *psMod;
	struct sList *psNext;
};

/* Active module pointer */
static struct sModule *psModule = NULL;
/* Global loaded module list */
static struct sList *psModuleList = NULL;

/**
 * \brief Register Module - add it to psModuleList
 * \param psMod new module pointer
 * \return error code. 0 on success, else error occurred
 */
static int RegisterModule( struct sModule *psMod ) {
	struct sList *psList;

	if ( psMod == NULL || psMod -> pnName == NULL || psMod -> nVersion != MODULE_LOADER_VERSION || psMod -> psOperations == NULL ) {
		return -1;
	}

	if ( psModuleList == NULL ) {
		psModuleList = malloc( sizeof( struct sList ) );
		if ( psModuleList != NULL ) {
			psModuleList -> psMod = psMod;
			psModuleList -> psNext = NULL;
		}

		return psModuleList != NULL ? 0 : -1;
	}

	psList = psModuleList;
	while ( psList -> psNext ) {
		psList = psList -> psNext;
	}

	psList -> psNext = malloc( sizeof( struct sList ) );
	if ( psList -> psNext != NULL ) {
		psList -> psNext -> psMod = psMod;
		psList -> psNext -> psNext = NULL;
		return 0;
	}

	return -2;
}

/**
 * \brief Try to load the module and register it
 * \param pnName full path name to module
 * \return error code, see RegisterModule
 */
static int LoadCapiModule( char *pnName ) {
	struct sModule *psMod;
	void *pHandle;
	typedef int ( *InitModule )( struct sModule *psModule );
	InitModule initModule;

	/* Try to open module */
	pHandle = dlopen( pnName, RTLD_GLOBAL | RTLD_LAZY );
	if ( pHandle == NULL ) {
		char etxt[1024];
		snprintf(etxt, 1024, "Could not open module %s - %s!\n", pnName, dlerror());
		CapiDebug(1, etxt);
		return -1;
	}

	/* Find InitModule function within module */
	initModule = dlsym( pHandle, "InitModule" );
	if ( initModule == NULL ) {
		CapiDebug( 1, "Module has no InitModule!\n" );
		dlclose( pHandle );
		return -2;
	}

	/* Create new module structure */
	psMod = malloc( sizeof( struct sModule ) );
	if ( psMod == NULL ) {
		CapiDebug( 1, "Could not alloc memory for module structure!\n" );
		dlclose( pHandle );
		return -2;
	}

	/* Initialize module */
	if ( initModule( psMod ) != 0 ) {
		CapiDebug( 1, "Could not init!\n" );
		free( psMod );
		psMod = NULL;
		dlclose( pHandle );
		return -3;
	}

	/* Success, add handle to module structure */
	psMod -> pHandle = pHandle;

	/* register module */
	return RegisterModule( psMod );
}

/**
 * \brief Initialize module
 * \param pnModuleDir directory where we can find the modules
 */
static void InitModules( char *pnModuleDir ) {
	DIR *psDir;
	struct dirent *psEntry;
	char *pnFullName;
	int nLen, pf_len;
	char mod_vers[10];

	if ( psModuleList != NULL ) {
		CapiDebug( 1, "Already initialized\n" );
		return;
	}

	/* try to open module directory */
#ifdef WIN32
	char anAppPath[ MAX_PATH ];
	char anPath[ MAX_PATH ];
	int nIndex;

	memset( anAppPath, 0, sizeof( anAppPath ) );
	GetModuleFileName( NULL, anAppPath, sizeof( anAppPath ) );
	for ( nIndex = strlen( anAppPath ) - 1; nIndex >= 0; nIndex-- ) {
		if ( anAppPath[ nIndex ] == '\\' ) {
			anAppPath[ nIndex ] = '\0';
			break;
		}
	}
	snprintf( anPath, sizeof( anPath ), "%s/../%s", anAppPath, pnModuleDir );
	pnModuleDir = anPath;
	printf("path: %s\n", pnModuleDir );
	psDir = opendir( pnModuleDir );
#else
	psDir = opendir( pnModuleDir );
#endif

	if ( psDir != NULL ) {
		/* read entry by entry */
#if __APPLE__ && __MACH__
		pf_len = snprintf(mod_vers, 10, "%d.dylib", MODULE_LOADER_VERSION);
#elif __WIN32__
		pf_len = snprintf(mod_vers, 10, ".dll");
#else
		pf_len = snprintf(mod_vers, 10, ".so.%d", MODULE_LOADER_VERSION);
#endif
		while ( ( psEntry = readdir( psDir ) ) != NULL )  {
			/* skip ".", ".." and files which do not end with module suffix */
			nLen = strlen( psEntry -> d_name );

			switch ( nLen ) {
				case 1:
					if ( psEntry -> d_name[ 0 ] == '.' ) {
						continue;
					}
					break;
				case 2:
					if ( psEntry -> d_name[ 0 ] == '.' && psEntry -> d_name[ 1 ] == '.' ) {
						continue;
					}
					break;
				default:
					if ( strcmp( psEntry -> d_name + nLen - pf_len, mod_vers) ) {
						continue;
					}
					break;
			}

			/* len of complete path + filename */
			nLen = strlen( pnModuleDir ) + strlen( psEntry -> d_name ) + 2;
			pnFullName = malloc( nLen );
			if ( pnFullName != NULL ) {
				/* create full name */
				snprintf( pnFullName, nLen, "%s/%s", pnModuleDir, psEntry -> d_name );
				/* load module */
				LoadCapiModule( pnFullName );

				/* free full name */
				free( pnFullName );
				pnFullName = NULL;
			}
		}

		/* close directory */
		closedir( psDir );
	}
}

unsigned capi20_isinstalled( void ) {
	struct sList *psList;

	/* check if we are already opened */
	if ( capi_fd >= 0 ) {
		return CapiNoError;
	}

	/* Load and initialize modules */
#ifdef __WIN32__
	InitModules( "lib/capi/" );
#else
	InitModules( LIBDIR );
#endif

	if ( psModuleList == NULL ) {
		/* if no modules are loaded, psModuleList is NULL, abort */
		return CapiRegNotInstalled;
	}

	/* read configuration file */
	read_config();

	/* no special driver requested, auto-detect */
	if ( strlen( driver ) <= 0 ) {
		/* backwards-compatible: check standard interface fist */
		psList = psModuleList;
		while ( psList != NULL ) {

			CapiDebug( 1, "[%s]: standard loop - module: %s\n", __FUNCTION__, psList -> psMod -> pnName );
			if ( !strcasecmp( psList -> psMod -> pnName, "standard" ) ) {
				psModule = psList -> psMod;
				capi_fd = psModule -> psOperations -> IsInstalled();
				CapiDebug( 1, "[%s]: capi_fd: %d\n", __FUNCTION__, capi_fd );
				if ( capi_fd >= 0 ) {
					/* no error */
					return CapiNoError;
				}
			}
			psList = psList -> psNext;
		}

		/* no standard device detect, try the other modules */
		psList = psModuleList;
		while ( psList != NULL ) {
			CapiDebug( 1, "[%s]: other loop - module: %s\n", __FUNCTION__, psList -> psMod -> pnName );
			if ( strcasecmp( psList -> psMod -> pnName, "standard" ) ) {
				psModule = psList -> psMod;
				capi_fd = psModule -> psOperations -> IsInstalled();
				CapiDebug( 1, "[%s]: capi_fd: %d\n", __FUNCTION__, capi_fd );
				if ( capi_fd >= 0 ) {
					/* no error */
					return CapiNoError;
				}
			}
			psList = psList -> psNext;
		}
	} else {
		/* Find requested driver in list */
		psList = psModuleList;
		while ( psList != NULL ) {
			if ( !strcasecmp( psList -> psMod -> pnName, driver ) ) {
				psModule = psList -> psMod;
				capi_fd = psModule -> psOperations -> IsInstalled();
				if ( capi_fd >= 0 ) {
					/* no error */
					return CapiNoError;
				}
				break;
			}
			psList = psList -> psNext;
		}
	}

	CapiDebug( 1, "[%s]: CapiRegNotInstalled\n", __FUNCTION__ );
	/* uhh, not installed */
	return CapiRegNotInstalled;
}

unsigned capi20_register( unsigned MaxB3Connection, unsigned MaxB3Blks, unsigned MaxSizeB3, unsigned *ApplID ) {
	unsigned int applid = 0;
	int fd = -1;

	*ApplID = 0;

	if (capi20_isinstalled() != CapiNoError)
		return CapiRegNotInstalled;

	fd = psModule -> psOperations -> Register( MaxB3Connection, MaxB3Blks, MaxSizeB3, &applid );
	if ( fd < 0 ) {
		return CapiRegOSResourceErr;
	}

	if (capi_remember_applid(applid, fd) < 0) {
		close(fd);
		return CapiRegOSResourceErr;
	}
	applinfo[applid] = alloc_buffers(MaxB3Connection, MaxB3Blks, MaxSizeB3);
	if (applinfo[applid] == 0) {
		close(fd);
		return CapiRegOSResourceErr;
	}
	*ApplID = applid;
	return CapiNoError;
}

unsigned
capi20_release (unsigned ApplID)
{
	int fd;

	if (capi20_isinstalled() != CapiNoError)
		return CapiRegNotInstalled;

	if (!capi_validapplid(ApplID))
		return CapiIllAppNr;

	if (psModule -> psOperations -> Release != NULL) {
		psModule -> psOperations -> Release(capi_applid2fd(ApplID), ApplID);
	}
	fd = capi_applid2fd(ApplID);
	/* maybe closed by the lower level */
	if (fd > -1)
		(void)close(fd);
	capi_freeapplid(ApplID);
	free_buffers(applinfo[ApplID]);
	applinfo[ApplID] = NULL;

	if ( capi_fd >= 0 ) {
		close( capi_fd );
		capi_fd = -1;
	}

	return CapiNoError;
}

/**
 * \brief Process data message (should be moved up to capi20.c as it is general)
 * \param pnMsg message data pointer
 * \param nApplId application id
 * \param nCommand major packet command
 * \param nSubCommand minor packet command
 * \param nLen len of message
 * \return length of full packet
 */
int capi_processMessage( unsigned char *pnMsg, unsigned nApplId, unsigned nCommand, unsigned nSubCommand, int nLen ) {
	/* DATA_B3_REQ specific:
	 * we have to copy  the buffer and patch the buffer address!
	 */
	if ( nCommand == CAPI_DATA_B3 ) {
		if ( nSubCommand == CAPI_REQ ) {
			int nDataLen = CAPIMSG_DATALEN( pnMsg );
			void *pDataPtr;

#if SIZEOF_VOID_P > 4
			if ( nLen >= 30 ) {
				/* 64Bit CAPI-extension */
				u_int64_t nData64;

				memcpy( &nData64, pnMsg + 22, sizeof( u_int64_t ) );
				if ( nData64 != 0 ) {
					pDataPtr = ( void * )( unsigned long ) qword2ptr( nData64 );
				} else {
					/* Assume data after message */
					pDataPtr = pnMsg + nLen;
				}
			} else {
				/* Assume data after message */
				pDataPtr = pnMsg + nLen;
			}
#else
			u_int32_t nData;

			memcpy( &nData, pnMsg + 12, sizeof( u_int32_t ) );
			if ( nData != 0 ) {
				pDataPtr = ( void * )( ( unsigned long ) dword2ptr( nData ) );
			} else {
				pDataPtr = pnMsg + nLen;
			}
#endif

			if ( nLen + nDataLen > SEND_BUFSIZ ) {
				return CapiMsgOSResourceErr;
			}

			memcpy( pnMsg + nLen, pDataPtr, nDataLen );
			nLen += nDataLen;
		} else if ( nSubCommand == CAPI_RESP ) {
			capimsg_setu16( pnMsg, 12, capi_return_buffer( nApplId, CAPIMSG_U16( pnMsg, 12 ) ) );
		}
	}

	if ( nCommand == CAPI_DISCONNECT_B3 && nSubCommand == CAPI_RESP ) {
		cleanup_buffers_for_ncci( nApplId, CAPIMSG_U32( pnMsg, 8 ) );
	}

	return nLen;
}

unsigned capi20_put_message( unsigned ApplID, unsigned char *Msg ) {
	if (capi20_isinstalled() != CapiNoError) {
		return CapiRegNotInstalled;
	}

	if (!capi_validapplid(ApplID)) {
		return CapiIllAppNr;
	}

	return psModule->psOperations->PutMessage(capi_applid2fd(ApplID), ApplID, Msg);
}

unsigned capi20_get_message( unsigned ApplID, unsigned char **Buf ) {
	if (capi20_isinstalled() != CapiNoError)
		return CapiRegNotInstalled;

	if (!capi_validapplid(ApplID))
		return CapiIllAppNr;

	return psModule->psOperations->GetMessage(capi_applid2fd(ApplID), ApplID, Buf);
}

unsigned char *capi20_get_manufacturer( unsigned Ctrl, unsigned char *Buf ) {
	if (capi20_isinstalled() != CapiNoError)
		return 0;

	return psModule -> psOperations -> GetManufactor( capi_fd, Ctrl, Buf );
}

unsigned char *
capi20_get_version(unsigned Ctrl, unsigned char *Buf)
{
	if (capi20_isinstalled() != CapiNoError)
		return 0;

	return psModule -> psOperations -> GetVersion( capi_fd, Ctrl, Buf );
}

unsigned char *
capi20_get_serial_number(unsigned Ctrl, unsigned char *Buf)
{
	if (capi20_isinstalled() != CapiNoError)
		return 0;

	return psModule -> psOperations -> GetSerialNumber( capi_fd, Ctrl, Buf );
}

unsigned
capi20_get_profile(unsigned Ctrl, unsigned char *Buf)
{
	if (capi20_isinstalled() != CapiNoError)
		return CapiMsgNotInstalled;

	return psModule -> psOperations -> GetProfile( capi_fd, Ctrl, Buf );
}
/*
 * functions added to the CAPI2.0 spec
 */

unsigned
capi20_waitformessage(unsigned ApplID, struct timeval *TimeOut)
{
	int fd;
	fd_set rfds;

	if (capi20_isinstalled() != CapiNoError)
		return CapiRegNotInstalled;

	if (!capi_validapplid(ApplID))
		return CapiIllAppNr;

	/* default method */
	fd = capi_applid2fd(ApplID);

	if (psModule->psOperations->waitformessage)
		return psModule->psOperations->waitformessage(fd, ApplID, TimeOut);

	FD_ZERO(&rfds);
	FD_SET(fd, &rfds);

	if (select(fd + 1, &rfds, NULL, NULL, TimeOut) < 1)
		return CapiReceiveQueueEmpty;

	return CapiNoError;
}

int
capi20_fileno(unsigned ApplID)
{
	return capi_applid2fd(ApplID);
}

/*
 * Extensions for middleware
 */

int
capi20ext_get_flags(unsigned ApplID, unsigned *flagsptr)
{
	if ( psModule && psModule -> psOperations -> GetFlags ) {
		return psModule -> psOperations -> GetFlags( ApplID, flagsptr );
	}

	return CapiMsgOSResourceErr;
}

int
capi20ext_set_flags(unsigned ApplID, unsigned flags)
{
	if ( psModule && psModule -> psOperations -> SetFlags ) {
		return psModule -> psOperations -> SetFlags( ApplID, flags );
	}

	return CapiMsgOSResourceErr;
}

int
capi20ext_clr_flags(unsigned ApplID, unsigned flags)
{
	if ( psModule && psModule -> psOperations -> ClearFlags ) {
		return psModule -> psOperations -> ClearFlags( ApplID, flags );
	}

	return CapiMsgOSResourceErr;
}

char *
capi20ext_get_tty_devname(unsigned applid, unsigned ncci, char *buf, size_t size)
{
	if ( psModule && psModule -> psOperations -> GetTtyDeviceName ) {
		return psModule -> psOperations -> GetTtyDeviceName( applid, ncci, buf, size );
	}

	return NULL;
}

char *capi20ext_get_raw_devname( unsigned applid, unsigned ncci, char *buf, size_t size ) {
	if ( psModule && psModule -> psOperations -> GetRawDeviceName ) {
		return psModule -> psOperations -> GetRawDeviceName( applid, ncci, buf, size );
	}

	return NULL;
}

int capi20ext_ncci_opencount( unsigned applid, unsigned ncci ) {
	if ( psModule && psModule -> psOperations -> GetNcciOpenCount ) {
		return psModule -> psOperations -> GetNcciOpenCount( applid, ncci );
	}

	return CapiMsgOSResourceErr;
}

static void initlib( void ) __attribute__( ( constructor ) );
static void exitlib( void ) __attribute__( ( destructor ) );

static void initlib( void ) {
	int i;

#ifdef __linux__
	int ret;
	mode_t old_um;
	struct stat stats;
	int need_init = 0;
	char sem_name[40], shr_name[40];

	if (!capi_sem) {
		snprintf(sem_name, 32, "%s.v%08x", CAPI20_SEMAPHORE_NAME, CAPI20_SHARED_MEM_VERSION);
		snprintf(shr_name, 32, "%s.v%08x", CAPI20_SHARED_MEM_NAME, CAPI20_SHARED_MEM_VERSION);
		old_um = umask(0);
		capi_sem = sem_open(sem_name, O_CREAT, CAPI20_SHARED_MEM_MODE, 1);
		shm_mem_fd = shm_open(shr_name, O_CREAT | O_RDWR, CAPI20_SHARED_MEM_MODE);
		umask(old_um);
		if (capi_sem == SEM_FAILED) {
			fprintf(stderr, "sem_open(%s, ...) failed - %s\n", sem_name, strerror(errno));
			exit(1);
		}
		if (shm_mem_fd < 0) {
			fprintf(stderr, "shm_open(%s, ...) failed - %s\n", shr_name, strerror(errno));
			exit(1);
		}

		lock_capi_shared();

		ret = fstat(shm_mem_fd, &stats);
		if (ret) {
			fprintf(stderr, "fstat(shm_mem_fd, &stats) failed - %s\n", strerror(errno));
			exit(1);
		}
		if (stats.st_size == 0) {
			/* We are the first user - so set size and init it */
			ret = ftruncate(shm_mem_fd, CAPI20_SHARED_MEM_SIZE);
			if (ret) {
				fprintf(stderr, "ftruncate(shm_mem_fd, %zd) failed - %s\n", CAPI20_SHARED_MEM_SIZE, strerror(errno));
				exit(1);
			}
			need_init = 1;
		}
		appids = mmap(NULL, CAPI20_SHARED_MEM_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, shm_mem_fd, 0);
		if (appids ==  MAP_FAILED) {
			fprintf(stderr, "mmap shm_mem_fd (%d) failed - %s\n", shm_mem_fd, strerror(errno));
			exit(1);
		}
		if (need_init) {
			for ( i = 0; i < MAX_APPL; i++ ) {
				appids->id[i].fd = -1;
			}
			appids->init_done = 1;
		}
	} else
		lock_capi_shared();
	appids->usage++;
	my_pid = getpid();
	unlock_capi_shared();

#else
	appids = malloc( sizeof( struct shr_applids ) );
	for ( i = 0; i < MAX_APPL; i++ ) {
		appids->id[i].fd = -1;
	}
#endif
}

static void exitlib( void ) {
	int i, last_inuse = 0;

	if ( capi_fd >= 0 ) {
		close( capi_fd );
		capi_fd = -1;
	}
	lock_capi_shared();
	appids->usage--;
	for (i = 0; i < MAX_APPL; i++ ) {
		if (appids->id[i].pid == my_pid) {
			appids->id[i].pid = 0;
			appids->id[i].inuse = 0;
			appids->id[i].fd = -1;
		} else if (appids->id[i].inuse)
			last_inuse = i;
	}
	appids->max_id = last_inuse;
#ifdef __linux__
	munmap(appids, CAPI20_SHARED_MEM_SIZE);
#endif
	unlock_capi_shared();
}
