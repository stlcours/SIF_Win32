/* 
   Copyright 2013 KLab Inc.

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/
#include "CKLBNetAPI.h"
#include "CKLBLuaEnv.h"
#include "CKLBUtility.h"
#include "CKLBJsonItem.h"
#include "CPFInterface.h"
#include "CKLBNetAPIKeyChain.h"

#include <time.h>
#include <ctype.h>

;
enum {
	// Command Values定義
	NETAPI_STARTUP,
	NETAPI_LOGIN,
	NETAPI_LOGOUT,
	NETAPI_SEND,				// send JSON packet
	NETAPI_CANCEL,				// selected session cancel.
	NETAPI_CANCEL_ALL,
	NETAPI_WATCH_MAINTENANCE,
	NETAPI_DEBUG_HDR,
	NETAPI_GEN_CMDNUMID,
};

static IFactory::DEFCMD cmd[] = {
	{"NETAPI_STARTUP",					NETAPI_STARTUP					},
	{"NETAPI_LOGIN",					NETAPI_LOGIN					},
	{"NETAPI_LOGOUT",					NETAPI_LOGOUT					},
	{"NETAPI_SEND",						NETAPI_SEND						},
    {"NETAPI_CANCEL",					NETAPI_CANCEL					},
	{"NETAPI_CANCEL_ALL",				NETAPI_CANCEL_ALL				},
	{"NETAPI_WATCH_MAINTENANCE",		NETAPI_WATCH_MAINTENANCE		},
	{"NETAPI_DEBUG_HDR",				NETAPI_DEBUG_HDR				},
	{"NETAPI_GEN_CMDNUMID",				NETAPI_GEN_CMDNUMID				},

	//
	// Callback constants
	//
	{"NETAPIMSG_CONNECTION_CANCELED",	NETAPIMSG_CONNECTION_CANCELED	},
	{"NETAPIMSG_CONNECTION_FAILED",		NETAPIMSG_CONNECTION_FAILED		},
	{"NETAPIMSG_INVITE_FAILED",			NETAPIMSG_INVITE_FAILED			},
	{"NETAPIMSG_STARTUP_FAILED",		NETAPIMSG_STARTUP_FAILED		},
	{"NETAPIMSG_SERVER_TIMEOUT",		NETAPIMSG_SERVER_TIMEOUT		},
	{"NETAPIMSG_REQUEST_FAILED",		NETAPIMSG_REQUEST_FAILED		},
	{"NETAPIMSG_LOGIN_FAILED",			NETAPIMSG_LOGIN_FAILED			},
	{"NETAPIMSG_SERVER_ERROR",			NETAPIMSG_SERVER_ERROR			},
	{"NETAPIMSG_UNKNOWN",				NETAPIMSG_UNKNOWN				},
	{"NETAPIMSG_LOGIN_SUCCESS",			NETAPIMSG_LOGIN_SUCCESS			},
	{"NETAPIMSG_REQUEST_SUCCESS",		NETAPIMSG_REQUEST_SUCCESS		},
	{"NETAPIMSG_STARTUP_SUCCESS",		NETAPIMSG_STARTUP_SUCCESS		},
	{"NETAPIMSG_INVITE_SUCCESS",		NETAPIMSG_INVITE_SUCCESS		},
	{0, 0}
};

static CKLBTaskFactory<CKLBNetAPI> factory("HTTP_API", CLS_KLBNETAPI, cmd);

enum {
	ARG_CALLBACK	= 5,
	ARG_REQUIRE     = ARG_CALLBACK,
};


CKLBNetAPI::CKLBNetAPI()
: CKLBLuaTask           ()
, m_http				(NULL)
, m_timeout				(30000)
, m_timestart			(0)
, m_canceled			(false)
, m_pRoot				(NULL)
, m_callback			(NULL)
, m_http_header_array	(NULL)
, m_http_header_length	(0)
{
}

CKLBNetAPI::~CKLBNetAPI() 
{
	// Done in Die()
}

u32 
CKLBNetAPI::getClassID() 
{
	return CLS_KLBNETAPI;
}

void
CKLBNetAPI::execute(u32 deltaT)
{
	if (!m_http) {
		return; // Do nothing if no active connection
	}

	m_timestart += deltaT;

	// Check cancel first
	if (m_canceled) {
		lua_callback(NETAPIMSG_CONNECTION_CANCELED, -1, NULL);

		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		// Reset flag
		m_canceled = false;
		return;
	}

	// Received data second
	if (m_http->httpRECV() || (m_http->getHttpState() != -1)) {
		// Get Data
		u8* body	= m_http->getRecvResource();
		u32 bodyLen	= body ? m_http->getSize() : 0;
		
		// Get Status Code
		int state = m_http->getHttpState();
		bool invalid = ((state >= 500) && (state <= 599)) || (state == 204);
		int msg = invalid ? NETAPIMSG_REQUEST_SUCCESS : NETAPIMSG_SERVER_ERROR;

		//
		// Support only JSon for callback
		// 
		freeJSonResult();
		m_pRoot = getJsonTree((const char*)body, bodyLen);
		lua_callback(msg, state, m_pRoot);

		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		return;
	}

	if ((m_http->m_threadStop == 1) && (m_http->getHttpState() == -1)) {
		lua_callback(NETAPIMSG_CONNECTION_FAILED, -1, NULL);
		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
	}

	// Time out third (after check that valid has arrived)
	if (m_timestart >= m_timeout) {
		lua_callback(NETAPIMSG_SERVER_TIMEOUT, -1, NULL);
		NetworkManager::releaseConnection(m_http);
		m_http = NULL;
		return;
	}
}

void
CKLBNetAPI::die()
{
	if (m_http) {
		NetworkManager::releaseConnection(m_http);
	}
	KLBDELETEA(m_callback);
	freeHeader();
	freeJSonResult();
}

void
CKLBNetAPI::freeJSonResult() {
	KLBDELETE(m_pRoot);
}

void
CKLBNetAPI::freeHeader() {
	if (m_http_header_array) {
		for (u32 n=0; n < m_http_header_length; n++) {
			KLBDELETEA(m_http_header_array[n]);
		}
		KLBDELETEA(m_http_header_array);
		m_http_header_array = NULL;
	}
}

CKLBNetAPI* 
CKLBNetAPI::create( CKLBTask* pParentTask, 
                    const char * callback) 
{
	CKLBNetAPI* pTask = KLBNEW(CKLBNetAPI);
    if(!pTask) { return NULL; }

	if(!pTask->init(pParentTask, callback)) {
		KLBDELETE(pTask);
		return NULL;
	}
	return pTask;
}

bool 
CKLBNetAPI::init(	CKLBTask* pTask,
					const char * callback) 
{
	m_callback = (callback) ? CKLBUtility::copyString(callback) : NULL;

	// 一通り初期化値が作れたのでタスクを登録
	bool res = regist(pTask, P_INPUT);
	return res;
}

bool
CKLBNetAPI::initScript(CLuaState& lua)
{
	int argc = lua.numArgs();
	lua.print_stack();

    if (argc < ARG_REQUIRE) { return false; }

	CKLBNetAPIKeyChain& kc=CKLBNetAPIKeyChain::getInstance();
	kc.setAppID(lua.getString(4));
	kc.setClient(lua.getString(3));
	kc.setURL(lua.getString(1));
	kc.setRegion(lua.getString(7));
	kc.setConsumernKey(lua.getString(2));

	return init(NULL, lua.getString(ARG_CALLBACK));
}

CKLBJsonItem *
CKLBNetAPI::getJsonTree(const char * json_string, u32 dataLen)
{
	CKLBJsonItem * pRoot = CKLBJsonItem::ReadJsonData((const char *)json_string, dataLen);

	return pRoot;
}

bool CKLBNetAPI::startUp(const char* user_id,const char* password,const char* uiv,unsigned int timeout,unsigned int* uiv2) {
	CKLBNetAPIKeyChain& kc=CKLBNetAPIKeyChain::getInstance();
	kc.setUserID(user_id);
	kc.setConsumernKey(password);
	return true;
}

int
CKLBNetAPI::commandScript(CLuaState& lua)
{
	int argc = lua.numArgs();

	if(argc < 2) {
		lua.retBoolean(false);
		return 1;
	}
	lua.print_stack();
	int cmd = lua.getInt(2);
	int ret = 1;

	switch(cmd)
	{
	default:
		{
			lua.retBoolean(false);
		}
		break;
	case NETAPI_STARTUP:
		{
			if(argc-4>=3) lua.retBoolean(false);
			else {
				const char* user_id=lua.getString(3);
				const char* password=lua.getString(4);
				const char* uiv=NULL;
				int timeout=0;
				unsigned int uiv2=0;

				if(lua.getType(5)!=LUA_TNIL && argc>=5) uiv=lua.getString(5);
				if(argc>=6) timeout=lua.getInt(6);

				lua.retInt(1);
				//startUp(user_id,password,uiv,timeout,&uiv2);
			}
		}
		break;
	case NETAPI_CANCEL:
		{
			if (m_http != NULL) {
				m_canceled = true;
			}
			lua.retBoolean(m_http != NULL);
		}
		break;
	/*case NETAPI_BUSY:
		{
			// If object is alive, then it is busy.
			lua.retBoolean(m_http != NULL);
		}
		break;*/
	case NETAPI_SEND:
		{
			//
			// 3. URL
			// 4. Header table
			// 5. POST optionnal
			// 6. Timeout
			//
			if(argc < 3 || argc > 6) {
				lua.retBoolean(false);
			}
			else
			if (m_http) {
				// Connection is still busy,you can not send
				lua.retBoolean(false);
			}
			else {
				const char * api = NULL;
				if(!lua.isNil(3)) api = lua.getString(3);

				// Header list
				const char** headers = NULL;
				const char** values  = NULL;
				freeHeader();

				if(!lua.isNil(4)) {
					// Get the asset list
					lua.retValue(4);

					// Count the number of elements
					int max = 0;
					lua.retNil();

					// Read indexes and count entries.
					while(lua.tableNext()) {
						lua.retValue(-2);
						max++;
						lua.pop(2);
					}
					max++; // Empty at the end.

					int idx=0;
					if (max) {
						m_http_header_array = KLBNEWA(const char*, max);

						if (m_http_header_array==NULL) {
							// Connection is still busy,you can not send
							lua.retBoolean(false);
							ret = 1;
							break;
						}

						// Reset all handle to NULL
						for (idx = 0; idx < max; idx++) {
							m_http_header_array[idx] = NULL;
						}

						lua.retNil();
						idx = 0;
						while(lua.tableNext()) {
							lua.retValue(-2);
							const char * key = lua.getString(-1);
							const char * value = lua.getString(-2);
							lua.pop(2);

							if (key && value) {
								int length = strlen(key) + strlen(value) + 4;
								char* string = KLBNEWA(char, length);
								// Combine string
								sprintf(string, "%s: %s", key, value);

								m_http_header_array[idx++] = string;
							}
						}
					}
					m_http_header_length = idx;
				}

				// POST JSon payload
				const char* send_json = NULL;
				if(!lua.isNil(5)) {
					// POST method
					send_json = lua.getString(5);
				} else {
					// GET method if null json.
				}

				m_http = NetworkManager::createConnection();

				if (m_http) {
					if (m_http_header_array) {
						m_http->setHeader(m_http_header_array);
					}
					if (send_json) {
						char* json;
						const char * items[2];
						const char* req = "request_data=";
						int send_json_length = strlen( send_json );
						int req_length = strlen( req );

						json = KLBNEWA( char , send_json_length+req_length+1 );
						strcpy( json , req );
						strcat( json , send_json );
						items[0] = json;
						items[1] = NULL;

						m_http->setForm(items);
						m_http->httpPOST(api, false);

						KLBDELETEA(json);
					} else {
						m_http->httpGET(api, false);
					}

					m_timeout	= lua.getInt(6);
					m_timestart = 0;

					lua.retBoolean(true);
				} else {
					// Connection creation failed.
					lua.retBoolean(false);
				}
			}
		}
		break;
	}
	return 1;
}

bool
CKLBNetAPI::lua_callback(int msg, int status, CKLBJsonItem * pRoot)
{
	return CKLBScriptEnv::getInstance().call_netAPI_callback(m_callback, this, 0, msg, status, pRoot);
}
