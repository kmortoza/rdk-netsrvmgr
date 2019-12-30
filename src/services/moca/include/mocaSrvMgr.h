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

#include "libIARM.h"
#include "libIBus.h"
#include "libIARMCore.h"
#include "rdk_moca_hal.h"
class MocaNetworkMgr
{
public:
    static MocaNetworkMgr* getInstance();
    int Start();
    static void printMocaTelemetry();
    static IARM_Result_t mocaTelemetryLogEnable(void *arg);
    static IARM_Result_t mocaTelemetryLogDuration(void *arg);
private:
    MocaNetworkMgr();
    virtual ~MocaNetworkMgr();
    static bool instanceIsReady;
    static MocaNetworkMgr* instance;
};

/**
 * @addtogroup NETSRVMGR_APIS
 * @{
 */

/**
 * @brief Initializes a thread to retrieve and send MoCA related telemetry data.
 */
void startMocaTelemetry();

/**
 * @brief This Thread function prints the statistics information on MOCA interface if the MOCA link is up.
 */
void *mocaTelemetryThread(void* arg);
static void _mocaEventHandler(const char *owner, IARM_EventId_t eventId, void *data, size_t len);
static void eventCallback(const enum RMH_Event event, const struct RMH_EventData *eventData, void* userContext);

/** @} */  //END OF GROUP NETSRVMGR_APIS
