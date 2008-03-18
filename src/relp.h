/* The RELP (reliable event logging protocol) core protocol library.
 *
 * Copyright 2008 by Rainer Gerhards and Adiscon GmbH.
 *
 * This file is part of librelp.
 *
 * Librelp is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Librelp is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with librelp.  If not, see <http://www.gnu.org/licenses/>.
 *
 * A copy of the GPL can be found in the file "COPYING" in this distribution.
 *
 * If the terms of the GPL are unsuitable for your needs, you may obtain
 * a commercial license from Adiscon. Contact sales@adiscon.com for further
 * details.
 *
 * ALL CONTRIBUTORS PLEASE NOTE that by sending contributions, you assign
 * your copyright to Adiscon GmbH, Germany. This is necessary to permit the
 * dual-licensing set forth here. Our apologies for this inconvenience, but
 * we sincerely believe that the dual-licensing model helps us provide great
 * free software while at the same time obtaining some funding for further
 * development.
 */
#ifndef RELP_H_INCLUDED
#define	RELP_H_INCLUDED

#include <pthread.h>


/* define some of our types that a caller must know about */
typedef unsigned char relpOctet_t;
typedef int relpTxnr_t;
typedef int relpRetVal;


/* IDs of librelp objects */
typedef enum relpObjID_e {
	eRelpObj_Invalid = 0,	/**< invalid object, must be zero to detect unitilized value */
	eRelpObj_Engine = 1,
	eRelpObj_Sess = 2,
	eRelpObj_Frame = 3,
	eRelpObj_Clt = 4,
	eRelpObj_Srv = 5,
	eRelpObj_Sendq = 6,
	eRelpObj_Sendqe = 7,
	eRelpObj_Sendbuf = 8,
	eRelpObj_Tcp = 9
} relpObjID_t;


/* the following macro MUST be specified as the first member of each
 * RELP object.
 */
#define BEGIN_RELP_OBJ relpObjID_t objID

/* the core intializer to call on top of each constructure (right after the calloc) */
#define RELP_CORE_CONSTRUCTOR(pObj, objType) \
	(pObj)->objID = eRelpObj_##objType


/* a linked list entry for the list of relp servers (of this engine) */
typedef struct relpEngSrvLst_s {
	struct relpEngSrvLst_s *pPrev;
	struct relpEngSrvLst_s *pNext;
	struct relpSrv_s *pSrv;
} relpEngSrvLst_t;


/* a linked list entry for the list of relp sessions (of this engine) */
typedef struct relpEngSessLst_s {
	struct relpEngSessLst_s *pPrev;
	struct relpEngSessLst_s *pNext;
	struct relpSess_s *pSess;
} relpEngSessLst_t;


/* the RELP engine object 
 * Having a specific engine object enables multiple plugins to call the
 * RELP engine at the same time. The core idea of librelp is to have no
 * static data at all and everything stored in RELP engine objects. Every
 * program/plugin should aquire a RELP engine once on startup (or first need)
 * and release it only when it is fully done with RELP, which usually means
 * on shutdown.
 * rgerhards, 2008-03-16
 */
typedef struct relpEngine_s {
	BEGIN_RELP_OBJ;
	void (*dbgprint)(char *fmt, ...) __attribute__((format(printf, 1, 2)));

	/* linked list of our servers */
	relpEngSrvLst_t *pSrvLstRoot;
	relpEngSrvLst_t *pSrvLstLast;
	int lenSrvLst;
	pthread_mutex_t mutSrvLst;

	/* linked list of our sessions */
	relpEngSessLst_t *pSessLstRoot;
	relpEngSessLst_t *pSessLstLast;
	int lenSessLst;
	pthread_mutex_t mutSessLst;
} relpEngine_t;


/* macro to assert we are dealing with the right relp object */
#ifdef NDEBUG
#	define RELPOBJ_assert(obj, type)
#else /* debug case */
#	define RELPOBJ_assert(pObj, type) \
		assert((pObj) != NULL); \
		assert((pObj)->objID == eRelpObj_##type)

#endif /* # ifdef NDEBUG */


/* some defines that may also come from the config */
#ifndef RELP_DFLT_PORT
#	define RELP_DFLT_PORT 20514
#endif
#ifndef RELP_DFLT_MAX_DATA_SIZE
#	define RELP_DFLT_MAX_DATA_SIZE 128 * 1024 /* 128K should be sufficient for everything... */
#endif

/* set the default receive buffer size if none is externally configured
 * NOTE: do not define to less than 1.5K or you'll probably see a severe
 * performance hit!
 */
#ifndef	RELP_RCV_BUF_SIZE
#	define RELP_RCV_BUF_SIZE 32 * 1024 /* 32K */
#endif

/* now define our externally-visible error codes */
#ifndef ERRCODE_BASE
	/* provide a basis for error numbers if not configured */
#	define ERRCODE_BASE 10000
#endif

/* we may argue if RELP_RET_OK should also be relative to ERRCODE_BASE. I have deciced against it,
 * because if it is 0, we can use it together with other project's iRet mechanisms, which is quite
 * useful. -- rgerhards, 2008-03-17
 */
#define RELP_RET_OK		0			/**< everything went well, no error */
#define RELP_RET_OUT_OF_MEMORY	ERRCODE_BASE + 1	/**< out of memory occured */
#define RELP_RET_INVALID_FRAME	ERRCODE_BASE + 2	/**< relp frame received is invalid */
#define RELP_RET_PARAM_ERROR	ERRCODE_BASE + 3	/**< an (API) calling parameer is in error */
#define RELP_RET_INVALID_PORT	ERRCODE_BASE + 4	/**< invalid port value */
#define RELP_RET_COULD_NOT_BIND	ERRCODE_BASE + 5	/**< could not bind socket, defunct */
#define RELP_RET_ACCEPT_ERR	ERRCODE_BASE + 6	/**< error during accept() system call */
#define RELP_RET_SESSION_BROKEN	ERRCODE_BASE + 7	/**< the RELP session is broken */
#define RELP_RET_SESSION_CLOSED	ERRCODE_BASE + 8	/**< the RELP session was closed (not an error) */
#define RELP_RET_INVALID_CMD	ERRCODE_BASE + 9	/**< the command contained in a RELP frame was unknown */
#define RELP_RET_DATA_TOO_LONG	ERRCODE_BASE + 10	/**< DATALEN exceeds permitted length */

/* some macros to work with librelp error codes */
#define CHKRet(code) if((iRet = code) != RELP_RET_OK) goto finalize_it
/* macro below is to be used if we need our own handling, eg for cleanup */
#define CHKRet_Hdlr(code) if((iRet = code) != RELP_RET_OK)
/* macro below is used in conjunction with CHKiRet_Hdlr, else use ABORT_FINALIZE */
#define FINALIZE goto finalize_it;
#define ENTER_RELPFUNC relpRetVal iRet = RELP_RET_OK
#define LEAVE_RELPFUNC return iRet
#define ABORT_FINALIZE(errCode)			\
	do {					\
		iRet = errCode;			\
		goto finalize_it;		\
	} while (0)


/* prototypes needed by library users */
relpRetVal relpEngineConstruct(relpEngine_t **ppThis);
relpRetVal relpEngineDestruct(relpEngine_t **ppThis);
relpRetVal relpEngineSetDbgprint(relpEngine_t *pThis, void (*dbgprint)(char *fmt, ...) __attribute__((format(printf, 1, 2))));
relpRetVal relpEngineAddListner(relpEngine_t *pThis, unsigned char *pLstnPort);
relpRetVal relpEngineRun(relpEngine_t *pThis);

#ifdef RELP_DO_INTERNAL_PROTOTYPES
#	include "relpframe.h"
	relpRetVal relpEngineDispatchFrame(relpEngine_t *pThis, relpSess_t *pSess, relpFrame_t *pFrame);
#endif /* #ifdef RELP_DO_INTERNAL_PROTOTYPES */

#endif /* #ifndef RELP_H_INCLUDED */