/**
 * @file RouterScheduler.cpp
 * @author whyP762 (3046961251@qq.com)
 * @brief    路线调度器类实现文件
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "RouterScheduler.hpp"

/**
 * @brief    连接到指定站点
 * 
 * @param    index     目标站点索引
 * @return   true      
 * @return   false     function of param
 */
bool RouterScheduler::Connect_To_Station(uint8_t index)
{
    if(index >= station_repo.Get_Station_Count())
    {
        LOG_WARN("Connect_To_Station: Invalid index %d", index);
        return false;
    }
    Station_t& station = station_repo.Get_Index_Station(index);
    bool connected = network_client.ensureWiFiConnected(station.ssid, station.password);
    if (connected)
    {
        station_repo.Change_current_index(index);
        station.isConnnectd = true;
        station.lastRSSI = WiFi.RSSI();
        station.lastVisitTime = millis();
        station.visitCount++;
        LOG_INFO("Connected to station %s successfully", station.name);
        return true;
    }
    else
    {
        LOG_WARN("Failed to connect to station %s", station.name);
        return false;
    }
}

/**
 * @brief    计算站点得分
 * @details  根据多个规则计算站点的优先级得分，不存在的SSID得分极低。
 *           优先级: 下一目标站点 > 信号强度 > 未访问过 > 长时间未访问 > 访问次数少
 * @date     2025-12-20 2:33
 * @param    index     站点索引
 * @return   float     站点得分 
 */
float RouterScheduler::CalculateStationScore(uint8_t index)
{
    uint8_t current_index = station_repo.Get_Current_Index();
    Station_t& station = station_repo.Get_Index_Station(index);
    float score = 0.0f;
    LOG_INFO("Calculating score for station %s (SSID: %s)", station.name, station.ssid);
    int8_t currentRSSI = network_client.RSSI_intesify(station.ssid);
    // int8_t currentRSSI = network_client.RSSI_intesify(String(station.ssid));
    if (currentRSSI == -1)
    {
        LOG_INFO("RouterScheduler: 站点 %s 不在扫描结果中，得分极低", station.name);
        return -1000.0f; // SSID 不在扫描结果中，得分极低
    }
    
    // 规则1: 下一目标站点最高优先级
    uint8_t nextTarget = current_index;
    if (index == nextTarget) 
    {
        score += 50.0f;
    }
    
    // 规则2: 信号强度权重
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

/**
 * @brief    找出最佳站点
 * @details  根据计算的站点得分选择最佳站点进行连接。
 * @date     2025-12-20 2:34
 * @return   int       最佳站点索引，找不到返回-1
 */
int RouterScheduler::FindBestStation()
{
    uint8_t used_num = station_repo.Get_Station_Count();
    uint8_t SSID_Num = network_client.getMaxSSIDNum();
    if (used_num == 0 || SSID_Num == 0)
    {
        LOG_INFO("FindBestStation: 无可用站点或无扫描结果");
        return -1;
    }

    int bestIndex = -1;
    float bestScore = -1000.0f;
    // 遍历所有站点，计算得分并选择最佳
    for (uint8_t i = 0; i < used_num; i++) 
    {
        Station_t& station = station_repo.Get_Index_Station(i);
        float score = CalculateStationScore(i);
        
        if (score > bestScore || (score == bestScore && i < bestIndex)) 
        {
            bestScore = score;
            bestIndex = i;
        }
    }
    if (bestIndex != -1) 
    {
        LOG_INFO("🏆 最佳站点: %s (得分: %.2f)", 
                station_repo.Get_Index_Station_Name(bestIndex, true).c_str(), bestScore);
    }
    
    return bestIndex;
}

/**
 * @brief    检查是否到达站点并决定是否离开
 * @todo     此函数在STATUS_CONNECTED状态下调用，但是其函数涉及到网络连接和HTTP请求，
 *           需要进行解耦，以增加维护性
 */
void RouterScheduler::CheckArrivingAndMaybeLeave(VehicleStatus status)
{
    uint8_t used_num = station_repo.Get_Station_Count();
    uint8_t current_index = station_repo.Get_Current_Index();
    if (used_num == 0 || current_index >= used_num)
    {
        LOG_INFO("CheckArriving: 无可用站点，跳过检查");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_INFO("CheckArriving: WiFi 未连接，跳过检查");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    // 构建 /api/info URL
    String url = station_repo.Get_Index_Station(current_index).ip;
    if (!url.startsWith("http://") && !url.startsWith("https://")) url = String("http://") + url;
    if (!url.endsWith("/")) url += "/";
    url += "api/info";

    LOG_DEBUG("CheckArriving GET: %s", url.c_str());
    StaticJsonDocument<MAX_NETWORK_JSON_BUFFER_SIZE> doc;
    bool success = network_client.sendGetRequest(url,doc);
    if (!success)
    {
        LOG_WARN("CheckArriving: GET 请求失败");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (!doc["passenger_list"].is<JsonArray>())
    {
        LOG_WARN("CheckArriving: JSON 中无 passenger_list 字段");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    JsonArray passenger_arr = doc["passenger_list"].as<JsonArray>();
    int routeIndex = static_cast<int>(vehicle_info.Get_Vehicle_Rounter());
    if (routeIndex < 0 || routeIndex >= (int)passenger_arr.size())
    {
        LOG_WARN("CheckArriving: routeIndex(%d) 超出 passenger_list 大小(%u)", routeIndex, (unsigned)passenger_arr.size());
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    int pnum = passenger_arr[routeIndex];
    LOG_INFO("CheckArriving: route %d passenger_num = %d", routeIndex, pnum);

    if (pnum == 0)
    {
        LOG_INFO("CheckArriving: route %d 无乘客，切换到 LEAVING", routeIndex);
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_LEAVING);
        return;
    }
    if (commandQueueCallback)       // 这个队列来自 main.cpp 的回调设置
    {
        DataPacket_t ack;
        ack.type = CmdType::VEHICLE_STATUS;
        ack.data[0] =static_cast<uint8_t>(status);
        ack.length = 1;
        commandQueueCallback(ack);
        LOG_INFO("RouterScheduler: 状态报告发送成功 状态:%d",static_cast<uint8_t>(status));
    }
    
    vehicle_info.Update_Vehicle_Status(status);
    sendSinglePost(station_repo.Get_Current_Index());
}


void RouterScheduler::CheckArrivingAndMaybeLeave_C_style(VehicleStatus status)
{
    uint8_t used_num = station_repo.Get_Station_Count();
    uint8_t current_index = station_repo.Get_Current_Index();
    if (used_num == 0 || current_index >= used_num)
    {
        LOG_INFO("CheckArriving: 无可用站点，跳过检查");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (WiFi.status() != WL_CONNECTED)
    {
        LOG_INFO("CheckArriving: WiFi 未连接，跳过检查");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    const char* ip = station_repo.Get_Index_Station(current_index).ip;
    LOG_ASSERT(ip != nullptr);

    char url_buffer[64];
    size_t ip_len = strlen(ip);
    bool has_http = (strncmp(ip, "http://", 7) == 0) || (strncmp(ip, "https://", 8) == 0);
    bool has_slash = (ip_len > 0 && ip[ip_len - 1] == '/');

    int written = 0;
    if (has_http)
    {
        written = snprintf(url_buffer, sizeof(url_buffer), "%s%sapi/info", ip, has_slash ? "" : "/");
    }
    else
    {
        written = snprintf(url_buffer, sizeof(url_buffer), "http://%s%sapi/info", ip, has_slash ? "" : "/");
    }

    if (written <= 0 || static_cast<size_t>(written) >= sizeof(url_buffer))
    {
        LOG_WARN("CheckArriving: URL 缓冲区不足");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    LOG_DEBUG("CheckArriving GET: %s", url_buffer);
    StaticJsonDocument<MAX_NETWORK_JSON_BUFFER_SIZE> doc;
    bool success = network_client.sendGetRequest(url_buffer, doc);
    if (!success)
    {
        LOG_WARN("CheckArriving: GET 请求失败");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    if (!doc["passenger_list"].is<JsonArray>())
    {
        LOG_WARN("CheckArriving: JSON 中无 passenger_list 字段");
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }
    JsonArray passenger_arr = doc["passenger_list"].as<JsonArray>();
    int routeIndex = static_cast<int>(vehicle_info.Get_Vehicle_Rounter());
    if (routeIndex < 0 || routeIndex >= (int)passenger_arr.size())
    {
        LOG_WARN("CheckArriving: routeIndex(%d) 超出 passenger_list 大小(%u)", routeIndex, (unsigned)passenger_arr.size());
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
        return;
    }

    int pnum = passenger_arr[routeIndex];
    LOG_INFO("CheckArriving: route %d passenger_num = %d", routeIndex, pnum);

    if (pnum == 0)
    {
        LOG_INFO("CheckArriving: route %d 无乘客，切换到 LEAVING", routeIndex);
        vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_LEAVING);
        return;
    }
    if (commandQueueCallback)       // 这个队列来自 main.cpp 的回调设置
    {
        DataPacket_t ack;
        ack.type = CmdType::VEHICLE_STATUS;
        ack.data[0] =static_cast<uint8_t>(status);
        ack.length = 1;
        commandQueueCallback(ack);
        LOG_INFO("RouterScheduler: 状态报告发送成功 状态:%d",static_cast<uint8_t>(status));
    }
    
    vehicle_info.Update_Vehicle_Status(status);
    sendSinglePost_C_style(station_repo.Get_Current_Index());
}


/**
 * @brief    设置命令队列回调函数
 * 
 * @param    callback  
 */
void RouterScheduler::setCommandQueueCallback(CommandQueueCallback callback)
{
    commandQueueCallback = callback;
}

/**
 * @brief    发送单次状态POST请求
 * 
 * @param    index     站点索引
 * @return   true      POST请求成功
 * @return   false     POST请求失败
 */
bool RouterScheduler::sendSinglePost(uint8_t index)
{
    if(WiFi.status() != WL_CONNECTED)
    {
        LOG_WARN("WiFi 未连接，无法发送数据");
        return false;
    }
    Station_t& station = station_repo.Get_Index_Station(index);
    Rounter rounter = vehicle_info.Get_Vehicle_Rounter();
    String plate = vehicle_info.Get_Vehicle_Plate();
    VehicleStatus status = vehicle_info.Get_Vehicle_Status(); 
    String postData = "route=" + String(static_cast<uint8_t>(rounter)) + 
                        "&plate=" +  plate + 
                        "&status=" + vehicle_info.Get_Status_Str(status);
    // todo: 修改字符串为c风格,目前暂时使用String
    char url_buffer[64];
    snprintf(url_buffer, sizeof(url_buffer), "%s/api/vehicle_report", station.ip);
    return network_client.sendPostRequest(url_buffer, postData);
}


bool RouterScheduler::sendSinglePost_C_style(uint8_t index)
{
    char postData_buffer[MAX_VEHICLE_JSON_LENGTH];
    char plate_buffer[MAX_VEHICLE_PLATE_LENGTH];
    char status_buffer[MAX_VEHICLE_STATUS_STRING_LENGTH];
    if(WiFi.status() != WL_CONNECTED)
    {
        LOG_WARN("WiFi 未连接，无法发送数据");
        return false;
    }
    size_t plate_len = vehicle_info.Get_Vehicle_Plate(plate_buffer, sizeof(plate_buffer));
    size_t status_len = vehicle_info.Get_Status_Str(status_buffer, sizeof(status_buffer), vehicle_info.Get_Vehicle_Status());
    if (plate_len == 0 || status_len == 0)
    {
        LOG_WARN("sendSinglePost_C_style: plate/status 为空");
        return false;
    }

    Station_t& station = station_repo.Get_Index_Station(index);
    Rounter rounter = vehicle_info.Get_Vehicle_Rounter();
    VehicleStatus status = vehicle_info.Get_Vehicle_Status(); 
    int post_written = snprintf(postData_buffer, sizeof(postData_buffer), "route=%u&plate=%s&status=%s", 
             static_cast<unsigned>(rounter), plate_buffer, status_buffer);
    if (post_written <= 0 || static_cast<size_t>(post_written) >= sizeof(postData_buffer))
    {
        LOG_WARN("sendSinglePost_C_style: postData 缓冲区不足");
        return false;
    }
    // todo: 修改字符串为c风格,目前暂时使用String
    char url_buffer[64];
    int url_written = snprintf(url_buffer, sizeof(url_buffer), "%s/api/vehicle_report", station.ip);
    if (url_written <= 0 || static_cast<size_t>(url_written) >= sizeof(url_buffer))
    {
        LOG_WARN("sendSinglePost_C_style: url 缓冲区不足");
        return false;
    }
    return network_client.sendPostRequest(url_buffer, postData_buffer);
}

/**
 * @brief    路线调度器执行函数
 * @date     2025-12-20 13:08
 */
void RouterScheduler::RouterScheduler_Executer()
{
    VehicleStatus status = vehicle_info.Get_Vehicle_Status();
    switch (status)
    {
        case VehicleStatus::STATUS_SCANNING:
        {
            if (!network_client.isWiFiscanning())
            {
                LOG_INFO("RouterScheduler: 开始扫描 WiFi 网络...");
                network_client.startWiFiScan();
                return;
            }
            bool isReady = network_client.checkWiFiScan();
            if (!isReady)
            {
                return;
            }
            LOG_INFO("RouterScheduler: WiFi 扫描完成，寻找合适的站点...");
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_GROPE); // 切换到下一个状态
            break;
        }
        case VehicleStatus::STATUS_GROPE:
        {
            int bestIndex = FindBestStation();
            if (bestIndex == -1)
            {
                LOG_INFO("RouterScheduler: 未找到合适的站点，继续扫描");
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_SCANNING);
                return;
            }
            station_repo.Change_current_index(bestIndex);
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_CONNECTING);
            break;
        }
        case VehicleStatus::STATUS_CONNECTING:
        {
            int current_index = station_repo.Get_Current_Index();
            bool connected = Connect_To_Station(current_index);
            if (connected)
            {
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STATUS_CONNECTED);
            }
            else
            {
                vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
            }
            break;
        }
        case VehicleStatus::STATUS_CONNECTED:
        {
            CheckArrivingAndMaybeLeave_C_style(VehicleStatus::STATUS_ARRIVING);
            break;
        }
        case VehicleStatus::STAUS_DISCONNECTED:
        {
            Station_t& station = station_repo.Get_Index_Station(station_repo.Get_Current_Index());
            station.isConnnectd = false;
            WiFi.disconnect();
            LOG_INFO("RouterScheduler: WiFi 已断开，重新扫描");
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
                CheckArrivingAndMaybeLeave_C_style(VehicleStatus::STATUS_WAITING);
                // if(commandQueueCallback)
                // {
                //     DataPacket_t ack;
                //     ack.type = CmdType::VEHICLE_STATUS;
                //     ack.data[0] = static_cast<uint8_t>(VehicleStatus::STATUS_WAITING);
                //     ack.length = 1;
                //     commandQueueCallback(ack);
                //     LOG_INFO("RouterScheduler: 状态报告发送成功 状态:%d",static_cast<uint8_t>(VehicleStatus::STATUS_WAITING));
                // }
                // LOG_INFO("RouterScheduler: 状态报告发送成功");
            }
            else
            {
                LOG_WARN("RouterScheduler: 状态报告发送失败");
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
            
            static uint32_t lastPostTime = 0;
            if (millis() - lastPostTime < 5000)
            {
                return;
            }
            lastPostTime = millis();
            // if (commandQueueCallback)
            // {
            //     DataPacket_t ack;
            //     ack.type = CmdType::VEHICLE_STATUS;
            //     ack.data[0] = static_cast<uint8_t>(VehicleStatus::STATUS_ARRIVING);
            //     ack.length = 1;
            //     commandQueueCallback(ack);
            //     LOG_INFO("RouterScheduler: 状态报告发送成功 状态:%d",static_cast<uint8_t>(VehicleStatus::STATUS_ARRIVING));
            // }
            // LOG_INFO("RouterScheduler: 车辆处于 ARRIVING 状态，保持连接");    
            CheckArrivingAndMaybeLeave_C_style(VehicleStatus::STATUS_ARRIVING);
            break;
        }
        case VehicleStatus::STATUS_LEAVING:
        {
            station_repo.Move_To_Next_Station();    
            bool isPosted = sendSinglePost(station_repo.Get_Current_Index());
            for (size_t i = 0; i < 5 && !isPosted; i++)
            {
                if (WiFi.status() != WL_CONNECTED)
                {
                    LOG_INFO("RouterScheduler: WiFi 未连接，无法发送离开状态报告");
                    break;
                }
                
                LOG_INFO("RouterScheduler: 正在发送离开状态报告，尝试次数 %d", i + 1);
                isPosted = sendSinglePost(station_repo.Get_Current_Index());
                if (isPosted)
                {
                    LOG_INFO("RouterScheduler: 离开状态报告发送成功");
                    break;
                } 
                delay(100);
            }
            if (commandQueueCallback)
            {
                DataPacket_t ack;
                ack.type = CmdType::VEHICLE_STATUS;
                ack.data[0] =static_cast<uint8_t>(VehicleStatus::STATUS_LEAVING);
                ack.length = 1;
                commandQueueCallback(ack);
                LOG_INFO("RouterScheduler: 状态报告发送成功 状态:%d",static_cast<uint8_t>(VehicleStatus::STATUS_LEAVING));
            }
            vehicle_info.Update_Vehicle_Status(VehicleStatus::STAUS_DISCONNECTED);
            break;
        }
        case VehicleStatus::STATUS_IDLE:
        {
            LOG_INFO("RouterScheduler: 车辆处于 IDLE 状态，重新扫描");
            break;
        }
    default:
        break;
    }
}

/**
 * @brief    获取路线调度信息的 JSON 表示
 * @details  返回的 JSON 格式如下：
 * {
 *   "station_repo": {  // 站点仓库信息
 *     ...  // 站点仓库的 JSON 表示
 *   },
 *   "vehicle_info": {  // 车辆信息
 *     ...  // 车辆信息的 JSON 表示
 *   }
 * }
 * @return   String    包含路线调度信息的 JSON 字符串
 */
String RouterScheduler::Get_RouterInfo_JSON() {
    // 定义 JSON 文档
    StaticJsonDocument<2048> doc;       // 创建容量为 2048 的 JsonDocument
    StaticJsonDocument<1024> stationdoc; // 创建容量为 1024 的 JsonDocument
    StaticJsonDocument<512> vehicledoc;  // 创建容量为 512 的 JsonDocument
    // 解析站点仓库的 JSON 数据
    DeserializationError err = deserializeJson(stationdoc, station_repo.Get_StationList_JSON());
    if (err) {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 解析站点仓库 JSON 失败: %s", err.c_str());
        return "{\"error\":\"Failed to parse station_repo JSON\"}";
    }
    doc["station_repo"] = stationdoc.as<JsonObject>();

    // 解析车辆信息的 JSON 数据
    err = deserializeJson(vehicledoc, vehicle_info.Vehiicle_Json());
    if (err) {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 解析车辆信息 JSON 失败: %s", err.c_str());
        return "{\"error\":\"Failed to parse vehicle_info JSON\"}";
    }
    doc["vehicle_info"] = vehicledoc.as<JsonObject>();

    // 序列化为 JSON 字符串
    String jsonStr;
    serializeJson(doc, jsonStr);
    return jsonStr;
}


size_t RouterScheduler::Get_RouterInfo_JSON(char* buffer, size_t buffer_size)
{
    StaticBufferError error = check_pool(buffer, buffer_size, MAX_ROUTER_INFO_JSON_LENGTH);
    if (error != StaticBufferError::OK)
    {
        LOG_FATAL("Buffer check failed with error code %d, cannot get router info JSON", static_cast<int>(error));
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    // 定义 JSON 文档
    StaticJsonDocument<2048> doc;       // 创建容量为 2048 的 JsonDocument
    StaticJsonDocument<1024> stationdoc; // 创建容量为 1024 的 JsonDocument
    StaticJsonDocument<1024> vehicledoc;  // 创建容量为 512 的 JsonDocument

    char station_json[MAX_STATION_LIST_JSON_LENGTH];
    size_t station_len = station_repo.Get_StationList_JSON(station_json, sizeof(station_json));
    if (station_len == 0)
    {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 站点仓库 JSON 为空");
        return snprintf(buffer, buffer_size, "{\"error\":\"Empty station_repo JSON\"}");
    }
    // 解析站点仓库的 JSON 数据
    DeserializationError err = deserializeJson(stationdoc, station_json, sizeof(station_json));
    if (err) {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 解析站点仓库 JSON 失败: %s", err.c_str());
        return snprintf(buffer, buffer_size, "{\"error\":\"Failed to parse station_repo JSON\"}");
    }
    doc["station_repo"] = stationdoc.as<JsonObject>();
    char vehicle_json[MAX_VEHICLE_JSON_LENGTH];
    size_t vehicle_len = vehicle_info.Vehiicle_Json(vehicle_json, sizeof(vehicle_json));
    if (vehicle_len == 0)
    {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 车辆信息 JSON 为空");
        return snprintf(buffer, buffer_size, "{\"error\":\"Empty vehicle_info JSON\"}");
    }
    // 解析车辆信息的 JSON 数据
    err = deserializeJson(vehicledoc, vehicle_json, sizeof(vehicle_json));
    if (err) {
        LOG_WARN("RouterScheduler::Get_RouterInfo_JSON 解析车辆信息 JSON 失败: %s", err.c_str());
        return snprintf(buffer, buffer_size, "{\"error\":\"Failed to parse vehicle_info JSON\"}");
    }
    doc["vehicle_info"] = vehicledoc.as<JsonObject>();

    // 序列化为 JSON 字符串
    return serializeJson(doc, buffer, buffer_size);
}