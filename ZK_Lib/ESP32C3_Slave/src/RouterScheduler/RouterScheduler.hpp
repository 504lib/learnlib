#pragma once

/**
 * @file RouterScheduler.hpp
 * @author whyP762 (3046961251@qq.com)
 * @brief    路线调度器类定义头文件
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include <Arduino.h>
#include <functional>
#include "../Station/Station.hpp"
#include "../NetworkClient/NetworkClient.hpp"
#include "../Vehicle/Vehicle.hpp"
#include "../StationRepo/StationRepo.hpp"
#include "../protocol/protocol.hpp"
#include "../Log/Log.h"

typedef std::function<void(const DataPacket_t)> CommandQueueCallback;

class RouterScheduler
{
    private:
        StationRepo& station_repo;        // 站点链表对象
        NetworkClient& network_client;    // 网络客户端对象
        Vehicle_Info& vehicle_info;      // 车辆信息对象 
        CommandQueueCallback commandQueueCallback;

        bool Connect_To_Station(uint8_t index);
        float CalculateStationScore(uint8_t index);
        int FindBestStation();
        void CheckArrivingAndMaybeLeave();
        bool sendSinglePost(uint8_t index);

    public:
        RouterScheduler(StationRepo& repo, NetworkClient& client, Vehicle_Info& vehicle)
            : station_repo(repo), network_client(client), vehicle_info(vehicle){}
        void setCommandQueueCallback(CommandQueueCallback callback);
        void RouterScheduler_Executer();
        String Get_RouterInfo_JSON();
};