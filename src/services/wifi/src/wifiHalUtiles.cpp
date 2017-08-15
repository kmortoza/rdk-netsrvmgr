/* COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file (and its contents) are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#include "wifiHalUtiles.h"
#include "netsrvmgrUtiles.h"


static WiFiStatusCode_t gWifiAdopterStatus = WIFI_UNINSTALLED;
static WiFiConnectionTypeCode_t gWifiConnectionType = WIFI_CON_UNKNOWN;
gchar deviceID[DEVICEID_SIZE];
gchar partnerID[PARTNERID_SIZE];
#ifdef ENABLE_LOST_FOUND
GList *lstLnfPvtResults=NULL;
#define WAIT_TIME_FOR_PRIVATE_CONNECTION 2
pthread_cond_t condLAF = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexLAF = PTHREAD_MUTEX_INITIALIZER;
pthread_t wifiStatusMonitorThread;
pthread_t lafConnectThread;
pthread_t lafConnectToPrivateThread;
pthread_mutex_t mutexTriggerLAF = PTHREAD_MUTEX_INITIALIZER;
WiFiLNFStatusCode_t gWifiLNFStatus = LNF_UNITIALIZED;
bool bDeviceActivated=false;
bool bLnfActivationLoop=false;
bool bLNFConnect=false;
bool bStopLNFWhileDisconnected=false;
bool isLAFCurrConnectedssid=false;
bool bIsStopLNFWhileDisconnected=false;
bool bAutoSwitchToPrivateEnabled=true;
bool bSwitch2Private=false;
#define TOTAL_NO_OF_RETRY 5
#endif
#ifdef USE_HOSTIF_WIFI_HAL
static WiFiStatusCode_t getWpaStatus();
#endif
static WiFiConnection wifiConnData;

static eConnMethodType wifi_conn_type;

pthread_mutex_t wifiStatusLock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t condGo = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutexGo = PTHREAD_MUTEX_INITIALIZER;
extern netMgrConfigProps confProp;

WiFiStatusCode_t get_WifiRadioStatus();


void set_WiFiConnectionType( WiFiConnectionTypeCode_t value);
WiFiConnectionTypeCode_t get_WifiConnectionType();

extern WiFiConnectionStatus savedWiFiConnList;
extern char gWifiMacAddress[MAC_ADDR_BUFF_LEN];
#ifdef USE_RDK_WIFI_HAL
static void wifi_status_action (wifiStatusCode_t , char *, unsigned short );
static wifiStatusCode_t connCode_prev_state;

extern telemetryParams wifiParams_Tele_Period1;
extern telemetryParams wifiParams_Tele_Period2;
static void logs_Period1_Params();
static void logs_Period2_Params();
#endif

#define RDK_ASSERT_NOT_NULL(P)          if ((P) == NULL) return EINVAL

struct _wifi_securityModes
{
    SsidSecurity 	securityMode;
    const char          *modeString;
} wifi_securityModes[] =
{
    { NET_WIFI_SECURITY_NONE,          		  	"No Security"                   },
    { NET_WIFI_SECURITY_WEP_64, 	          	"WEP (Open & Shared)"        	},
    { NET_WIFI_SECURITY_WPA_PSK_AES, 		  	"WPA-Personal, AES encryp."    	},
    { NET_WIFI_SECURITY_WPA_PSK_TKIP, 		 	"WPA-Personal, TKIP encryp."   	},
    { NET_WIFI_SECURITY_WPA2_PSK_AES,  			"WPA2-Personal, AES encryp."   	},
    { NET_WIFI_SECURITY_WPA2_PSK_TKIP, 			"WPA2-Personal, TKIP encryp."  	},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_TKIP,		"WPA-ENTERPRISE, TKIP"		},
    { NET_WIFI_SECURITY_WPA_ENTERPRISE_AES,		"WPA-ENTERPRISE, AES"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_TKIP,		"WPA2-ENTERPRISE, TKIP"		},
    { NET_WIFI_SECURITY_WPA2_ENTERPRISE_AES,		"WPA2-ENTERPRISE, AES"		},
    { NET_WIFI_SECURITY_NOT_SUPPORTED, 		  	"Security format not supported" },
};

SsidSecurity get_wifiSecurityModeFromString(char *secModeString)
{
    SsidSecurity mode = NET_WIFI_SECURITY_NOT_SUPPORTED;

    if(!secModeString) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed due to NULL. \n",__FUNCTION__, __LINE__ );
        return NET_WIFI_SECURITY_NONE;
    }

    int len = sizeof(wifi_securityModes)/sizeof(_wifi_securityModes);

    for(int i = 0; i < len; i++) {
	if(NULL != strcasestr(secModeString,wifi_securityModes[i].modeString)) {
//        if(0 == strncasecmp(wifi_securityModes[i].modeString, secModeString, strlen(secModeString))) {
            mode = wifi_securityModes[i].securityMode;
            break;
        }
    }
    return mode;
}

int get_int(const char* ptr)
{
    int *ret = (int *)ptr;
    return *ret;
}

bool get_boolean(const char *ptr)
{
    bool *ret = (bool *)ptr;
    return *ret;
}

bool ethernet_on()
{
    if(access( "/tmp/wifi-on", F_OK ) != -1 )
        return false;
    else
        return true;
}

static IARM_Result_t WiFi_IARM_Bus_BroadcastEvent(const char *ownerName, IARM_EventId_t eventId, void *data, size_t len)
{
    if( !ethernet_on() )
        IARM_Bus_BroadcastEvent(ownerName, eventId, data, len);
}

static void set_WiFiStatusCode( WiFiStatusCode_t status)
{
    pthread_mutex_lock(&wifiStatusLock);
    if(gWifiAdopterStatus != status)
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Wifi Status changed to %d \n", MODULE_NAME,__FUNCTION__, __LINE__, status );

    gWifiAdopterStatus = status;
    pthread_mutex_unlock(&wifiStatusLock);
}

static WiFiStatusCode_t get_WiFiStatusCode()
{
    return gWifiAdopterStatus;
}

static bool get_WiFiStatusCodeAsString (WiFiStatusCode_t code, char* string)
{
    switch (code)
    {
    case WIFI_UNINSTALLED:
        strcpy (string, "UNINSTALLED");
        break;
    case WIFI_DISABLED:
        strcpy (string, "DISABLED");
        break;
    case WIFI_DISCONNECTED:
        strcpy (string, "DISCONNECTED");
        break;
    case WIFI_PAIRING:
        strcpy (string, "PAIRING");
        break;
    case WIFI_CONNECTING:
        strcpy (string, "CONNECTING");
        break;
    case WIFI_CONNECTED:
        strcpy (string, "CONNECTED");
        break;
    case WIFI_FAILED:
        strcpy (string, "FAILED");
        break;
    default:
        return false;
    }
    return true;
}

#ifdef ENABLE_LOST_FOUND
WiFiLNFStatusCode_t get_WiFiLNFStatusCode()
{
    return gWifiLNFStatus;
}
#endif
void get_CurrentSsidInfo(WiFiConnectionStatus *curSSIDConnInfo)
{
    strncpy(curSSIDConnInfo->ssidSession.ssid, wifiConnData.ssid, strlen(wifiConnData.ssid)+1);
    curSSIDConnInfo->isConnected = (WIFI_CONNECTED == get_WiFiStatusCode())? true: false;
}

WiFiStatusCode_t get_WifiRadioStatus()
{
    bool ret = false;
    WiFiStatusCode_t status = WIFI_UNINSTALLED;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n",MODULE_NAME, __FUNCTION__, __LINE__ );

#ifdef USE_HOSTIF_WIFI_HAL
    HOSTIF_MsgData_t wifiParam = {0};
    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));

    strncpy(wifiParam.paramName, WIFI_ADAPTER_ENABLE_PARAM, strlen(WIFI_ADAPTER_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 1;
    wifiParam.paramtype = hostIf_BooleanType;
    wifiParam.reqType = HOSTIF_GET;

    ret = gpvFromTR069hostif(&wifiParam);
    if(ret)
    {
        bool radioStatus = get_boolean(wifiParam.paramValue);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Retrieved values from tr69hostif as \'%s\':\'%d\'.\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_ADAPTER_ENABLE_PARAM, radioStatus);
        if(true == radioStatus)
        {
            gWifiAdopterStatus = WIFI_CONNECTED;
            /*Check for Wpa status */
            gWifiAdopterStatus = getWpaStatus();
        }
        else
        {
            gWifiAdopterStatus = (WIFI_CONNECTED == gWifiAdopterStatus)?WIFI_DISCONNECTED:WIFI_DISABLED;
        }
    }

    status = gWifiAdopterStatus;
#else
    status = get_WiFiStatusCode();
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n",MODULE_NAME, __FUNCTION__, __LINE__ );
    return status;
}

void set_WiFiConnectionType( WiFiConnectionTypeCode_t value)
{
    if(value < 0 || value > WIFI_CON_MAX)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Trying to set invalid Wifi Connection Type %d \n",__FUNCTION__, __LINE__, value );
        return;
    }

    if(gWifiConnectionType != value)
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Wifi Connection Type changed to %d ..\n",__FUNCTION__, __LINE__, value );

    gWifiConnectionType = value;
}

WiFiConnectionTypeCode_t get_WifiConnectionType()
{
    return gWifiConnectionType;
}

#ifdef USE_HOSTIF_WIFI_HAL
bool updateWiFiList()
{
    bool ret = false;
    HOSTIF_MsgData_t wifiParam = {0};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
    strncpy(wifiParam.paramName, WIFI_SSID_ENABLE_PARAM, strlen(WIFI_SSID_ENABLE_PARAM) +1);
    wifiParam.instanceNum = 0;
    wifiParam.paramtype = hostIf_BooleanType;

    memset(&gSsidList, '\0', sizeof(ssidList));
    ret = gpvFromTR069hostif(&wifiParam);

    if(ret)
    {
        bool ssidStatus = get_boolean(wifiParam.paramValue);

        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Retrieved values from tr69hostif as \'%s\':\'%d\'.\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);

        if(ssidStatus)
        {
            // Get SSID
            memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
            strncpy(wifiParam.paramName, WIFI_SSID_NAME_PARAM, strlen(WIFI_SSID_NAME_PARAM) +1);
            wifiParam.paramtype = hostIf_StringType;
            wifiParam.instanceNum = 0;
            ret = gpvFromTR069hostif(&wifiParam);

            if(ret)
            {
                strncpy(gSsidList.ssid, wifiParam.paramValue, strlen(wifiParam.paramValue) +1);
            }

            // Get SSID
            memset(&wifiParam, '\0', sizeof(HOSTIF_MsgData_t));
            strncpy(wifiParam.paramName, WIFI_SSID_BSSID_PARAM, strlen(WIFI_SSID_BSSID_PARAM) +1);
            wifiParam.paramtype = hostIf_StringType;
            wifiParam.instanceNum = 0;
            ret = gpvFromTR069hostif(&wifiParam);

            if(ret)
            {
                strncpy(gSsidList.bssid,  wifiParam.paramValue, strlen(wifiParam.paramValue) +1);
            }

            /* Not Implemented in tr69 Hal.*/
            gSsidList.frequency = -1;
            gSsidList.signalstrength = -1;


        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] WiFi is disabled. [\'%s\':%d] \n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_SSID_ENABLE_PARAM, ssidStatus);
            ret = false;
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}




//---------------------------------------------------------
// generic Api for get HostIf parameters through IARM_TR69Bus
// --------------------------------------------------------

bool gpvFromTR069hostif( HOSTIF_MsgData_t *param)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;

    param->reqType = HOSTIF_GET;
//    HOSTIF_MsgData_t param = &wifiParam;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    /* IARM get Call*/
    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME, IARM_BUS_TR69HOSTIFMGR_API_GetParams, (void *)param, sizeof(HOSTIF_MsgData_t));

    if(ret != IARM_RESULT_SUCCESS)
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d] Failed with returned \'%d\' for '\%s\''. \n",  MODULE_NAME,__FUNCTION__, __LINE__, ret, param->paramName);
        //ret = ERR_REQUEST_DENIED;
        return false;
    }
    else
    {
        ret = IARM_RESULT_SUCCESS;
        RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Successfully returned the value of \'%s\'.\n",  MODULE_NAME,__FUNCTION__, __LINE__, param->paramName);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}

static WiFiStatusCode_t getWpaStatus()
{
    WiFiStatusCode_t ret = WIFI_UNINSTALLED;
    FILE *in = NULL;
    char buff[512] = {'\0'};

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    if(!(in = popen("wpa_cli status | grep -i wpa_state | cut -d = -f 2", "r")))
    {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d] Failed to read wpa_cli status.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ret;
    }

    if(fgets(buff, sizeof(buff), in)!=NULL)
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d]  State: %s\n", MODULE_NAME,__FUNCTION__, __LINE__, buff);
//		printf("%s", buff);
    }
    else {
        RDK_LOG( RDK_LOG_FATAL, LOG_NMGR, "[%s:%s:%d]  Failed to get State.\n", MODULE_NAME,__FUNCTION__, __LINE__);
        pclose(in);
        return ret;
    }

    if(0 == strncasecmp(buff, "DISCONNECTED", strlen("DISCONNECTED")))
    {
        ret = WIFI_DISCONNECTED;
    }
    else if (0 == strncasecmp(buff, "SCANNING", strlen("SCANNING")))
    {
        ret = WIFI_PAIRING;
    }
    else if (0 == strncasecmp(buff, "ASSOCIATED", strlen("ASSOCIATED")))
    {
        ret = WIFI_CONNECTING;
    }
    else if (0 == strncasecmp(buff, "COMPLETED", strlen("COMPLETED")))
    {
        ret = WIFI_CONNECTED;
    }
    else if (0 == strncasecmp(buff, "INTERFACE_DISABLED", strlen("INTERFACE_DISABLED")))
    {
        ret = WIFI_UNINSTALLED;
    }
    else
    {
        ret = WIFI_FAILED;
    }

    pclose(in);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}
#endif /* #ifdef USE_HOSTIF_WIFI_HAL */

#ifdef  USE_RDK_WIFI_HAL

/*Connect using WPS Push */
bool connect_WpsPush()
{
    bool ret = false;
    INT ssidIndex = 1;
    INT wpsStatus = RETURN_OK;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&eventData, 0, sizeof(eventData));

    if(WIFI_CONNECTING == get_WiFiStatusCode()) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Connecting to AP is in Progress..., please try after 120 seconds.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ret;
    }

    if (WIFI_CONNECTED != get_WiFiStatusCode()) {

        set_WiFiConnectionType(WIFI_CON_WPS);

        wpsStatus = wifi_setCliWpsButtonPush(ssidIndex);

        if(RETURN_OK == wpsStatus)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Successfully called \"wifi_setCliWpsButtonPush(%d)\"; WPS Push button Success. \n",\
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);
            ret = true;

            /*Generate Event for Connect State*/
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            set_WiFiStatusCode(WIFI_CONNECTING);
            wifi_conn_type = WPS_PUSH_CONNECT;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_setCliWpsButtonPush(%d)\", WPS Push button press failed with status code (%d) \n",
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);

            eventData.data.wifiStateChange.state = WIFI_FAILED;
        }
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
    }
    else if (WIFI_CONNECTED == get_WiFiStatusCode())
    {
        /*If connected, do Disconnect first, the again re-connect to same or different AP.
         * Wait for 60 Sec timeout period if fails to disconnect, then show error.
             * If successfully disconncted, then go ahead and so call connect WpsPush. */
        if(RETURN_OK == wifi_disconnectEndpoint(1, wifiConnData.ssid))
        {
            time_t start_time = time(NULL);
            time_t timeout_period = start_time + confProp.wifiProps.max_timeout/*MAX_TIME_OUT_PERIOD */;

            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\"; Already Connected to \"%s\" AP. \"wifi_disconnectEndpointd)\" Successfully on WPS Push. \n",\
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wifiConnData.ssid, wpsStatus);
            /* Check for status change from Callback function */
            while (start_time < timeout_period)
            {
                if(WIFI_DISCONNECTED == gWifiAdopterStatus)  {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully received Disconnect state \"%d\". \n", MODULE_NAME,__FUNCTION__, __LINE__, gWifiAdopterStatus);
                    set_WiFiConnectionType(WIFI_CON_WPS);
                    ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
                    eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
                    set_WiFiStatusCode(eventData.data.wifiStateChange.state);
                    WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'%s\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__,
                             (ret?"CONNECTING": "FAILED"), eventData.data.wifiStateChange.state);
                    if(ret) {
                        remove(WIFI_BCK_FILENAME);
                        wifi_conn_type = WPS_PUSH_CONNECT;
                    }
                    break;
                }
                else {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to Disconnect \"%s\";  wait for %d sec (i.e., loop time : %s) to update disconnect state by wifi_disconnect_callback. (%d) \n",
                             MODULE_NAME,__FUNCTION__, __LINE__, wifiConnData.ssid, MAX_TIME_OUT_PERIOD, ctime(&start_time), gWifiAdopterStatus);
                    sleep(RETRY_TIME_INTERVAL);
                    start_time = time(NULL);
                }
            }
        } else {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Received WPS KeyCode \"%d\";  Failed in \"wifi_disconnectEndpointd(%d)\", WPS Push button press failed with status code (%d) \n",
                     MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, wpsStatus);
        }

        if(false == ret)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Since failed to disconnect, so reconnecintg again. \"%d\". \n", MODULE_NAME,__FUNCTION__, __LINE__);
            set_WiFiConnectionType(WIFI_CON_WPS);
            ret = (RETURN_OK == wifi_setCliWpsButtonPush(ssidIndex))?true:false;
            eventData.data.wifiStateChange.state = (ret)? WIFI_CONNECTING: WIFI_FAILED;
            set_WiFiStatusCode(eventData.data.wifiStateChange.state);
            WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
            if(ret) {
                wifi_conn_type = WPS_PUSH_CONNECT;
            }
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}



INT wifi_connect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short) ACTION_ON_CONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

INT wifi_disconnect_callback(INT ssidIndex, CHAR *AP_SSID, wifiStatusCode_t *connStatus)
{
    int ret = RETURN_OK;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifiStatusCode_t connCode = *connStatus;
    wifi_status_action (connCode, AP_SSID, (unsigned short)ACTION_ON_DISCONNECT);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}

void wifi_status_action (wifiStatusCode_t connCode, char *ap_SSID, unsigned short action)
{
    static unsigned int switchLnf2Priv=0;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_MAX;
    bool notify = false;
    char command[128]= {'\0'};
    const char *connStr = (action == ACTION_ON_CONNECT)?"Connect": "Disconnect";
    memset(&eventData, 0, sizeof(eventData));

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] WiFi Code %d \n", __FUNCTION__, __LINE__ ,connCode);
    switch(connCode) {
    case WIFI_HAL_SUCCESS:

        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully %s to AP %s. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, ap_SSID);

        if (ACTION_ON_CONNECT == action) {
            set_WiFiStatusCode(WIFI_CONNECTED);
            notify = true;
#if 0       /* Do not bounce for any network switch. Do it only from LF to private. */
            if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
            {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] refresh xre session failed .\n", MODULE_NAME,__FUNCTION__, __LINE__);
            }
#endif
            /* one condition variable is signaled */

            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);
            if (! laf_is_lnfssid(ap_SSID))
            {
                isLAFCurrConnectedssid=false;
		gWifiLNFStatus=CONNECTED_PRIVATE;
		if(bStopLNFWhileDisconnected)
		{
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Enable LNF \n",__FUNCTION__, __LINE__ );
		    bStopLNFWhileDisconnected=false;
		}
                if(strcmp(savedWiFiConnList.ssidSession.ssid, ap_SSID) != 0)
                    storeMfrWifiCredentials();
                memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
                strncpy(savedWiFiConnList.ssidSession.ssid, ap_SSID, strlen(ap_SSID)+1);
                strcpy(savedWiFiConnList.ssidSession.passphrase, " ");
                savedWiFiConnList.conn_type = wifi_conn_type;
		if(switchLnf2Priv)
		{
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] get dhcp lease since there is a network change. \n",__FUNCTION__, __LINE__ );
                    netSrvMgrUtiles::triggerDhcpLease();
		    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Connecting private ssid. Bouncing xre connection.\n",__FUNCTION__, __LINE__ );
        	    if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
        	    {
            		RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] refresh xre session failed .\n",__FUNCTION__, __LINE__);
        	    }
		    switchLnf2Priv=0;
		}
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:CONNECTED,%s\n",ap_SSID);
            }
            else {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] This is a LNF SSID so no storing \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                isLAFCurrConnectedssid=true;
		if(!switchLnf2Priv)
		   switchLnf2Priv=1;
            }


            /*Write into file*/
//            WiFiConnectionStatus wifiParams;

            /*Generate Event for Connect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTED\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_CONNECTED);
            pthread_mutex_lock(&mutexGo);
            if(0 == pthread_cond_signal(&condGo))
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast to monitor. \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            pthread_mutex_unlock(&mutexGo);
        } else if (ACTION_ON_DISCONNECT == action) {
            notify = true;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            memset(&wifiConnData, '\0', sizeof(wifiConnData));
            strncpy(wifiConnData.ssid, ap_SSID, strlen(ap_SSID)+1);

            /*Generate Event for Disconnect State*/
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_DISCONNECTED;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'DISCONNECTED\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_DISCONNECTED);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_DISCONNECTED\n");
        }
        break;
    case WIFI_HAL_CONNECTING:
            notify = true;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connecting to AP in Progress... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            set_WiFiStatusCode(WIFI_CONNECTING);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_CONNECTING;
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged' with state as \'CONNECTING\'(%d).\n", MODULE_NAME,__FUNCTION__, __LINE__, WIFI_CONNECTING);
        break;

    case WIFI_HAL_DISCONNECTING:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Disconnecting from AP in Progress... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        }
        break;
    case WIFI_HAL_ERROR_NOT_FOUND:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., AP not found). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_NO_SSID;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'NO_SSID\'(%d), CurrentState set as %d (DISCONNECTED)\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code, WIFI_DISCONNECTED);
        }
        break;

    case WIFI_HAL_ERROR_TIMEOUT_EXPIRED:
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., Timeout expired). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
        /* Event Id & Code */
        if(connCode_prev_state != connCode) {
            notify = true;
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;

    case WIFI_HAL_ERROR_DEV_DISCONNECT:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with wifiStatusCode %d (i.e., Fail in Device/AP Disconnect). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connStr, connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
        }
        break;
    /* the SSID of the network changed */
    case WIFI_HAL_ERROR_SSID_CHANGED:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to SSID Change (%d) . \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_SSID_CHANGED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            if(confProp.wifiProps.bEnableLostFound)
            {
                lnfConnectPrivCredentials();
            }
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'SSID_CHANGED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_SSID_CHANGED\n");
        }
        break;
    /* the connection to the network was lost */
    case WIFI_HAL_ERROR_CONNECTION_LOST:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to  CONNECTION LOST (%d) from %s. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode, ap_SSID);
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_LOST;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_LOST\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_LOST\n");
        }
        break;
    /* the connection failed for an unknown reason */
    case WIFI_HAL_ERROR_CONNECTION_FAILED:
        if(connCode_prev_state != connCode)
        {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Connection Failed (%d) due to unknown reason. \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_FAILED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_FAILED\n");
        }
        break;
    /* the connection was interrupted */
    case WIFI_HAL_ERROR_CONNECTION_INTERRUPTED:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to Connection Interrupted (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_CONNECTION_INTERRUPTED;
            set_WiFiStatusCode(WIFI_DISCONNECTED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'CONNECTION_INTERRUPTED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_CONNECTION_INTERRUPTED\n");
        }
        break;
    /* the connection failed due to invalid credentials */
    case WIFI_HAL_ERROR_INVALID_CREDENTIALS:
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to Invalid Credentials. (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_INVALID_CREDENTIALS;
            set_WiFiStatusCode(WIFI_DISCONNECTED);

            if(confProp.wifiProps.bEnableLostFound)
            {
                lnfConnectPrivCredentials();
            }
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'INVALID_CREDENTIALS\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_INVALID_CREDENTIALS\n");
        break;
    case WIFI_HAL_UNRECOVERABLE_ERROR:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed due to UNRECOVERABLE ERROR. (%d). \n", MODULE_NAME,__FUNCTION__, __LINE__ , connCode );
            eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;
            eventData.data.wifiStateChange.state = WIFI_FAILED;
            set_WiFiStatusCode(WIFI_FAILED);
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onWIFIStateChanged (%d)' with state as \'FAILED\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiStateChange.state);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_UNRECOVERABLE_ERROR\n");
        }
        break;
    case WIFI_HAL_ERROR_UNKNOWN:
    default:
        if(connCode_prev_state != connCode) {
            notify = true;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed in %s with WiFi Error Code %d (i.e., WIFI_HAL_ERROR_UNKNOWN). \n", MODULE_NAME,__FUNCTION__, __LINE__, connStr, connCode );
            /* Event Id & Code */
            eventId = IARM_BUS_WIFI_MGR_EVENT_onError;
            eventData.data.wifiError.code = WIFI_UNKNOWN;
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Notification on 'onError (%d)' with state as \'UNKNOWN\'(%d).\n", \
                     MODULE_NAME,__FUNCTION__, __LINE__,eventId,  eventData.data.wifiError.code);
	    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:DISCONNECTED,WIFI_HAL_ERROR_UNKNOWN\n");
        }
        break;
    }

    connCode_prev_state = connCode;
    if(notify && ((eventId >= IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged) && (eventId < IARM_BUS_WIFI_MGR_EVENT_MAX)))
    {
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,
                                     (IARM_EventId_t) eventId,
                                     (void *)&eventData, sizeof(eventData));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Broadcast Event id \'%d\'. \n", MODULE_NAME,__FUNCTION__, __LINE__, eventId);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

/*Connect using SSID Selection */
bool connect_withSSID(int ssidIndex, char *ap_SSID, SsidSecurity ap_security_mode, CHAR *ap_security_WEPKey, CHAR *ap_security_PreSharedKey, CHAR *ap_security_KeyPassphrase,int saveSSID,CHAR * eapIdentity,CHAR * carootcert,CHAR * clientcert,CHAR * privatekey,int conType)
{
    int ret = true;
    IARM_BUS_WiFiSrvMgr_EventData_t eventData;
    wifiSecurityMode_t securityMode;
    IARM_Bus_NMgr_WiFi_EventId_t eventId = IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    set_WiFiConnectionType((WiFiConnectionTypeCode_t)conType);
    securityMode=(wifiSecurityMode_t)ap_security_mode;
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR,"[%s:%s:%d] ssidIndex %d; ap_SSID : %s; ap_security_mode : %d; saveSSID = %d  \n",
             MODULE_NAME,__FUNCTION__, __LINE__, ssidIndex, ap_SSID, (int)ap_security_mode, saveSSID );
    if(saveSSID)
    {
        set_WiFiStatusCode(WIFI_CONNECTING);
        eventData.data.wifiStateChange.state = WIFI_CONNECTING;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"[%s:%s:%d] connecting to ssid %s  with passphrase %s \n",
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_SSID, ap_security_KeyPassphrase);
    }
    ret=wifi_connectEndpoint(ssidIndex, ap_SSID, securityMode, ap_security_WEPKey, ap_security_PreSharedKey, ap_security_KeyPassphrase, saveSSID,eapIdentity,carootcert,clientcert,privatekey);
    if(ret)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] Error in connecting to ssid %s  with passphrase %s \n",
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_SSID, ap_security_KeyPassphrase);
        eventData.data.wifiStateChange.state = WIFI_FAILED;
        WiFi_IARM_Bus_BroadcastEvent(IARM_BUS_NM_SRV_MGR_NAME,  (IARM_EventId_t) eventId, (void *)&eventData, sizeof(eventData));
        ret = false;
    }
    else
    {
        ret = true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}

bool scan_Neighboring_WifiAP(char *buffer)
{
    bool ret = false;
    INT radioIndex = 0,  index;
    UINT output_array_size = 0;
    wifi_neighbor_ap_t *neighbor_ap_array = NULL;
    char *ssid = NULL;
    int security = 0;
    int signalStrength = 0;
    double frequency = 0;
    SsidSecurity encrptType = NET_WIFI_SECURITY_NONE;
    cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
    char *out = NULL;
    char *pFreq = NULL;


    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter..\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(RETURN_OK != wifi_disconnectEndpoint(1,NULL))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint()", __FUNCTION__, __LINE__);
    }

    ret = wifi_getNeighboringWiFiDiagnosticResult(radioIndex, &neighbor_ap_array, &output_array_size);

    if((RETURN_OK != ret ) && ((NULL == neighbor_ap_array) || (0 == output_array_size)))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] Failed in \'wifi_getNeighboringWiFiDiagnosticResult()\' with NULL \'neighbor_ap_array\', size %d and returns as %d \n",
                 MODULE_NAME,__FUNCTION__, __LINE__, output_array_size, ret);
        return false;
    }
    else {
        ret = true;
    }

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR,"[%s:%s:%d] \'wifi_getNeighboringWiFiDiagnosticResult()\' comes with \'neighbor_ap_array\', size %d and returns as %d \n",
             MODULE_NAME,__FUNCTION__, __LINE__, output_array_size, ret);

    rootObj = cJSON_CreateObject();

    if(NULL == rootObj) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] \'Failed to create root json object.\n",  MODULE_NAME,__FUNCTION__, __LINE__);
        return false;
    }

    cJSON_AddItemToObject(rootObj, "getAvailableSSIDs", array_obj=cJSON_CreateArray());

    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n*********** Start: SSID Scan List **************** \n");
    for (index = 0; index < output_array_size; index++ )
    {
//        char temp[500] = {'\0'};

        ssid = neighbor_ap_array[index].ap_SSID;
        if(ssid[0] != '\0') {
            signalStrength = neighbor_ap_array[index].ap_SignalStrength;
            frequency = strtod(neighbor_ap_array[index].ap_OperatingFrequencyBand, &pFreq);


            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] [%d] => SSID : \"%s\"  | SignalStrength : \"%d\" | frequency : \"%f\" | EncryptionMode : \"%s\" \n",\
                     MODULE_NAME,__FUNCTION__, __LINE__, index, ssid, signalStrength, frequency, neighbor_ap_array[index].ap_EncryptionMode );

            /* The type of encryption the neighboring WiFi SSID advertises.*/
            encrptType = get_wifiSecurityModeFromString((char *)neighbor_ap_array[index].ap_EncryptionMode);

            cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
            cJSON_AddStringToObject(array_element, "ssid", ssid);
            cJSON_AddNumberToObject(array_element, "security", encrptType);
            cJSON_AddNumberToObject(array_element, "signalStrength", signalStrength);
            cJSON_AddNumberToObject(array_element, "frequency", frequency);
        }
    }
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n***********End: SSID Scan List **************** \n\n");


    out = cJSON_PrintUnformatted(rootObj);

    if(out) {
        strncpy(buffer, out, strlen(out)+1);
    }

    if(rootObj) {
        cJSON_Delete(rootObj);
    }
    if(out) free(out);

    if(neighbor_ap_array) {
        RDK_LOG( RDK_LOG_DEBUG,LOG_NMGR, "[%s:%s:%d] malloc allocated = %d \n", MODULE_NAME,__FUNCTION__, __LINE__ , malloc_usable_size(neighbor_ap_array));
        free(neighbor_ap_array);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}
bool lastConnectedSSID(WiFiConnectionStatus *ConnParams)
{
    char ap_ssid[SSID_SIZE];
    char ap_passphrase[PASSPHRASE_BUFF];
    char ap_bssid[BUFF_LENGTH_32];
    char ap_security[BUFF_LENGTH_32];
    wifi_pairedSSIDInfo_t pairedSSIDInfo={0};
    bool ret = true;
    int retVal;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    retVal=wifi_lastConnected_Endpoint(&pairedSSIDInfo);
    if(retVal != RETURN_OK )
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] Error in getting lost connected SSID \n", MODULE_NAME,__FUNCTION__, __LINE__);
        ret = false;
    }
    else
    {
        strncpy(ConnParams->ssidSession.ssid, pairedSSIDInfo.ap_ssid, sizeof(ConnParams->ssidSession.ssid)-1);
        ConnParams->ssidSession.ssid[sizeof(ConnParams->ssidSession.ssid)-1]='\0';
        strncpy(ConnParams->ssidSession.passphrase, pairedSSIDInfo.ap_passphrase, sizeof(ConnParams->ssidSession.passphrase)-1);
        ConnParams->ssidSession.passphrase[sizeof(ConnParams->ssidSession.passphrase)-1]='\0';
        strncpy(ConnParams->ssidSession.bssid, pairedSSIDInfo.ap_bssid, sizeof(ConnParams->ssidSession.bssid)-1);
        ConnParams->ssidSession.bssid[sizeof(ConnParams->ssidSession.bssid)-1]='\0';
        strncpy(ConnParams->ssidSession.security, pairedSSIDInfo.ap_security, sizeof(ConnParams->ssidSession.security)-1);
        ConnParams->ssidSession.security[sizeof(ConnParams->ssidSession.security)-1]='\0';
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR,"[%s:%s:%d] last connected  ssid is  %s  bssid is %s \n", MODULE_NAME,__FUNCTION__, __LINE__,ConnParams->ssidSession.ssid,ConnParams->ssidSession.bssid);
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return ret;
}
#endif

bool isWiFiCapable()
{
    bool isCapable = false;
    unsigned long numRadioEntries = 0;
#ifdef  USE_RDK_WIFI_HAL
    wifi_getRadioNumberOfEntries(&numRadioEntries);
    /* Default set to 1 for Xi5, it will change for other devices.*/
    numRadioEntries = 1;
#endif

    if(numRadioEntries)	{
        gWifiAdopterStatus = WIFI_DISCONNECTED;
        isCapable = true;
    }
    return isCapable;
}

#ifdef USE_RDK_WIFI_HAL

void *wifiConnStatusThread(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    wifi_sta_stats_t stats;
    WiFiStatusCode_t wifiStatusCode;
    char wifiStatusAsString[32];

    while (true ) {
        pthread_mutex_lock(&mutexGo);

        if(ret = pthread_cond_wait(&condGo, &mutexGo) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] ***** Monitor activated by signal ***** \n", MODULE_NAME,__FUNCTION__, __LINE__ );

            pthread_mutex_unlock(&mutexGo);

            while (true) {
                wifiStatusCode = get_WiFiStatusCode();

                if (get_WiFiStatusCodeAsString (wifiStatusCode, wifiStatusAsString)) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:%s\n", wifiStatusAsString);
                }
                else {
                    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "TELEMETRY_WIFI_CONNECTION_STATUS:Unmappable WiFi status code %d\n", wifiStatusCode);
                }

                if (WIFI_CONNECTED == wifiStatusCode) {
                    //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****Start Monitoring ***** \n");
                    memset(&stats, 0, sizeof(wifi_sta_stats_t));
                    wifi_getStats(1, &stats);
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_WIFI_STATS:%s,%d,%d,%d\n",
                            stats.sta_SSID, (int)stats.sta_PhyRate, (int)stats.sta_Noise, (int)stats.sta_RSSI);
                    //RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "\n *****End Monitoring  ***** \n");

                    /*Telemetry Parameter logging*/
                    logs_Period1_Params();
                    logs_Period2_Params();
                }

                sleep(confProp.wifiProps.statsParam_PollInterval);
            }
        }
        else
        {
            pthread_mutex_unlock(&mutexGo);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

void monitor_WiFiStatus()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&wifiStatusMonitorThread, &attr, wifiConnStatusThread, NULL);
}


bool clearSSID_On_Disconnect_AP()
{
    bool ret = true;
    int status = 0;
    char *ap_ssid = savedWiFiConnList.ssidSession.ssid;
    if(RETURN_OK != wifi_disconnectEndpoint(1, ap_ssid))
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Failed to  Disconnect in wifi_disconnectEndpoint() for AP : \"%s\".\n",\
                 MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
        ret = false;
    }
    else
    {
        RDK_LOG(RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Successfully called \"wifi_disconnectEndpointd()\" for AP: \'%s\'.  \n",\
                MODULE_NAME,__FUNCTION__, __LINE__, ap_ssid);
        if ( false == isLAFCurrConnectedssid )
        {
            memset(&savedWiFiConnList, 0 ,sizeof(savedWiFiConnList));
            eraseMfrWifiCredentials();
        }
    }
    return ret;
}

void getConnectedSSIDInfo(WiFiConnectedSSIDInfo_t *conSSIDInfo)
{
    bool ret = true;
    int radioIndex = 1;
    wifi_sta_stats_t stats;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);
    strncpy((char *)conSSIDInfo->ssid, (const char *)stats.sta_SSID, (size_t)BUFF_LENGTH_64);
    strncpy((char *)conSSIDInfo->bssid, (const char *)stats.sta_BSSID, (size_t)BUFF_LENGTH_64);
    conSSIDInfo->rate = stats.sta_PhyRate;
    conSSIDInfo->noise = stats.sta_Noise;
    conSSIDInfo->signalStrength = stats.sta_RSSI;

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Connected SSID info: \n \
    		[SSID: \"%s\"| BSSID : \"%s\" | PhyRate : \"%f\" | Noise : \"%f\" | SignalStrength(rssi) : \"%f\"] \n",
            MODULE_NAME,__FUNCTION__, __LINE__,
            stats.sta_SSID, stats.sta_BSSID, stats.sta_PhyRate, stats.sta_Noise, stats.sta_RSSI);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

#endif

#ifdef ENABLE_LOST_FOUND
bool triggerLostFound(LAF_REQUEST_TYPE lafRequestType)
{
    laf_conf_t* conf = NULL;
    laf_client_t* clnt = NULL;
    laf_ops_t *ops = NULL;
    laf_device_info_t *dev_info = NULL;
    char currTime[BUFF_LENGTH_32];
    static bool bLastLnfSuccess=false;
    int err;
    bool bRet=true;
    mfrSerializedType_t mfrType;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    ops = (laf_ops_t*) malloc(sizeof(laf_ops_t));
    if(!ops)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] laf_ops malloc failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return ENOMEM;
    }
    dev_info = (laf_device_info_t*) malloc(sizeof(laf_device_info_t));
    if(!dev_info)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] laf_device_info malloc failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        free(ops);
        return ENOMEM;
    }
    memset(dev_info,0,sizeof(laf_device_info_t));
    memset(ops,0,sizeof(laf_ops_t));
    /* set operation parameters */
    ops->laf_log_message = log_message;
    ops->laf_wifi_connect = laf_wifi_connect;
    ops->laf_wifi_disconnect = laf_wifi_disconnect;
    ops->laf_get_lfat = laf_get_lfat; 
    ops->laf_set_lfat = laf_set_lfat;

    bRet = getDeviceInfo(dev_info);
    if (bRet == false) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getDeviceInfo failed\n", __FUNCTION__, __LINE__ );
    }
    /* configure laf */
    err = laf_config_new(&conf, ops, dev_info);
    if(err != 0) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Lost and found client configurtion failed with error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
        bRet=false;
    }
    else
    {
        /* initialize laf client */
        err = laf_client_new(&clnt, conf);
        if(err != 0) {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Lost and found client initialization failed with error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
            bRet=false;
        }
        else
        {
            /* start laf */
            clnt->req_type = lafRequestType;
            err = laf_start(clnt);
            if(err != 0) {
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Error in lost and found client, error code %d \n", MODULE_NAME,__FUNCTION__, __LINE__,err );
                  bRet=false;
            }
            if(!bAutoSwitchToPrivateEnabled)
            {
	      if((!bRet) && (bLastLnfSuccess))
	      {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] !AutoSwitchToPrivateEnabled + lastLnfSuccessful + currLnfFailure = clr switch2prv Results \n", MODULE_NAME,__FUNCTION__, __LINE__ );
		clearSwitchToPrivateResults();
		bLastLnfSuccess=false;
              }
	      if(bRet)
		bLastLnfSuccess=true;
              netSrvMgrUtiles::getCurrentTime(currTime,(char *)TIME_FORMAT);
              addSwitchToPrivateResults(err,currTime);
  	      RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Added Time=%s lnf error=%d to list,length of list = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,currTime,err,g_list_length(lstLnfPvtResults));
            }
        }
    }
    free(dev_info);
    free(ops);
    /* destroy config */
    if(conf)
        laf_config_destroy(conf);
    /* destroy lost and found client */
    if(clnt)
        laf_client_destroy(clnt);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bRet;
}
bool getmacaddress(gchar* ifname,GString *data)
{
    int fd;
    int ioRet;
    struct ifreq ifr;
    unsigned char *mac;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] ERROR opening socket %d \n", MODULE_NAME,__FUNCTION__, __LINE__,fd );
        return false;
    }
    ifr.ifr_addr.sa_family = AF_INET;
    strncpy(ifr.ifr_name , ifname , IFNAMSIZ-1);
    ioRet=ioctl(fd, SIOCGIFHWADDR, &ifr);
    if (ioRet < 0)
    {
        close(fd);
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] ERROR in ioctl function to retrieve mac address %d\n", MODULE_NAME,__FUNCTION__, __LINE__,fd );
        return false;
    }
    close(fd);
    mac = (unsigned char *)ifr.ifr_hwaddr.sa_data;
    g_string_printf(data,"%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}
#if 1   // when rdk-c is moved to stable2 they need to put the correct #define
bool getMfrData(GString* mfrDataStr,mfrSerializedType_t mfrType)
{
    bool bRet;
    IARM_Bus_MFRLib_GetSerializedData_Param_t param;
    IARM_Result_t iarmRet = IARM_RESULT_IPCCORE_FAIL;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    memset(&param, 0, sizeof(param));
    param.type = mfrType;
    iarmRet = IARM_Bus_Call(IARM_BUS_MFRLIB_NAME, IARM_BUS_MFRLIB_API_GetSerializedData, &param, sizeof(param));
    if(iarmRet == IARM_RESULT_SUCCESS)
    {
        if(param.buffer && param.bufLen)
        {
            RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] serialized data %s for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,param.buffer,mfrType );
            g_string_assign(mfrDataStr,param.buffer);
            bRet = true;
        }
        else
        {
            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] serialized data is empty for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrType );
            bRet = false;
        }
    }
    else
    {
        bRet = false;
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] IARM CALL failed  for mfrtype %d \n", MODULE_NAME,__FUNCTION__, __LINE__,mfrType );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bRet;
}

bool getDeviceInfo(laf_device_info_t *dev_info)
{
    bool bRet = true;
    GString* mfrSerialNum = g_string_new(NULL);
    GString* mfrMake = g_string_new(NULL);
    GString* mfrModelName = g_string_new(NULL);
    GString* deviceMacAddr = g_string_new(NULL);
    mfrSerializedType_t mfrType;

    mfrType = mfrSERIALIZED_TYPE_SERIALNUMBER;
    if(partnerID && !partnerID[0])
    {
      if(getDeviceActivationState() == false)
      {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Partner ID Missing in url May be not activated \n", MODULE_NAME,__FUNCTION__, __LINE__);
      }
      else
      {
        g_stpcpy(dev_info->partnerId,partnerID);
      }
    }
    else
    {
      g_stpcpy(dev_info->partnerId,partnerID);
    }
    if(getMfrData(mfrSerialNum,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] mfrSerialNum = %s mfrType = %d \n", __FUNCTION__, __LINE__,mfrSerialNum->str,mfrType );
        strcpy(dev_info->serial_num,mfrSerialNum->str);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting serial num from mfr failed \n", __FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_DEVICEMAC;
    if(getMfrData(deviceMacAddr,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] deviceMacAddr = %s mfrType = %d \n", __FUNCTION__, __LINE__,deviceMacAddr->str,mfrType );
        strcpy(dev_info->mac,deviceMacAddr->str);

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting device mac addr from mfr failed \n", __FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }
    mfrType = mfrSERIALIZED_TYPE_MODELNAME;
    if(getMfrData(mfrModelName,mfrType))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] mfrModelName = %s mfrType = %d \n", __FUNCTION__, __LINE__,mfrModelName->str,mfrType );
        strcpy(dev_info->model,mfrModelName->str);
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] getting model name from mfr failed \n", __FUNCTION__, __LINE__);
        bRet = false;
        goto out;
    }
out:
    g_string_free(mfrSerialNum,TRUE);
    g_string_free(mfrMake,TRUE);
    g_string_free(mfrModelName,TRUE);
    g_string_free(deviceMacAddr,TRUE);

    return bRet;
}

#else

bool getDeviceInfo( laf_device_info_t *dev_info )
{
#ifdef ENABLE_XCAM_SUPPORT
    int ret = 0;
    struct basic_info *xcam_dev_info;

    xcam_dev_info = (struct basic_info*)malloc(sizeof(xcam_dev_info));
    if (xcam_dev_info)
    {
        ret = rdkc_get_device_basic_info(xcam_dev_info);
        if (ret == 0)
        {
            strcpy(dev_info->serial_num, xcam_dev_info->serial_number);
            strcpy(dev_info->mac, xcam_dev_info->mac_addr);
            strcpy(dev_info->model, xcam_dev_info->model);
            return true;
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] basic_info malloc failed\n", __FUNCTION__, __LINE__);
    }
#endif

    return false;
}

#endif

/* Callback function to connect to wifi */
int laf_wifi_connect(laf_wifi_ssid_t* const wificred)
{
    /* call api to connect to wifi */
    int retry = 0;
    bool retVal=false;
    int ssidIndex=1;
    bool notify = false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if (laf_is_lnfssid(wificred->ssid) && (true == isLAFCurrConnectedssid) && (WIFI_CONNECTED == get_WifiRadioStatus()))
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already connected to LF ssid Ignoring the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }
    if (laf_is_lnfssid(wificred->ssid))
    {
        bLNFConnect=true;
    }
    else
    {
        bLNFConnect=false;
    }
#ifdef USE_RDK_WIFI_HAL
        retVal=connect_withSSID(ssidIndex, wificred->ssid, (SsidSecurity)wificred->security_mode, NULL, wificred->psk, wificred->passphrase, (int)(!bLNFConnect), NULL, NULL, NULL, NULL, bLNFConnect ? WIFI_CON_LNF : WIFI_CON_PRIVATE);
//    retVal=connect_withSSID(ssidIndex, wificred->ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, wificred->psk);
#endif
    if(false == retVal)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] connect with ssid %s passphrase %s failed \n", MODULE_NAME,__FUNCTION__, __LINE__,wificred->ssid,wificred->passphrase );
        return EPERM;
    }
    while(get_WifiRadioStatus() != WIFI_CONNECTED && retry <= 30) {
        retry++; //max wait for 180 seconds
        if(bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            return EPERM;
        }
        sleep(1);
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Device not connected to wifi. waiting to connect... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    if(retry > 30) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Waited for 30seconds. Failed to connect. Abort \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }

    /* Bounce xre session only if switching from LF to private */
    if(! laf_is_lnfssid(wificred->ssid))
    {
        notify = true;
//        sleep(10); //waiting for default route before bouncing the xre connection
//        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connecting private ssid. Bouncing xre connection.\n", MODULE_NAME,__FUNCTION__, __LINE__ );
//#ifdef ENABLE_IARM
//        if(false == setHostifParam(XRE_REFRESH_SESSION ,hostIf_BooleanType ,(void *)&notify))
//        {
//            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] refresh xre session failed .\n", MODULE_NAME,__FUNCTION__, __LINE__);
//        }
//#endif
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return 0;
}

/* Callback function to disconenct wifi */
int laf_wifi_disconnect(void)
{
    int retry = 0;
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if ( false == isLAFCurrConnectedssid )
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Connected to private ssid Ignoring the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return 0;
    }
    if(bStopLNFWhileDisconnected)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }
#ifdef USE_RDK_WIFI_HAL
    retVal = clearSSID_On_Disconnect_AP();
#endif
    if(retVal == false) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Tried to disconnect before connect and it failed \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }

    while(get_WifiRadioStatus() != WIFI_DISCONNECTED && retry <= 30) {
        if(bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] Already WPS flow has intiated so skipping the request the request \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            return EPERM;
        }
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Device is connected to wifi. waiting to disconnect... \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retry++; //max wait for 180 seconds
        sleep(1);
    }
    if(retry > 30) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Waited for 30seconds. Failed to disconnect. Abort \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return EPERM;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return 0;
}

/* log level to display */
static rdk_LogLevel map_to_rdkloglevel(int level)
{
    rdk_LogLevel rdklevel;

    switch(level)
    {
    case LAF_LOG_DEBUG:
        rdklevel = RDK_LOG_DEBUG;
        break;
    case LAF_LOG_INFO:
        rdklevel = RDK_LOG_INFO;
        break;
    case LAF_LOG_WARNING:
        rdklevel = RDK_LOG_WARN;
        break;
    case LAF_LOG_ERROR:
        rdklevel = RDK_LOG_ERROR;
        break;
    default:
        rdklevel = RDK_LOG_INFO;
        break;
    }
    return rdklevel;
}
/* callback function to log messages from lost and found */
void log_message(laf_loglevel_t level, char const* function, int line, char const* msg)
{
    RDK_LOG( map_to_rdkloglevel(level), LOG_NMGR, "[%s:%s:%s:%d] %s\n",MODULE_NAME,SUB_MODULE_NAME, function, line, msg );
}
void *lafConnPrivThread(void* arg)
{
    int ret = 0;
    int counter=0;
    bool retVal;
    int ssidIndex=1;
    LAF_REQUEST_TYPE reqType;
    bool lnfReturnStatus=false;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n",MODULE_NAME, __FUNCTION__, __LINE__ );

    while (true ) {
        pthread_mutex_lock(&mutexLAF);
        if(ret = pthread_cond_wait(&condLAF, &mutexLAF) == 0) {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "\n[%s:%s:%d] Starting the LAF Connect private SSID \n",MODULE_NAME, __FUNCTION__, __LINE__ );
            retVal=false;
            retVal=lastConnectedSSID(&savedWiFiConnList);
            if(retVal == false)
                RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%s:%d] Last connected ssid fetch failure \n",MODULE_NAME, __FUNCTION__, __LINE__ );
            gWifiLNFStatus=LNF_IN_PROGRESS;
            pthread_mutex_unlock(&mutexLAF);
            do
            {
                if((bAutoSwitchToPrivateEnabled) || (bSwitch2Private))
        	{
          	    bSwitch2Private=false;
          	    if(gWifiLNFStatus == CONNECTED_LNF)
          	    {
              		reqType = LAF_REQUEST_SWITCH_TO_PRIVATE;
          	    }
          	    else
          	    {
              		reqType = LAF_REQUEST_CONNECT_TO_PRIV_WIFI;
          	    }
            	}
            	else
            	{
               	    reqType = LAF_REQUEST_CONNECT_TO_LFSSID;
            	}
                if (true == bStopLNFWhileDisconnected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", __FUNCTION__, __LINE__ );
                    break;
                }

                if((gWifiAdopterStatus == WIFI_CONNECTED) && (false == isLAFCurrConnectedssid))
                {
                    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] connection status %d is LAF current ssid % \n", __FUNCTION__, __LINE__,gWifiAdopterStatus,isLAFCurrConnectedssid );
                    gWifiLNFStatus=CONNECTED_PRIVATE;
                    break;
                }

                bIsStopLNFWhileDisconnected=false;
                pthread_mutex_trylock(&mutexTriggerLAF);
                lnfReturnStatus=triggerLostFound(reqType);
                pthread_mutex_unlock(&mutexTriggerLAF);
                if (false == lnfReturnStatus)
//                else if (!triggerLostFound(LAF_REQUEST_CONNECT_TO_PRIV_WIFI) && counter < TOTAL_NO_OF_RETRY)
                {
                    if (true == bStopLNFWhileDisconnected)
                    {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", __FUNCTION__, __LINE__ );
                        break;
                    }
//                  counter++;
                    if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
                    {
                        retVal=connect_withSSID(ssidIndex, savedWiFiConnList.ssidSession.ssid, NET_WIFI_SECURITY_NONE, NULL, NULL, savedWiFiConnList.ssidSession.passphrase,true,NULL,NULL,NULL,NULL,WIFI_CON_PRIVATE);
                        if(false == retVal)
                        {
                            RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] connect with ssid %s  failed \n",MODULE_NAME, __FUNCTION__, __LINE__,savedWiFiConnList.ssidSession.ssid );
                        }
                    }
                    sleep(confProp.wifiProps.lnfRetryInSecs);
                }
                else
                {
                    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] pressed coming out of LNF since box is connected to private \n", __FUNCTION__, __LINE__ );
                    break;
                }
                sleep(1);
            }while ((gWifiLNFStatus != CONNECTED_PRIVATE) && (bAutoSwitchToPrivateEnabled));
            bIsStopLNFWhileDisconnected=true;
	    sleep(1);
     }
     else
     {
         pthread_mutex_unlock(&mutexLAF);
     }
     sleep(1);
   }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
void *lafConnThread(void* arg)
{
    int ret = 0;
    bool lnfReturnStatus=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    gWifiLNFStatus=LNF_IN_PROGRESS;
    while (false == bDeviceActivated) {
        bLnfActivationLoop=true;
        while(gWifiLNFStatus != CONNECTED_LNF)
        {
            bIsStopLNFWhileDisconnected=false;
            if (gWifiAdopterStatus == WIFI_CONNECTED)
            {
                if (false == isLAFCurrConnectedssid)
                {
                    gWifiLNFStatus=CONNECTED_PRIVATE;
                    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Connected through non LAF path\n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    bIsStopLNFWhileDisconnected=true;
                    bLnfActivationLoop=false;
                    return NULL;
                }
                break;
            }
            if (true == bStopLNFWhileDisconnected)
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                bIsStopLNFWhileDisconnected=true;
                bLnfActivationLoop=false;
                return NULL;
            }
            pthread_mutex_trylock(&mutexTriggerLAF);
            lnfReturnStatus=triggerLostFound(LAF_REQUEST_CONNECT_TO_LFSSID);
            pthread_mutex_unlock(&mutexTriggerLAF);
            if (false == lnfReturnStatus)
            {

                if (true == bStopLNFWhileDisconnected)
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
                    bIsStopLNFWhileDisconnected=true;
                    bLnfActivationLoop=false;
                    return NULL;
                }
                sleep(confProp.wifiProps.lnfRetryInSecs);
                continue;
            }
            else
            {
                gWifiLNFStatus=CONNECTED_LNF;
                RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Connection to LAF success\n", MODULE_NAME,__FUNCTION__, __LINE__ );
                break;
            }
        }
        bIsStopLNFWhileDisconnected=false;
        sleep(1);
        if((gWifiLNFStatus != CONNECTED_PRIVATE) && (false == isLAFCurrConnectedssid) && (gWifiAdopterStatus == WIFI_CONNECTED))
                    gWifiLNFStatus=CONNECTED_PRIVATE;
        if (true == bStopLNFWhileDisconnected)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] stopLNFWhileDisconnected pressed coming out of LNF \n", MODULE_NAME,__FUNCTION__, __LINE__ );
            bLnfActivationLoop=false;
            return NULL;
        }
//        getDeviceActivationState();
    }
    bLnfActivationLoop=false;
    if(gWifiLNFStatus == CONNECTED_LNF)
    {
        lnfConnectPrivCredentials();
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n",MODULE_NAME, __FUNCTION__, __LINE__ );
    pthread_exit(NULL);
}

void connectToLAF()
{
    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    pthread_attr_t attr;
    bool retVal=false;
    char lfssid[33] = {'\0'};
    if((getDeviceActivationState() == false) && (IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam))
    {
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        pthread_create(&lafConnectThread, &attr, lafConnThread, NULL);
    }
    else
    {
        retVal=lastConnectedSSID(&savedWiFiConnList);
        if (savedWiFiConnList.ssidSession.ssid[0] != '\0')
        {
            sleep(confProp.wifiProps.lnfStartInSecs);
        }

        /* If Device is activated and already connected to Private or any network,
            but defineltly not LF SSID. - No need to trigger LNF*/
        /* Here, 'getDeviceActivationState == true'*/
        laf_get_lfssid(lfssid);
        lastConnectedSSID(&savedWiFiConnList);

        if((! laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid)) &&  (WIFI_CONNECTED == get_WifiRadioStatus()))
        {
            RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Now connected to Private SSID \n", MODULE_NAME,__FUNCTION__, __LINE__ , savedWiFiConnList.ssidSession.ssid);
        }
        else {
            if(IARM_BUS_SYS_MODE_WAREHOUSE != sysModeParam)
            {
                pthread_mutex_lock(&mutexLAF);
                if(0 == pthread_cond_signal(&condLAF))
                {
                    RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%d] Signal to start LAF private SSID \n", __FUNCTION__, __LINE__ );
                }
                pthread_mutex_unlock(&mutexLAF);
            }
        }
    }
}


void lafConnectToPrivate()
{
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&lafConnectToPrivateThread, &attr, lafConnPrivThread, NULL);
}


bool getLAFssid()
{
    char lafSsid[SSID_SIZE];
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    memset(lafSsid, 0, SSID_SIZE);
    laf_get_lfssid(lafSsid);
    if(lafSsid)
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] lfssid is %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,lafSsid);
        return true;
    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] lfssid is empty \n", MODULE_NAME,__FUNCTION__, __LINE__);
        return false;
    }
}
#if 0
bool isLAFCurrConnectedssid()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(laf_is_lnfssid(savedWiFiConnList.ssidSession.ssid))
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] LAF is the current connected ssid \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        retVal=true;
    }
    else
    {
        retVal=false;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retVal;

}
#endif
bool getDeviceActivationState()
{
#ifndef ENABLE_XCAM_SUPPORT
    unsigned int count=0;
    std::string str;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(! confProp.wifiProps.authServerURL)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Authserver URL is NULL \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return bDeviceActivated;
    }
    str.assign(confProp.wifiProps.authServerURL,strlen(confProp.wifiProps.authServerURL));
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] Authserver URL is %s %s \n", MODULE_NAME,__FUNCTION__, __LINE__,confProp.wifiProps.authServerURL ,str.c_str());

    while(deviceID && !deviceID[0])
    {
        CurlObject authServiceURL(str);
        g_stpcpy(deviceID,authServiceURL.getCurlData());
        if ((!deviceID && deviceID[0]) || (count >= 3))
            break;
        sleep(5);
        count++;
    }
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] device id string is %s \n", MODULE_NAME,__FUNCTION__, __LINE__ ,deviceID);
    gchar **tokens = g_strsplit_set(deviceID,"{}:,\"", -1);
    guint tokLength = g_strv_length(tokens);
    guint loopvar=0;
    for (loopvar=0; loopvar<tokLength; loopvar++)
    {
        if (g_strrstr(g_strstrip(tokens[loopvar]), "deviceId"))
        {
            //"deviceId": "T00xxxxxxx" so omit 3 tokens ":" fromDeviceId
            if ((loopvar+3) < tokLength )
            {
                g_stpcpy(deviceID, g_strstrip(tokens[loopvar+3]));
                if(deviceID[0] != '\0')
                {
                    bDeviceActivated = true;
                }
            }
        }
      }
      for (loopvar=0; loopvar<tokLength; loopvar++)
      {
        if (g_strrstr(g_strstrip(tokens[loopvar]), "partnerId"))
        {
            //"deviceId": "T00xxxxxxx" so omit 3 tokens ":" fromDeviceId
            if ((loopvar+3) < tokLength )
            {
                g_stpcpy(partnerID, g_strstrip(tokens[loopvar+3]));
                if(partnerID[0] != '\0')
                {
                  RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] partner id is %s \n", MODULE_NAME,__FUNCTION__, __LINE__,partnerID);
                }
            }
        }
      }
    if(tokens)
        g_strfreev(tokens);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return bDeviceActivated;

}
bool isWifiConnected()
{
    if (get_WiFiStatusCode() == WIFI_CONNECTED)
    {
        return true;
    }
    return false;
}

void lnfConnectPrivCredentials()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(bLnfActivationLoop)
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Still in LnF Activation Loop so discarding getting private credentials \n",MODULE_NAME,__FUNCTION__, __LINE__ );
        return;
    }
    pthread_mutex_lock(&mutexLAF);
    if(0 == pthread_cond_signal(&condLAF))
    {
        RDK_LOG( RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] Signal to start LAF private SSID \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    pthread_mutex_unlock(&mutexLAF);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
#endif
bool setHostifParam (char *name, HostIf_ParamType_t type, void *value)
{
    IARM_Result_t ret = IARM_RESULT_IPCCORE_FAIL;
    bool retValue=false;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    HOSTIF_MsgData_t param = {0};

    if(NULL == name || value == NULL) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d]  Null pointer caught for Name/Value \n", MODULE_NAME,__FUNCTION__ ,__LINE__ );
        return retValue;
    }

    /* Initialize hostIf Set structure */
    strncpy(param.paramName, name, strlen(name)+1);
    param.reqType = HOSTIF_SET;

    switch (type) {
    case hostIf_StringType:
    case hostIf_DateTimeType:
        param.paramtype = hostIf_StringType;
        strcpy(param.paramValue, (char *) value);
        break;
    /*    case  hostIf_IntegerType:
        case hostIf_UnsignedIntType:
            put_int(param.paramValue, value);
            param.paramtype = hostIf_IntegerType;*/
    case hostIf_BooleanType:
        put_boolean(param.paramValue,*(bool*)value);
        param.paramtype = hostIf_BooleanType;
        break;
    default:
        break;
    }

    ret = IARM_Bus_Call(IARM_BUS_TR69HOSTIFMGR_NAME,
                        IARM_BUS_TR69HOSTIFMGR_API_SetParams,
                        (void *)&param,
                        sizeof(param));
    if(ret != IARM_RESULT_SUCCESS) {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] Error executing Set parameter call from tr69Client, error code \n",MODULE_NAME,__FUNCTION__,__LINE__ );
    }
    else
    {
        RDK_LOG(RDK_LOG_DEBUG,LOG_NMGR,"[%s:%s:%d] sent iarm message to set hostif parameter\n",MODULE_NAME,__FUNCTION__,__LINE__ );
        retValue=true;
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return retValue;
}
void put_boolean(char *ptr, bool val)
{
    bool *tmp = (bool *)ptr;
    *tmp = val;
}

bool storeMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    IARM_BUS_MFRLIB_API_WIFI_Credentials_Param_t param = {0};
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    retVal=lastConnectedSSID(&savedWiFiConnList);
    if(retVal == false)
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "\n[%s:%s:%d] Last connected ssid fetch failure \n", MODULE_NAME,__FUNCTION__, __LINE__ );
        return retVal;
    }
    else
    {
        retVal=false;
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] fetched ssid details  %s \n",MODULE_NAME,__FUNCTION__,__LINE__,savedWiFiConnList.ssidSession.ssid);
    }
    param.requestType=WIFI_GET_CREDENTIALS;
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        if((g_strcmp0(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid) == 0 ) && (g_strcmp0(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase) == 0))
        {
            RDK_LOG(RDK_LOG_INFO,LOG_NMGR,"[%s:%s:%d] Same ssid info not storing it stored ssid %s new ssid %s \n",MODULE_NAME,__FUNCTION__,__LINE__,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
            return true;
        }
        else
        {
            RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] ssid info is different continue to store ssid \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        }
    }
    memset(&param,0,sizeof(param));
    param.requestType=WIFI_SET_CREDENTIALS;
    strcpy(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
    strcpy(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase);
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in storing wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        memset(&param,0,sizeof(param));
        param.requestType=WIFI_GET_CREDENTIALS;
        if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_Credentials,(void *)&param,sizeof(param)))
        {
            RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in retrieving the stored wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
            if((g_strcmp0(param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid) == 0 ) && (g_strcmp0(param.wifiCredentials.cPassword,savedWiFiConnList.ssidSession.passphrase) == 0 ))
            {
                retVal=true;
                RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] Successfully stored the credentails and verified stored ssid %s current ssid %s \n",MODULE_NAME,__FUNCTION__,__LINE__,param.wifiCredentials.cSSID,savedWiFiConnList.ssidSession.ssid);
            }
            else
            {
                RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] failure in storing  wifi credentials   \n",MODULE_NAME,__FUNCTION__,__LINE__ );
            }

        }
        else
        {
            RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] IARM error in retrieving stored wifi credentials mfr error code %d \n",MODULE_NAME,__FUNCTION__,__LINE__,param.returnVal );
        }
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] IARM error in storing wifi credentials mfr error code \n",MODULE_NAME,__FUNCTION__,__LINE__ ,param.returnVal);
    }
#endif
    return retVal;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}


bool eraseMfrWifiCredentials(void)
{
    bool retVal=false;
#ifdef USE_RDK_WIFI_HAL
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(IARM_RESULT_SUCCESS == IARM_Bus_Call(IARM_BUS_MFRLIB_NAME,IARM_BUS_MFRLIB_API_WIFI_EraseAllData,0,0))
    {
        RDK_LOG(RDK_LOG_TRACE1,LOG_NMGR,"[%s:%s:%d] IARM success in erasing wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
        retVal=true;
    }
    else
    {
        RDK_LOG(RDK_LOG_ERROR,LOG_NMGR,"[%s:%s:%d] failure in erasing  wifi credentials \n",MODULE_NAME,__FUNCTION__,__LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#endif
    return retVal;
}


void getEndPointInfo(WiFi_EndPoint_Diag_Params *endPointInfo)
{
    bool ret = true;
    int radioIndex = 1;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
#ifdef USE_RDK_WIFI_HAL
    wifi_sta_stats_t stats;
    memset(&stats, '\0', sizeof(stats));

    wifi_getStats(radioIndex, &stats);

    bool enable = (WIFI_CONNECTED == get_WiFiStatusCode())? true: false;
    if (enable)
    {
        strncpy((char *)endPointInfo->status, "Enabled", (size_t)BUFF_LENGTH_64);
        endPointInfo->enable = enable;
        strncpy((char *)endPointInfo->SSIDReference, (const char *)stats.sta_SSID, (size_t)BUFF_LENGTH_64);

        endPointInfo->stats.signalStrength = stats.sta_RSSI;
        endPointInfo->stats.retransmissions = stats.sta_Retransmissions;
        endPointInfo->stats.lastDataDownlinkRate = stats.sta_LastDataDownlinkRate;
        endPointInfo->stats.lastDataUplinkRate = stats.sta_LastDataUplinkRate;
        /*endPointInfo->security.modesSupported = ;*/
    }
    else
    {
        strncpy((char *)endPointInfo->status, "Disabled", (size_t)BUFF_LENGTH_64);
        endPointInfo->enable = enable;
    }

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] \n Profile : \"EndPoint.1.\": \n \
    		[Enable : \"%d\"| Status : \"%s\" | SSIDReference : \"%s\" ] \n",
            MODULE_NAME,__FUNCTION__, __LINE__, endPointInfo->enable, endPointInfo->status, endPointInfo->SSIDReference);

    RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "[%s:%s:%d] \n Profile : \"EndPoint.1.Stats.\": \n \
    		[SignalStrength : \"%d\"| Retransmissions : \"%d\" | LastDataUplinkRate : \"%d\" | LastDataDownlinkRate : \" %d\" ] \n",
            MODULE_NAME,__FUNCTION__, __LINE__, endPointInfo->stats.signalStrength, endPointInfo->stats.retransmissions,
            endPointInfo->stats.lastDataDownlinkRate, endPointInfo->stats.lastDataUplinkRate);
#endif
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}


#ifdef USE_RDK_WIFI_HAL

bool getRadioStats(WiFi_Radio_Stats_Diag_Params *params)
{
    int radioIndex=1;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );

    wifi_radioTrafficStats_t *trafficStats =(wifi_radioTrafficStats_t *) malloc(sizeof(wifi_radioTrafficStats_t));
    if(trafficStats == NULL)
    {
        RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s]Malloc Memory allocation failure\n", MODULE_NAME,__FUNCTION__);
        return false;
    }
    printf("malloc allocated = %d ", malloc_usable_size(trafficStats));
    if (wifi_getRadioTrafficStats(radioIndex, trafficStats) == RETURN_OK) {
        params->bytesSent = trafficStats->radio_BytesSent;
        params->bytesReceived  = trafficStats->radio_BytesReceived;
        params->packetsSent = trafficStats->radio_PacketsSent;
        params->packetsReceived = trafficStats->radio_PacketsReceived;
        params->errorsSent = trafficStats->radio_ErrorsSent;
        params->errorsReceived = trafficStats->radio_ErrorsReceived;
        params->discardPacketsSent  = trafficStats->radio_DiscardPacketsSent;
        params->discardPacketsReceived  = trafficStats->radio_DiscardPacketsReceived;
        params->plcErrorCount = trafficStats->radio_DiscardPacketsReceived;
        params->fcsErrorCount = trafficStats->radio_FCSErrorCount;
        params->invalidMACCount = trafficStats->radio_InvalidMACCount;
        params->packetsOtherReceived = trafficStats->radio_PacketsOtherReceived;
        params->noiseFloor = trafficStats->radio_NoiseFloor;

    }
    else
    {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%s:%d] HAL wifi_getRadioTrafficStats FAILURE \n", MODULE_NAME,__FUNCTION__, __LINE__);
    }
    free(trafficStats);

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    return true;
}

void logs_Period1_Params()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if(NULL != wifiParams_Tele_Period1.paramlist)
    {
        WiFi_EndPoint_Diag_Params endPointInfo;
        getEndPointInfo(&endPointInfo);
        WiFi_Radio_Stats_Diag_Params params;
        getRadioStats(&params);

        GList *iter = g_list_first(wifiParams_Tele_Period1.paramlist);

        while(iter)
        {
            /* Device.WiFi.EndPoint.{i}.Stats.LastDataDownlinkRate*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "LastDataDownlinkRate")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.lastDataDownlinkRate);
            }
            /* Device.WiFi.EndPoint.{i}.Stats.LastDataUplinkRate */
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "LastDataUplinkRate ")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.lastDataUplinkRate);
            }

            /* Device.WiFi.EndPoint.{i}.Stats.Retransmissions*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Retransmissions")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, endPointInfo.stats.retransmissions);
            }

            /*Device.WiFi.Endpoint.{i}.Stats.SignalStrength*/
            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "SignalStrength")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n",  (char *)iter->data, endPointInfo.stats.signalStrength);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Channel")) {
                unsigned long output_ulong = 0;
                int radioIndex = 1;
                if (wifi_getRadioChannel(radioIndex, &output_ulong) == RETURN_OK) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n", (char *)iter->data , output_ulong);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.FCSErrorCount")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.fcsErrorCount);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.Noise")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.noiseFloor);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "TransmitPower"))
            {
                INT output_INT = 0;
                int radioIndex = 1;
                if (wifi_getRadioTransmitPower( radioIndex,  &output_INT) == RETURN_OK) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n", (char *)iter->data , output_INT);
                }
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsReceived")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.errorsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.ErrorsSent")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%u \n",  (char *)iter->data, params.errorsSent);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsReceived")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, params.packetsReceived);
            }

            if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Stats.PacketsSent")) {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%lu \n",  (char *)iter->data, params.packetsSent);
            }

            iter = g_list_next(iter);
        }

    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}


void logs_Period2_Params()
{
    static bool print_flag = true;
    time_t start_t;
    static time_t lastExec_t;
    double diff_t;

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );

    time(&start_t);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] print_flag : %d\n", __FUNCTION__, __LINE__, print_flag);
    if(print_flag)
    {
        if(NULL != wifiParams_Tele_Period2.paramlist)
        {
            char bssid_string[BUFF_MAX];
            int ssidIndex = 1;
            memset(bssid_string,0, BUFF_MAX);
            char output_string[BUFF_MAX];
            wifi_getBaseBSSID(ssidIndex, bssid_string);

            WiFiConnectionStatus currSsidInfo;
            memset(&currSsidInfo, '\0', sizeof(currSsidInfo));
            get_CurrentSsidInfo(&currSsidInfo);

            GList *iter = g_list_first(wifiParams_Tele_Period2.paramlist);
            while(iter)
            {
                RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d]%s \n",__FUNCTION__, __LINE__, (char *)iter->data);
                /* Device.WiFi.Endpoint.{i}.NumberOfEntries*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "NumberOfEntries")) {
                    int noOfEndpoint = 1;
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%d \n",  (char *)iter->data, noOfEndpoint);
                }

                if((g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.Profile.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.SSID"))
                        || (g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.EndPoint.1.SSIDReference")))
                {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, currSsidInfo.ssidSession.ssid);
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "BSSID")) {
                    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, bssid_string);
                }

                /* Device.WiFi.EndPoint.{i}.Profile.{i}.SSID*/
                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Name")) {
                    memset(output_string,0, BUFF_MAX);
                    if(wifi_getSSIDName(ssidIndex, output_string) == RETURN_OK) {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, output_string);
                    }
                }

                if(g_strrstr ((const gchar *)iter->data, (const gchar *) "Device.WiFi.SSID.1.MACAddress")) {
                    if(gWifiMacAddress[0] != '\0') {
                        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "%s:%s \n",  (char *)iter->data, gWifiMacAddress);
                    }
                }

                iter = g_list_next(iter);
            }
        }
    }
    diff_t = difftime(start_t, lastExec_t);

    if (diff_t >= wifiParams_Tele_Period2.timePeriod) {
        time(&lastExec_t);
        print_flag = true;
    }
    else
    {
        print_flag = false;
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}


/* get lfat from auth service */
int laf_get_lfat(laf_lfat_t *lfat)
{
  long          http_err_code;
  cJSON         *response;
  std::string   str;
  gchar         *retStr;

  str.assign(confProp.wifiProps.getLfatUrl,strlen(confProp.wifiProps.getLfatUrl));

  CurlObject getLfatUrl(str);
  retStr  = getLfatUrl.getCurlData();
  http_err_code = getLfatUrl.gethttpcode();

  if((http_err_code != 200) || (strlen(retStr ) <= 1)){
    RDK_LOG(RDK_LOG_ERROR, LOG_NMGR, "get lfat from auth service failed with error code %d ", http_err_code);
    if(http_err_code == 0)
      http_err_code = 404;
    return http_err_code;
  }

  /* parse json response */
  RDK_LOG(RDK_LOG_DEBUG, LOG_NMGR, "Response from authservice to get lfat %s", retStr);
  response = cJSON_Parse(retStr);
  RDK_ASSERT_NOT_NULL(response);
  RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "version"));
  strcpy(lfat->version, cJSON_GetObjectItem(response, "version")->valuestring);
  RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "expires"));
  lfat->ttl = cJSON_GetObjectItem(response, "expires")->valueint;
  RDK_ASSERT_NOT_NULL(cJSON_GetObjectItem(response, "LFAT"));
  lfat->len = strlen(cJSON_GetObjectItem(response, "LFAT")->valuestring);
  lfat->token = (char *) malloc(lfat->len+1);
  if(lfat->token == NULL)
    return ENOMEM;
  strcpy(lfat->token, cJSON_GetObjectItem(response, "LFAT")->valuestring);
  lfat->token[lfat->len] = '\0';
  cJSON_Delete(response);
  return 0;
}


/* store lfat with auth service */
int laf_set_lfat(laf_lfat_t* const lfat)
{
    cJSON         *req_payload;
    char          *data;
    long          http_err_code;
    std::string   str;
    gchar         *retStr;

    str.assign(confProp.wifiProps.setLfatUrl,strlen(confProp.wifiProps.setLfatUrl));
    req_payload = cJSON_CreateObject();
    RDK_ASSERT_NOT_NULL(req_payload);
    cJSON_AddStringToObject(req_payload, "version", lfat->version);
    cJSON_AddNumberToObject(req_payload, "expires", lfat->ttl);
    cJSON_AddStringToObject(req_payload, "lfat", lfat->token);

    data = cJSON_Print(req_payload);
    RDK_ASSERT_NOT_NULL(data);
    CurlObject setLfatUrl(str,data);
    retStr  = setLfatUrl.getCurlData();
    http_err_code = setLfatUrl.gethttpcode();

    if(http_err_code != 200) {
        RDK_LOG(RDK_LOG_ERROR, LOG_NMGR,  "set lfat to auth service failed with error code %d message %s", http_err_code);
        cJSON_Delete(req_payload);
        if(data)
            free(data);
        if(http_err_code == 0)
            http_err_code = 404;
        return http_err_code;
    }
  cJSON_Delete(req_payload);
  if(data)
    free(data);

  return 0;
}


#endif


bool addSwitchToPrivateResults(int lnfError,char *currTime)
{
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData = g_new(WiFiLnfSwitchPrivateResults_t,1);
  WiFiLnfSwitchPrivateResultsData->lnfError = lnfError;
  strcpy((char *)WiFiLnfSwitchPrivateResultsData->currTime,currTime);
  lstLnfPvtResults=g_list_append(lstLnfPvtResults,WiFiLnfSwitchPrivateResultsData);
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Time = %s lnf error = %d length of list = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError,g_list_length(lstLnfPvtResults));
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}

bool convertSwitchToPrivateResultsToJson(char *buffer)
{
  cJSON *rootObj = NULL, *array_element = NULL, *array_obj = NULL;
  UINT output_array_size = 0;
  INT index;
  char *out = NULL;
  char privateResultsLength=0;
  WiFiLnfSwitchPrivateResults_t *WiFiLnfSwitchPrivateResultsData=NULL;
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  rootObj = cJSON_CreateObject();
  if(NULL == rootObj) {
    RDK_LOG( RDK_LOG_ERROR, LOG_NMGR,"[%s:%s:%d] \'Failed to create root json object.\n",  MODULE_NAME,__FUNCTION__, __LINE__);
    return false;
  }
  cJSON_AddItemToObject(rootObj, "results", array_obj=cJSON_CreateArray());
  GList *element = g_list_first(lstLnfPvtResults);
  privateResultsLength=g_list_length(lstLnfPvtResults);

  while ((element)&&(privateResultsLength > 0))
  {
    WiFiLnfSwitchPrivateResultsData = (WiFiLnfSwitchPrivateResults_t*)element->data;
    cJSON_AddItemToArray(array_obj,array_element=cJSON_CreateObject());
    cJSON_AddStringToObject(array_element, "timestamp",(const char*)WiFiLnfSwitchPrivateResultsData->currTime);
    cJSON_AddNumberToObject(array_element, "result", WiFiLnfSwitchPrivateResultsData->lnfError);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Time = %s lnf error = %d \n", MODULE_NAME,__FUNCTION__, __LINE__,WiFiLnfSwitchPrivateResultsData->currTime,WiFiLnfSwitchPrivateResultsData->lnfError );
    element = g_list_next(element);
    privateResultsLength--;
  }
  out = cJSON_PrintUnformatted(rootObj);

  if(out) {
    strncpy(buffer, out, strlen(out)+1);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Buffer = %s \n", MODULE_NAME,__FUNCTION__, __LINE__,buffer );
  }
  if(rootObj) {
    cJSON_Delete(rootObj);
  }
  if(out) free(out);
  RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
  return true;
}
bool clearSwitchToPrivateResults()
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Enter\n", MODULE_NAME,__FUNCTION__, __LINE__ );
    if((lstLnfPvtResults) && (g_list_length(lstLnfPvtResults) != 0))
    {
	g_list_free_full(lstLnfPvtResults,g_free);
    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%s:%d] switch to private results list is empty \n", MODULE_NAME,__FUNCTION__, __LINE__ );
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%s:%d] Exit\n", MODULE_NAME,__FUNCTION__, __LINE__ );
}
#endif
bool shutdownWifi()
{
    bool result=true;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    if ((wifiStatusMonitorThread) && ( pthread_cancel(wifiStatusMonitorThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] wifiStatusMonitorThread cancel failed! \n",__FUNCTION__, __LINE__);
        result=false;
    }
#ifdef ENABLE_LOST_FOUND
    if ((lafConnectThread) && (pthread_cancel(lafConnectThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] lafConnectThread cancel failed! \n", __FUNCTION__, __LINE__);
        result=false;
    }
    if ((lafConnectToPrivateThread) && (pthread_cancel(lafConnectToPrivateThread) == -1 )) {
        RDK_LOG( RDK_LOG_ERROR, LOG_NMGR, "[%s:%d] lafConnectToPrivateThread failed! \n",__FUNCTION__, __LINE__);
        result=false;
    }
#endif
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Start WiFi Uninitialization \n", __FUNCTION__, __LINE__);
#ifdef USE_RDK_WIFI_HAL
    wifi_uninit();
#endif
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d]  WiFi Uninitialization done \n", __FUNCTION__, __LINE__);
    gWifiAdopterStatus = WIFI_UNINSTALLED;
#ifdef ENABLE_LOST_FOUND
    condLAF = PTHREAD_COND_INITIALIZER;
    mutexLAF = PTHREAD_MUTEX_INITIALIZER;
    mutexTriggerLAF = PTHREAD_MUTEX_INITIALIZER;
    gWifiLNFStatus = LNF_UNITIALIZED;
    bLNFConnect=false;
    isLAFCurrConnectedssid=false;
#endif
    wifiStatusLock = PTHREAD_MUTEX_INITIALIZER;
    condGo = PTHREAD_COND_INITIALIZER;
    mutexGo = PTHREAD_MUTEX_INITIALIZER;
    memset(&gSsidList,0,sizeof gSsidList);
    memset(&savedWiFiConnList,0,sizeof savedWiFiConnList);
    memset(&wifiConnData,0,sizeof wifiConnData);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
    return result;
}
