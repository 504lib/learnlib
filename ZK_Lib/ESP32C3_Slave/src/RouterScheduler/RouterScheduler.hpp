#pragma once

#include <Arduino.h>
#include "../Station/Station.hpp"
#include "../NetworkClient/NetworkClient.hpp"
#include "../Vehicle/Vehicle.hpp"
#include "../StationRepo/StationRepo.hpp"
#include "../protocol/protocol.hpp"

class RouterScheduler
{
    private:
        StationRepo& station_repo;        // 站点链表对象
        NetworkClient& network_client;    // 网络客户端对象
        Vehicle_Info& vehicle_info;      // 车辆信息对象 
        bool Connect_To_Station(uint8_t index);
        float CalculateStationScore(uint8_t index);
        int FindBestStation();
        void CheckArrivingAndMaybeLeave();
        bool sendSinglePost(uint8_t index);

    public:
        RouterScheduler(StationRepo& repo, NetworkClient& client, Vehicle_Info& vehicle)
            : station_repo(repo), network_client(client), vehicle_info(vehicle){}
        void RouterScheduler_Executer();

};