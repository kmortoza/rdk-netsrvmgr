/*
 * ============================================================================
 * COMCAST C O N F I D E N T I A L AND PROPRIETARY
 * ============================================================================
 * This file (and its contents) are the intellectual property of Comcast.  It may
 * not be used, copied, distributed or otherwise  disclosed in whole or in part
 * without the express written permission of Comcast.
 * ============================================================================
 * Copyright (c) 2015 Comcast. All rights reserved.
 * ============================================================================
 */

#ifndef _WIFIMGR_IARMIF_H
#define _WIFIMGR_IARMIF_H

//#include "libIBus.h"
#include "libIARM.h"

#define IARM_BUS_NM_MGR_NAME "NETWORK_MGR"

#define IARM_BUS_WIFI_MGR_API_getAvailableSSIDs     "getAvailableSSIDs"      /*!< Retrives the array of strings representing ssids*/
#define IARM_BUS_WIFI_MGR_API_getCurrentState       "getCurrentState"        /*!< Retrives the current state*/
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Returns the paired ssid as a string*/
#define IARM_BUS_WIFI_MGR_API_setEnabled            "setEnabled"             /*!< Enable the wifi adapter on the box*/
#define IARM_BUS_WIFI_MGR_API_connect               "connect"                /*!< Connect with given or saved ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_saveSSID              "saveSSID"               /*!< Save the ssid and passphrase */
#define IARM_BUS_WIFI_MGR_API_clearSSID             "clearSSID"              /*!< Clear the given ssid*/
#define IARM_BUS_WIFI_MGR_API_getPairedSSID         "getPairedSSID"          /*!< Get the paired SSID */
#define IARM_BUS_WIFI_MGR_API_isPaired              "isPaired"               /*!< Retrieve the paired status*/

/*! Event Data associated with WiFi Manager */
typedef enum _WiFiStatusCode_t {
    WIFI_UNINSTALLED,
    WIFI_DISABLED,
    WIFI_DISCONNECTED,
    WIFI_PAIRING,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED
} WiFiStatusCode_t;

typedef enum _WiFiErrorCode_t {
    WIFI_SSID_CHANGED,
    WIFI_CONNECTION_LOST,
    WIFI_CONNECTION_FAILED,
    WIFI_CONNECTION_INTERRUPTED,
    WIFI_INVALID_CREDENTIALS,
    WIFI_NO_SSID,
    WIFI_UNKNOWN
} WiFiErrorCode_t;

typedef struct _IARM_STATUSCHANGED_DATA {
} IARM_StatusChanged_data;

/*! Events published from WiFi Manager */
typedef enum _IARM_Bus_NMgr_WiFi_EventId_t {
    IARM_BUS_WIFI_MGR_EVENT_onWIFIStateChanged = 1,
    IARM_BUS_WIFI_MGR_EVENT_onError,
    IARM_BUS_WIFI_MGR_EVENT_onSSIDsChanged
} IARM_Bus_NMgr_WiFi_EventId_t;

typedef struct _NM_WiFi_getAvailableSSID_ARGS
{
    char ssid[256];
    int security;
    int signalstrength;
    double frequency;
} NM_WiFi_getAvailableSSID_Args;

typedef struct _NM_WiFi_CONNECT_ARGS
{
    char ssid[256];
    char passphrase[256];
    bool ret;
} NM_WiFi_Connect_Args;

typedef struct _NM_WiFi_SAVESSID_ARGS
{
    char ssid[256];
    char passphrase[256];
    bool ret;
} NM_WiFi_SaveSSID_Args;

/*
typedef struct _NM_getAvailableSSID_ARGS
{
    std::string ssid;
    int security;
    int signalstrength;
    double frequency;
} NM_getAvailableSSID__Args;

typedef struct _NM_CONNECT_ARGS
{
    std::string ssid;
    std::string passphrase;
    bool ret;
} NM_Connect_Args;

typedef struct _NM_SAVESSID_ARGS
{
    std::string ssid;
    std::string passphrase;
    bool ret;
} NM_SaveSSID_Args;
*/
#endif
