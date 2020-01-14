/*
 * If not stated otherwise in this file or this component's Licenses.txt file the
 * following copyright and licenses apply:
 *
 * Copyright 2016 RDK Management
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <iostream>
#ifdef ENABLE_IARM
#include "libIBus.h"
#include "libIBusDaemon.h"
#include "wifiSrvMgrIarmIf.h"
#endif
#include "NetworkMgrMain.h"

#define NM_MGR_WIFI_CLIENT "NetworkMgrWiFiClientApps"

enum {
    Test_getAvailableSSIDs =1,
    Test_getCurrentState = 2,
    Test_setEnabled = 3,
    Test_getPairedSSID =4,
    Test_connect = 5,
    Test_initiateWPSPairing = 6,
    Test_saveSSID = 7,
    Test_clearSSID =8,
    Test_isPaired =9,
    Test_getRadioProps =10,
    Test_getRadioStatsProps =11,
    Test_getSSIDProps =12,
#ifdef ENABLE_LOST_FOUND
    Test_getLAFState = 13,
#endif
    Test_getConnectedSSID = 14,
    Test_getEndPointProps = 15,
    Test_getAvailableSSIDsWithName = 16,
    Test_getAvailableSSIDsIncr = 17,
    Test_stopProgressiveScanning = 18,
    Test_disconnectSSID = 19,
    Test_cancelWPSPairing = 20,
    Test_Max_Api,
};

static void WIFI_MGR_API_getAvailableSSIDsWithName()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    char ssid[64] = {'\0'};
    double freq ;
    int timeout = 10000;
    IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    printf("[%s] Entering...\r\n", __FUNCTION__);
    printf("\nEnter SSID to get the info :");
    scanf("%s",ssid);
    printf("\nEnter the band to scan 2.4 / 5 / 0 :");
    scanf("%lf",&freq);
    strcpy(param.SSID,ssid);
    param.frequency = freq;
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SpecificSsidList_Param_t),timeout);
    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status)
    {
        printf("[\n[%s (with Message size: %d)]:\n %s \n '. \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, param.curSsids.jdataLen, param.curSsids.jdata);
    }
    else
    {
       printf("%s : \"Empty\" with message size: \'%d\'). \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsWithName, param.curSsids.jdataLen);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}
static void WIFI_MGR_API_stopProgressiveScanning()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME,IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_stopProgressiveWifiScanning, ((retVal == IARM_RESULT_SUCCESS)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_disconnectSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_disconnectSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_disconnectSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_cancelWPSPairing()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_cancelWPSPairing, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_cancelWPSPairing, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif

}

static void WIFI_MGR_API_getAvailableSSIDsIncr()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_SsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    int timeout = 5000;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SsidList_Param_t),timeout);

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDsAsyncIncr, ((retVal == IARM_RESULT_SUCCESS)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getAvailableSSIDs()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_SsidList_Param_t param;
    memset(&param, 0, sizeof(param));
    int timeout = 10000;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    retVal = IARM_Bus_Call_with_IPCTimeout(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, (void *)&param, sizeof(IARM_Bus_WiFiSrvMgr_SsidList_Param_t),timeout);

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf("[\n[%s (with Message size: %d)]:\n %s \n '. \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, param.curSsids.jdataLen, param.curSsids.jdata);
    }
    else {
        printf("%s : \"Empty\" with message size: \'%d\'). \n", IARM_BUS_WIFI_MGR_API_getAvailableSSIDs, param.curSsids.jdataLen);
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getCurrentState()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getCurrentState, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getCurrentState, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf(" \"getCurrentState\" : %d \n", param.data.wifiStatus );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

#ifdef ENABLE_LOST_FOUND
static void WIFI_MGR_API_getLAFState()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getLNFState, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getLNFState, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    if(retVal == IARM_RESULT_SUCCESS && param.status) {
        printf(" \"getLAFState\" : %d \n", param.data.wifiLNFStatus );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}
#endif
static void WIFI_MGR_API_setEnabled()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    param.data.setwifiadapter.enable = true;
    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_setEnabled, (void *)&param, sizeof(param));

    if(retVal == IARM_RESULT_SUCCESS)    {
        printf("[%s] to \'%d\'.  \n", IARM_BUS_WIFI_MGR_API_setEnabled, param.data.setwifiadapter.enable );
    }
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getPairedSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    char *ssid = NULL;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getPairedSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getPairedSSID, ((param.status)?"true":"false"));
    printf("\n***********************************\n");

    ssid = param.data.getPairedSSID.ssid;
    if(retVal == IARM_RESULT_SUCCESS) {
        printf("\nSSID: %s \n",  (ssid)?ssid:"No ssid is assigned.");
    }

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_connect()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    std::string ssid, passphrase, ans;
    int securitymode;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\"", IARM_BUS_WIFI_MGR_API_connect);
    printf("\n***********************************\n");

    printf( "Would like to connect using ssid & passphrase. (Y/N)?" );
    std::cin >> ans;

    if(!strcasecmp(ans.c_str(),"Y"))
    {
        std::cout<< "	Enter ssid 	: 	";
        std::cin >> ssid;
        std::cout <<"	Enter passphrase	:	" ;
        std::cin >> passphrase;
        std::cout <<"	Enter securitymode	:	" ;
        std::cin >> securitymode;
        strcpy (param.data.connect.ssid, ssid.c_str());
        strcpy (param.data.connect.passphrase, passphrase.c_str());
        param.data.connect.security_mode=(SsidSecurity)securitymode;
	printf("\n ssid = %s  passphrase = %s  security mode = %d",param.data.connect.ssid,param.data.connect.passphrase,param.data.connect.security_mode);
    }
    else {
        printf( "\nNot selected any ssid & passphrase. so, connect\n" );
    }

    retVal = IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_connect, (void *)&param, sizeof(param));

    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_connect, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Success":"Failure"));

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_initiateWPSPairing()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;
    printf("[%s] Entering...\r\n", __FUNCTION__);

    IARM_Bus_Call( IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_initiateWPSPairing, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_initiateWPSPairing, ((param.status)?"true":"false"));
    printf("\n***********************************\n");
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_saveSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    std::string ssid, passphrase, ans;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);
    memset(&param, 0, sizeof(param));
    printf("\n***********************************");
    printf("\n \"%s\"", IARM_BUS_WIFI_MGR_API_saveSSID);
    printf("\n***********************************\n");

    printf( "This would saves the ssid and passphrase for future sessions.\n" );
    std::cout << "Enter ssid :" ;
    std::cin >>ssid;
    std::cout << "\nEnter passphrase	:" ;
    std::cin >> passphrase;
    strcpy (param.data.connect.ssid, ssid.c_str());
    strcpy (param.data.connect.passphrase, passphrase.c_str());

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_saveSSID, (void *)&param, sizeof(param));

    printf("\n%s : %s \n", IARM_BUS_WIFI_MGR_API_saveSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"Successfully saved the SSID":"failed to save SSID"));
    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_clearSSID()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_clearSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_clearSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_isPaired()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_isPaired, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_isPaired, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    printf("%s \n", (param.data.isPaired)?"Paied.":"Not paired.");

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getConnectedSSID() {
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_Bus_WiFiSrvMgr_Param_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getConnectedSSID, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getConnectedSSID, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("Connected SSID info: \n \
	    	\tSSID: \"%s\"\n \
			\tBSSID : \"%s\"\n \
			\tPhyRate : \"%f\"\n \
			\tNoise : \"%f\" \n \
                        \tBand : \"%s\"\n \
			\tSignalStrength(rssi) : \"%f\" \n \
                        \tAvgSignalStrength(avgRssi): \"%f\" \n \
                        \tFrequency : \"%d\"\n \
                        \tSecurityMode : \"%s\"\n  ",
           param.data.getConnectedSSID.ssid, \
           param.data.getConnectedSSID.bssid, \
           param.data.getConnectedSSID.rate, \
           param.data.getConnectedSSID.noise, \
           param.data.getConnectedSSID.band, \
           param.data.getConnectedSSID.signalStrength, \
           param.data.getConnectedSSID.avgSignalStrength, \
           param.data.getConnectedSSID.frequency, \
           param.data.getConnectedSSID.securityMode);

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getEndPointProps() 
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getEndPointProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getEndPointProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");

    printf("\n Profile : \"EndPoint.1.\": \n \
     		[Enable : \"%d\"| Status : \"%s\" | SSIDReference : \"%s\" ] \n",
           param.data.endPointInfo.enable, param.data.endPointInfo.status, param.data.endPointInfo.SSIDReference);

    printf(" \n Profile : \"EndPoint.1.Stats.\": \n \
     		[SignalStrength : \"%d\"| Retransmissions : \"%d\" | LastDataUplinkRate : \"%d\" | LastDataDownlinkRate : \" %d\" ] \n",
           param.data.endPointInfo.stats.signalStrength, param.data.endPointInfo.stats.retransmissions,
           param.data.endPointInfo.stats.lastDataDownlinkRate, param.data.endPointInfo.stats.lastDataUplinkRate);

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getSSIDProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getSSIDProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getSSIDProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "enable : " << param.data.ssid.params.enable << std::endl;
    std::cout << "status : " << param.data.ssid.params.status << std::endl;
    std::cout << "name : " << param.data.ssid.params.name << std::endl;
    std::cout << "bssid : " << param.data.ssid.params.bssid << std::endl;
    std::cout << "macaddr : " <<  param.data.ssid.params.macaddr << std::endl;
    std::cout << "ssid : " << param.data.ssid.params.ssid << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


static void WIFI_MGR_API_getRadioProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getRadioProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getRadioProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "enable : " << param.data.radio.params.enable << std::endl;
    std::cout << "status : " << param.data.radio.params.status << std::endl;
    std::cout << "name : " << param.data.radio.params.name << std::endl;
    std::cout << "maxBitRate : " << param.data.radio.params.maxBitRate << std::endl;
    std::cout << "supportedFrequencyBands : " <<  param.data.radio.params.supportedFrequencyBands << std::endl;
    std::cout << "operatingFrequencyBand : "<< param.data.radio.params.operatingFrequencyBand << std::endl;
    std::cout << "autoChannelEnable : "<< param.data.radio.params.autoChannelEnable << std::endl;
    std::cout << "autoChannelRefreshPeriod : "<< param.data.radio.params.autoChannelRefreshPeriod << std::endl;
    std::cout << "autoChannelSupported : "<< param.data.radio.params.autoChannelSupported << std::endl;
    std::cout << "channelsInUse : " << param.data.radio.params.channelsInUse << std::endl;
    std::cout << "channel : " << param.data.radio.params.channel << std::endl;
    std::cout << "extensionChannel : " << param.data.radio.params.extensionChannel << std::endl;
    std::cout << "guardInterval : " << param.data.radio.params.guardInterval << std::endl;
    std::cout << "mcs : " << param.data.radio.params.mcs << std::endl;
    std::cout << "operatingChannelBandwidth : " << param.data.radio.params.operatingChannelBandwidth << std::endl;
    std::cout << "operatingStandards : " << param.data.radio.params.operatingStandards << std::endl;
    std::cout << "possibleChannels : " << param.data.radio.params.possibleChannels << std::endl;
    std::cout << "regulatoryDomain : " << param.data.radio.params.regulatoryDomain << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}

static void WIFI_MGR_API_getRadioStatsProps()
{
#ifdef ENABLE_IARM
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
    IARM_BUS_WiFi_DiagsPropParam_t param;

    printf("[%s] Entering...\r\n", __FUNCTION__);

    memset(&param, 0, sizeof(param));

    retVal = IARM_Bus_Call(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_WIFI_MGR_API_getRadioStatsProps, (void *)&param, sizeof(param));

    printf("\n***********************************");
    printf("\n \"%s\", status: \"%s\"", IARM_BUS_WIFI_MGR_API_getRadioStatsProps, ((retVal == IARM_RESULT_SUCCESS && param.status)?"true":"false"));
    printf("\n***********************************\n");
    std::cout << "BytesSent : " << param.data.radio_stats.params.bytesSent << std::endl;
    std::cout << "BytesReceived : " << param.data.radio_stats.params.bytesReceived << std::endl;
    std::cout << "PacketsSent : " << param.data.radio_stats.params.packetsSent << std::endl;
    std::cout << "PacketsReceived : " << param.data.radio_stats.params.packetsReceived << std::endl;
    std::cout << "ErrorsSent : " <<  param.data.radio_stats.params.errorsSent << std::endl;
    std::cout << "ErrorsReceived : "<< param.data.radio_stats.params.errorsReceived << std::endl;
    std::cout << "DiscardPacketsSent : " << param.data.radio_stats.params.discardPacketsSent << std::endl;
    std::cout << "DiscardPacketsReceived : " << param.data.radio_stats.params.discardPacketsReceived << std::endl;
    std::cout << "PLCPErrorCount : " << param.data.radio_stats.params.plcErrorCount << std::endl;
    std::cout << "FCSErrorCount : " << param.data.radio_stats.params.fcsErrorCount << std::endl;
    std::cout << "InvalidMACCount : " << param.data.radio_stats.params.invalidMACCount << std::endl;
    std::cout << "PacketsOtherReceived : " << param.data.radio_stats.params.packetsOtherReceived << std::endl;

    printf("[%s] Exiting..\r\n", __FUNCTION__);
#endif
}


int main()
{
#ifdef ENABLE_IARM
    int input;
    bool loop = true;
    IARM_Result_t retVal = IARM_RESULT_SUCCESS;
//    printf("[%s] Starting \'%s\' Client.\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME);

    IARM_Bus_Init(NM_MGR_WIFI_CLIENT);
    IARM_Bus_Connect();

    do
    {
        printf("\n==================================================================\n");
        printf("*****	Network Mgr: Execute WiFi Manager Api's		**** 	");
        printf("\n==================================================================\n");
        printf( "1. getAvailableSSIDs\n" );
        printf( "2. getCurrentState\n" );
        printf( "3. setEnabled\n" );
        printf( "4. getPairedSSID\n");
        printf( "5. connect\n");
        printf( "6. initiateWPSPairing\n");
        printf( "7. saveSSID\n");
        printf( "8. clearSSID\n");
        printf( "9. isPaired\n");
        printf( "10. getRadioProps\n");
        printf( "11. getRadioStatsProps\n");
        printf( "12. getSSIDProps\n");
#ifdef ENABLE_LOST_FOUND
        printf( "13. getLAFState\n");
#endif
        printf( "14. getConnectedSSID\n");
        printf( "15. getEndPointProps\n");
        printf( "16. getAvailableSSIDsWithName\n");
        printf( "17. getAvailableSSIDsAsycIncr\n");
        printf( "18. stopProgressiveWifiScanning\n");
        printf( "19. disconnectSSID\n");
        printf( "20. cancelWPSPairing\n");
        printf( "0. Exit." );
        printf( "\n==================================================================\n");

        printf( "\n Selection: " );
        std::cin >> input;

        /* Get/Set RPC handler*/
        switch (input) {
        case Test_getAvailableSSIDs:
            WIFI_MGR_API_getAvailableSSIDs();
            break;
        case Test_getAvailableSSIDsWithName:
            WIFI_MGR_API_getAvailableSSIDsWithName();
            break;
        case Test_getCurrentState:
            WIFI_MGR_API_getCurrentState();
            break;
        case Test_setEnabled:
            printf( "\'setEnabled\' Not Applicable any more..!!!\n" );
//			WIFI_MGR_API_setEnabled();
            break;
        case Test_getPairedSSID:
            WIFI_MGR_API_getPairedSSID();
            break;
        case Test_connect:
            WIFI_MGR_API_connect();
            break;
        case Test_initiateWPSPairing:
            WIFI_MGR_API_initiateWPSPairing();
            break;
        case Test_saveSSID:
            WIFI_MGR_API_saveSSID();
            break;
        case Test_clearSSID:
            WIFI_MGR_API_clearSSID();
            break;
        case Test_isPaired:
            WIFI_MGR_API_isPaired();
            break;
        case Test_getRadioProps:
            WIFI_MGR_API_getRadioProps();
            break;
        case Test_getRadioStatsProps:
            WIFI_MGR_API_getRadioStatsProps();
            break;
        case Test_getSSIDProps:
            WIFI_MGR_API_getSSIDProps();
            break;
#ifdef ENABLE_LOST_FOUND
        case Test_getLAFState:
            WIFI_MGR_API_getLAFState();
            break;
#endif
        case Test_getConnectedSSID:
            WIFI_MGR_API_getConnectedSSID();
            break;
        case Test_getEndPointProps:
            WIFI_MGR_API_getEndPointProps();
            break;
        case Test_getAvailableSSIDsIncr:
            WIFI_MGR_API_getAvailableSSIDsIncr();
            break;
        case Test_stopProgressiveScanning:
            WIFI_MGR_API_stopProgressiveScanning();
            break;
        case Test_disconnectSSID:
            WIFI_MGR_API_disconnectSSID();
            break;
        case Test_cancelWPSPairing:
            WIFI_MGR_API_cancelWPSPairing();
            break;
        default:
            loop = false;
            printf( "Wrong Input..., try again.\n" );
            break;
        }
    } while (loop);

    IARM_Bus_Disconnect();
    IARM_Bus_Term();
//    printf("[%s] Exiting... \'%s\' Client Exiting\r\n", __FUNCTION__, IARM_BUS_NM_SRV_MGR_NAME );

#endif
}

