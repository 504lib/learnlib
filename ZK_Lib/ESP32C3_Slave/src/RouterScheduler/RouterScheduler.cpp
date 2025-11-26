#include "RouterScheduler.hpp"


bool RouterScheduler::Connect_To_Station(uint8_t index)
{
    if(index >= station_repo.Get_Station_Count()) return false;
    Station_t& station = station_repo.Get_Index_Station(index);
    bool connected = network_client.ensureWiFiConnected(station.ssid.c_str(), station.password.c_str());
    if (connected)
    {
        station_repo.Change_current_index(index);
        station.isConnnectd = true;
        station.lastRSSI = WiFi.RSSI();
        station.lastVisitTime = millis();
        station.visitCount++;
        return true;
    }
    else
    {
        return false;
    }
}


float RouterScheduler::CalculateStationScore(uint8_t index)
{
    uint8_t current_index = station_repo.Get_Current_Index();
    Station_t& station = station_repo.Get_Index_Station(index);
    float score = 0.0f;
    
    // 规则1: 下一目标站点最高优先级
    uint8_t nextTarget = (current_index + 1) % station_repo.Get_Station_Count();
    if (index == nextTarget) 
    {
        score += 50.0f;
    }
    
    // 规则2: 信号强度权重
    int8_t currentRSSI = network_client.RSSI_intesify(station.ssid);
    if (currentRSSI > -50) score += 30.0f;
    else if (currentRSSI > -70) score += 20.0f;
    else if (currentRSSI > -80) score += 10.0f;
    else score += 5.0f;
    
    // 规则3: 未访问过的站点加分
    if (!station.isProcessed) 
    {
        score += 15.0f;
    }
    
    // 规则4: 长时间未访问的站点加分
    unsigned long timeSinceLastVisit = millis() - station.lastVisitTime;
    if (timeSinceLastVisit > 300000)  // 5分钟
    {
        score += 10.0f;
    }
    
    // 规则5: 访问次数少的站点加分（避免总是连接同一个）
    score += (10.0f - min(station.visitCount, 10));
    
    return score;
}


int RouterScheduler::FindBestStation()
{
    uint8_t used_num = station_repo.Get_Station_Count();
    uint8_t SSID_Num = network_client.getMaxSSIDNum();
    if (used_num == 0 || SSID_Num == 0) return -1;

    int bestIndex = -1;
    float bestScore = -1000.0f;
    
    for (uint8_t i = 0; i < used_num; i++) 
    {
        float score = CalculateStationScore(i);
        
        if (score > bestScore || (score == bestScore && i < bestIndex)) 
        {
            bestScore = score;
            bestIndex = i;
        }
    }
    if (bestIndex != -1) 
    {
        Serial.printf("🏆 最佳站点: %s (得分: %.2f)\n", 
                     station_repo.Get_Index_Station_Name(bestIndex, true).c_str(), bestScore);
    }
    
    return bestIndex;
}


void RouterScheduler::CheckArrivingAndMaybeLeave()
{
    uint8_t used_num = station_repo.Get_Station_Count();
    uint8_t current_index = station_repo.Get_Current_Index();
    if (used_num == 0 || current_index >= used_num)
    {
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("CheckArriving: WiFi 未连接，跳过检查");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    // 构建 /api/info URL
    String url = station_repo.Get_Index_Station(current_index).ip;
    if (!url.startsWith("http://") && !url.startsWith("https://")) url = String("http://") + url;
    if (!url.endsWith("/")) url += "/";
    url += "api/info";

    Serial.printf("CheckArriving GET: %s\n", url.c_str());
    JsonDocument doc;
    bool success = network_client.sendGetRequest(url,doc);
    if (!success)
    {
        Serial.println("CheckArriving: GET 请求失败");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (!doc["passenger_list"].is<JsonArray>())
    {
        Serial.println("CheckArriving: JSON 中无 passenger_list 字段");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    JsonArray passenger_arr = doc["passenger_list"].as<JsonArray>();
    int routeIndex = static_cast<int>(vehicle_info.Get_Vehicle_Rounter());
    if (routeIndex < 0 || routeIndex >= (int)passenger_arr.size())
    {
        Serial.printf("CheckArriving: routeIndex(%d) 超出 passenger_list 大小(%u)\n", routeIndex, (unsigned)passenger_arr.size());
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    int pnum = passenger_arr[routeIndex];
    Serial.printf("CheckArriving: route %d passenger_num = %d\n", routeIndex, pnum);

    if (pnum == 0)
    {
        Serial.printf("CheckArriving: route %d 无乘客，切换到 LEAVING\n", routeIndex);
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_LEAVING);
        return;
    }

    vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_ARRIVING);
    sendSinglePost(station_repo.Get_Current_Index());
}


bool RouterScheduler::sendSinglePost(uint8_t index)
{
    if(WiFi.status() != WL_CONNECTED)
    {
        Serial.printf("WiFi 未连接，无法发送数据\n");
        return false;
    }
    Station_t& station = station_repo.Get_Index_Station(index);
    Rounter rounter = vehicle_info.Get_Vehicle_Rounter();
    String plate = vehicle_info.Get_Vehicle_Plate();
    VehicleStatus status = vehicle_info.Get_Vehicle_Status(); 
    String postData = "route=" + String(static_cast<uint8_t>(rounter)) + 
                        "&plate=" +  plate + 
                        "&status=" + vehicle_info.Get_Status_Str(status);
    
    return network_client.sendPostRequest(station.ip + "/api/vehicle_report", postData);
}


void RouterScheduler::RouterScheduler_Executer()
{
    VehicleStatus status = vehicle_info.Get_Vehicle_Status();
    switch (status)
    {
        case VehicleStatus::STATUS_SCANNING:
        {
            network_client.startWiFiScan();
            bool isReady = network_client.checkWiFiScan();
            if (!isReady)
            {
                Serial.println("RouterScheduler: 仍在扫描WiFi...");
                return;
            }
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_GROPE);
            break;
        }
        case VehicleStatus::STATUS_GROPE:
        {
            int bestIndex = FindBestStation();
            if (bestIndex == -1)
            {
                Serial.println("RouterScheduler: 未找到合适的站点，继续扫描");
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_SCANNING);
                return;
            }
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_CONNECTING);
            break;
        }
        case VehicleStatus::STATUS_CONNECTING:
        {
            int current_index = station_repo.Get_Current_Index();
            bool connected = Connect_To_Station(current_index);
            if (connected)
            {
                Serial.printf("RouterScheduler: 成功连接到站点 %s\n", station_repo.Get_Index_Station_Name(current_index, true).c_str());
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_CONNECTED);
            }
            else
            {
                Serial.printf("RouterScheduler: 连接站点 %s 失败，重新扫描\n", station_repo.Get_Index_Station_Name(current_index, true).c_str());
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_SCANNING);
            }
            break;
        }
        case VehicleStatus::STATUS_CONNECTED:
        {
            CheckArrivingAndMaybeLeave();
            break;
        }
        case VehicleStatus::STAUS_DISCONNECTED:
        {
            Station_t& station = station_repo.Get_Index_Station(station_repo.Get_Current_Index());
            station.isConnnectd = false;
            Serial.println("RouterScheduler: WiFi 已断开，重新扫描");
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_SCANNING);
            break;
        }
        case VehicleStatus::STATUS_WAITING:
        {
            static uint32_t lastPostTime = 0;
            if (millis() - lastPostTime < 5000)
            {
                return;
            }
            lastPostTime = millis();
            bool postSuccess = sendSinglePost(station_repo.Get_Current_Index());
            if (postSuccess)
            {
                Serial.println("RouterScheduler: 状态报告发送成功");
            }
            else
            {
                Serial.println("RouterScheduler: 状态报告发送失败");
            }
            break;
        }
        case VehicleStatus::STATUS_ARRIVING:
        {
            if (WiFi.status() != WL_CONNECTED)
            {
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
                break;
            }
            
            Serial.println("RouterScheduler: 车辆处于 ARRIVING 状态，保持连接");    
            break;
        }
        case VehicleStatus::STATUS_LEAVING:
        {
            Station_t& station = station_repo.Get_Index_Station(station_repo.Get_Current_Index());
            station.isProcessed = true;
            station_repo.Move_To_Next_Station();    
            bool isPosted = sendSinglePost(station_repo.Get_Current_Index());
            if (isPosted)
            {
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_SCANNING);
            }
            else
            {
                Serial.println("RouterScheduler: 离开状态报告发送失败，中断连接");
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
            }
            break;
        }
        case VehicleStatus::STATUS_IDLE:
        {
            Serial.println("RouterScheduler: 车辆处于 IDLE 状态，重新扫描");
            break;
        }
    default:
        break;
    }
}