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

#include <iostream>
#include <stdint.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <glib.h>
#include <sys/types.h>
#include <ifaddrs.h>
#include "mocaSrvMgr.h"
extern "C"
{
#include "moca_hal.h"
}
#include "netsrvmgrIarm.h"
#include "NetworkMgrMain.h"
#include "NetworkMedium.h"


bool mocaLogEnable=false;
unsigned int mocaLogDuration=3600;


MocaNetworkMgr* MocaNetworkMgr::instance = NULL;
bool MocaNetworkMgr::instanceIsReady = false;


MocaNetworkMgr::MocaNetworkMgr() {}
MocaNetworkMgr::~MocaNetworkMgr() { }

MocaNetworkMgr* MocaNetworkMgr::getInstance()
{
    if (instance == NULL)
    {
        instance = new MocaNetworkMgr();
        instanceIsReady = true;
    }
    return instance;
}


int  MocaNetworkMgr::Start()
{
    bool retVal=false;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG, _mocaEventHandler);
    IARM_Bus_RegisterEventHandler(IARM_BUS_NM_SRV_MGR_NAME, IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION, _mocaEventHandler);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogStatus, mocaTelemetryLogEnable);
    IARM_Bus_RegisterCall(IARM_BUS_NETWORK_MANAGER_MOCA_getTelemetryLogDuration, mocaTelemetryLogDuration);
    startMocaTelemetry();
}

void *mocaTelemetryThrd(void* arg)
{
    int ret = 0;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );

    while (true) {
        if(mocaLogEnable)
        {
            MocaNetworkMgr::printMocaTelemetry();
            sleep(mocaLogDuration);
        }
        else
        {
            sleep(300);
        }
    }

    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}

void startMocaTelemetry()
{
    pthread_t mocaTelemetryThread;
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    pthread_create(&mocaTelemetryThread, &attr, &mocaTelemetryThrd, NULL);
}


void MocaNetworkMgr::printMocaTelemetry()
{
    eMoCALinkStatus status;
    unsigned int ncNodeID;
    unsigned char mac[6];
    unsigned int totalMoCANode;
    unsigned int count=0;
    GString *phyRate=g_string_new(NULL);
    GString *mocaPower=g_string_new(NULL);
    mocaPhyRateData mocaPhyRate = { 0 };
    mocaPowerLevelData mocaPowerLevel = { 0 };
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    moca_init();
    moca_getStatus(&status);
    if(status == UP)
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_STATUS:UP\n");
        moca_getNetworkControllerNodeId(&ncNodeID);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_NC_NODEID:%d\n",ncNodeID);
        moca_getNetworkControllerMac(mac);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_NC_MAC:%02x:%02x:%02x:%02x:%02x:%02x\n",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
        moca_getNumberOfNodesConnected(&totalMoCANode);
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TOTAL_NODE:%d\n",++totalMoCANode);
        moca_getPhyRxRate(&mocaPhyRate);
        while(count < mocaPhyRate.totalNodesPresent)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHYRXRATE_%d_%d:%d\n",mocaPhyRate.deviceNodeId,mocaPhyRate.neighNodeId[count],mocaPhyRate.phyRate[count]);
            g_string_append_printf(phyRate,"%d",mocaPhyRate.phyRate[count]);
            count ++;
            if(count < mocaPhyRate.totalNodesPresent)
            {
                g_string_append_c(phyRate,',');
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHY_RX_RATE:%s\n",phyRate->str);
            }
        }
        memset(&mocaPhyRate, 0, sizeof mocaPhyRate);
        moca_getPhyTxRate(&mocaPhyRate);
        count=0;
        g_string_assign(phyRate,"");
        while(count < mocaPhyRate.totalNodesPresent)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHYTXRATE_%d_%d:%d\n",mocaPhyRate.deviceNodeId,mocaPhyRate.neighNodeId[count],mocaPhyRate.phyRate[count]);
            g_string_append_printf(phyRate,"%d",mocaPhyRate.phyRate[count]);
            count ++;
            if(count < mocaPhyRate.totalNodesPresent)
            {
                g_string_append_c(phyRate,',');
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_PHY_TX_RATE:%s\n",phyRate->str);
            }
        }
        moca_getRxPower(&mocaPowerLevel);
	count=0;
        while(count < mocaPowerLevel.totalNodesPresent)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_RXPOWER_%d_%d:%d\n",mocaPowerLevel.deviceNodeId,mocaPowerLevel.neighNodeId[count],mocaPowerLevel.mocaPower[count]);
            g_string_append_printf(mocaPower,"%d",mocaPowerLevel.mocaPower[count]);
            count ++;
            if(count < mocaPowerLevel.totalNodesPresent)
            {
                g_string_append_c(mocaPower,',');
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_RX_POWER:%s\n",mocaPower->str);
            }
        }
        memset(&mocaPowerLevel, 0, sizeof mocaPhyRate);
        moca_getTxPowerReduction(&mocaPowerLevel);
        count=0;
        g_string_assign(mocaPower,"");
        while(count < mocaPowerLevel.totalNodesPresent)
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TXPOWERREDUCTION_%d_%d:%d\n",mocaPowerLevel.deviceNodeId,mocaPowerLevel.neighNodeId[count],mocaPowerLevel.mocaPower[count]);
            g_string_append_printf(mocaPower,"%d",mocaPowerLevel.mocaPower[count]);
            count ++;
            if(count < mocaPowerLevel.totalNodesPresent)
            {
                g_string_append_c(mocaPower,',');
            }
            else
            {
                RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_TX_POWER_REDUCTION:%s\n",mocaPower->str);
            }
        }
        if (mocaPhyRate.deviceNodeId == ncNodeID )
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_IS_CURRENT_NODE_NC:1\n");
        }
        else
        {
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR,"TELEMETRY_MOCA_IS_CURRENT_NODE_NC:0\n");
        }

    }
    else
    {
        RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "TELEMETRY_MOCA_STATUS:DOWN \n");

    }
    g_string_free(phyRate,TRUE);
    g_string_free(mocaPower,TRUE);
    moca_UnInit();
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );

}

static void _mocaEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len)
{
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    if (strcmp(owner, IARM_BUS_NM_SRV_MGR_NAME)  == 0) {
        switch (eventId) {
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG:
        {
            bool *param = (bool *)data;
            mocaLogEnable=*param;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG %d \n",__FUNCTION__, __LINE__,mocaLogEnable);
        }
        break;
        case IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION:
        {
            unsigned int *param = (unsigned int *)data;
            mocaLogDuration=*param;
            RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] got event IARM_BUS_NETWORK_MANAGER_MOCA_TELEMETRY_LOG_DURATION %d \n",__FUNCTION__, __LINE__,mocaLogDuration);
        }
        break;
        default:
            break;
        }
    }
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogEnable(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n",__FUNCTION__, __LINE__ );
    bool *param = (bool *)arg;
    *param=mocaLogEnable;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] get event to get status of telemetry log enable %d \n",__FUNCTION__, __LINE__,mocaLogEnable);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n",__FUNCTION__, __LINE__ );
    return ret;
}

IARM_Result_t MocaNetworkMgr::mocaTelemetryLogDuration(void *arg)
{
    IARM_Result_t ret = IARM_RESULT_SUCCESS;
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Enter\n", __FUNCTION__, __LINE__ );
    unsigned int *param = (unsigned int *)arg;
    *param=mocaLogDuration;
    RDK_LOG( RDK_LOG_INFO, LOG_NMGR, "[%s:%d] get venent to get the moca log duration set %d \n",__FUNCTION__, __LINE__,mocaLogDuration);
    RDK_LOG( RDK_LOG_TRACE1, LOG_NMGR, "[%s:%d] Exit\n", __FUNCTION__, __LINE__ );
    return ret;
}
