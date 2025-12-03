/**
 * @file StationRepo.cpp
 * @author whyP762 (3046961251@qq.com)
 * @brief    StationRepo类的实现文件
 * @version 0.1
 * @date 2025-11-28
 * 
 * @copyright Copyright (c) 2025
 * 
 */

#include "StationRepo.hpp"

/**
 * @brief    查找字符串中是否包含中文字符
 * 
 * @param    str       输入字符串
 * @return   true      string中包含中文字符
 * @return   false     string中不包含中文字符
 */
bool containsChinese(const String& str) 
{
    for (size_t i = 0; i < str.length(); ++i) 
    {
        uint16_t c = str[i]; 
        if(c < 0x80) continue;
        if (c >= 0x4E00 && c <= 0x9FA5) 
        {
            return true;
        }
    }
    return false;
}

/**
 * @brief    通过站点名称查找站点索引
 * 
 * @param    target_name        目标站点名称（可以是中文或英文）
 * @return   int8_t             站点索引，未找到返回-1
 */
int8_t StationRepo::find_Station(const String& target_name)
{
    if(containsChinese(target_name))
    {
        for (uint8_t i = 0; i < this->used_num; i++)  
        {
            if (strcmp(this->station_list[i].name_ch.c_str(), target_name.c_str()) == 0)  // 修复：添加 == 0 比较
            {
                LOG_DEBUG("Found station by Chinese name: %s at index %d", target_name.c_str(), i);
                return i;
            }
        }
        LOG_DEBUG("Station with Chinese name: %s not found", target_name.c_str());
        return -1;
    }
    else
    {
        for (uint8_t i = 0; i < this->used_num; i++)  
        {
            if (strcmp(this->station_list[i].name.c_str(), target_name.c_str()) == 0)  // 修复：添加 == 0 比较
            {
                LOG_DEBUG("Found station by name: %s at index %d", target_name.c_str(), i);
                return i;
            }
        }
        LOG_DEBUG("Station with name: %s not found", target_name.c_str());
        return -1;
    }
}


/**
 * @brief    获得当前目标站点索引
 * 
 * @return   uint8_t   当前站点索引
 */
uint8_t StationRepo::Get_Current_Index()
{
    return this->current_index;
}

bool StationRepo::Change_current_index(uint8_t index)
{
    if (index >= this->used_num) return false;
    this->current_index = index;
    return true;
}

/**
 * @brief    添加站点到链表尾部
 * 
 * @param    station   站点信息结构体
 * @return   true      添加成功
 * @return   false     添加失败（如站点已存在或链表已满）
 */
bool StationRepo::Add_Station_to_Tail(const Station_t& station)
{
    if (this->used_num >= N)  // 链表已满
    {
        LOG_WARN("Station list is full, cannot add more stations");
        return false;
    }
    if (this->find_Station(station.name) != -1 
         || this->find_Station(station.name_ch) != -1)  // 站点已存在
    {
        LOG_WARN("Input Station Name is same as station of the list, Please check");
        return false;
    }
    this->station_list[this->used_num] = station;
    this->station_list[this->used_num].isConnnectd = false;
    this->station_list[this->used_num].isProcessed = false;
    this->station_list[this->used_num].lastRSSI = -100;
    this->station_list[this->used_num].lastVisitTime = 0;
    this->station_list[this->used_num].visitCount = 0;
    this->used_num++;
    LOG_INFO("Station %s added to the list successfully", station.name.c_str());
    return true;
}

/**
 * @brief    添加站点到指定索引站点后
 * 
 * @param    station   站点信息结构体
 * @param    index     目标索引位置
 * @return   true      添加成功
 * @return   false     添加失败（如索引无效或链表已满）
 */
bool StationRepo::Add_Station_ByIndex(const Station_t& station, uint8_t index)
{
    if (index >= N || this->used_num >= N)  // 修复：index是uint8_t，不会<0
    {
        LOG_WARN("Invalid index or station list is full, cannot add station");
        return false;
    }
    
    // 如果index大于当前数量，则添加到末尾
    if (index > this->used_num) 
    {
        return Add_Station_to_Tail(station);
    }
    
    // 移动元素腾出空间
    for (uint8_t i = this->used_num; i > index; i--)
    {
        this->station_list[i] = this->station_list[i-1];
    }
    
    // 插入新元素
    this->station_list[this->used_num] = station;
    this->station_list[this->used_num].isConnnectd = false;
    this->station_list[this->used_num].isProcessed = false;
    this->station_list[this->used_num].lastRSSI = -100;
    this->station_list[this->used_num].lastVisitTime = 0;
    this->station_list[this->used_num].visitCount = 0;
    this->used_num++;
    LOG_WARN("Station %s added at index %d successfully", station.name.c_str(), index);
    return true;
}

/**
 * @brief    添加站点到指定名称站点后
 * 
 * @param    station        站点信息结构体
 * @param    target_name    目标站点名称
 * @return   true           添加成功
 * @return   false          添加失败（如目标站点不存在或链表已满）
 */
bool StationRepo::Add_StationToLater_ByString(const Station_t& station, const String& target_name)
{
    if (this->used_num >= N)  // 修复：移除index检查，因为index还没定义
    {
        LOG_WARN("Station list is full, cannot add more stations");
        return false;
    }
    
    int8_t index = this->find_Station(target_name);
    
    if (index != -1)
    {
        return this->Add_Station_ByIndex(station,index);  // 修复：添加右括号，插入到目标后面
    }
    LOG_WARN("Target station %s not found, cannot add station", target_name.c_str());
    return false; 
}

/**
 * @brief    移除指定索引的站点
 * 
 * @param    index     目标站点索引
 * @return   true      移除成功
 * @return   false     移除失败（如索引无效）
 */
bool StationRepo::Remove_Station_ByIndex(uint8_t index)
{
    if(!this->used_num || index >= this->used_num) 
    {
        LOG_WARN("Invalid index or station list is empty, cannot remove station");
        return false;
    }
    
    for (uint8_t i = index; i < this->used_num - 1; i++)
    {
        this->station_list[i] = this->station_list[i + 1];
    }
    this->used_num--;
    LOG_INFO("Station at index %d removed successfully", index);
    return true;
}

/**
 * @brief    移除指定名称的站点
 * 
 * @param    name      目标站点名称
 * @return   true      移除成功
 * @return   false     移除失败（如站点不存在）
 */
bool StationRepo::Remove_Station_ByString(const String& name)
{
    int8_t temp = this->find_Station(name);
    
    if (temp == -1)
    {
        LOG_WARN("Station %s not found, cannot remove station", name.c_str());
        return false;
    }
    else
    {
        return this->Remove_Station_ByIndex(temp);
    }
}

/**
 * @brief    移除链表头部站点
 * 
 * @return   true      移除成功
 * @return   false     移除失败（如链表为空）
 */
bool StationRepo::Remove_Station_Head()
{
    return this->Remove_Station_ByIndex(0);
}

/**
 * @brief    移除链表尾部站点
 * 
 * @return   true      移除成功
 * @return   false     移除失败（如链表为空）
 */
bool StationRepo::Remove_Station_Tail()
{
    if (!this->used_num) 
    {
        LOG_WARN("Station list is empty, cannot remove tail station");
        return false;
    }
    this->used_num--; 
    LOG_INFO("Tail station removed successfully");
    return true;
}

/**
 * @brief    获得当前站点名称
 * 
 * @return   String    当前站点名称，链表为空时返回空字符串
 */
String StationRepo::Get_Current_Station()
{
    if (this->used_num == 0) return String("");
    return this->station_list[this->current_index].name;
}

/**
 * @brief    获得当前站点中文名称
 * 
 * @return   String    当前站点中文名称，链表为空时返回空字符串
 */
String StationRepo::Get_Current_Station_Chinese()
{
    if (this->used_num == 0) return String("");
    return this->station_list[this->current_index].name_ch;
}

/**
 * @brief    移动到下一个站点
 * 
 * @return   true      成功移动到下一个站点
 * @return   false     链表为空，无法移动
 * @attention 环形移动，到达末尾后返回头部
 */
bool StationRepo::Move_To_Next_Station()
{
    if (this->used_num == 0) 
    {
        LOG_WARN("Station list is empty, cannot move to next station");
        return false;
    }
    station_list[this->current_index].isProcessed = true;
    station_list[this->current_index].visitCount += 1;
    station_list[this->current_index].lastVisitTime = millis();
    station_list[this->current_index].isConnnectd = false;
    this->current_index = (this->current_index + 1) % this->used_num;
    LOG_INFO("Moved to next station, current index is now %d", this->current_index);
    return true;
}

/**
 * @brief    移动到上一个站点
 * 
 * @return   true      成功移动到上一个站点
 * @return   false     链表为空，无法移动
 * @attention 环形移动，到达头部后返回末尾
 */
bool StationRepo::Move_To_Previous_Station()
{
    if (this->used_num == 0) 
    {
        LOG_WARN("Station list is empty, cannot move to previous station");
        return false;
    }
    this->current_index = (this->current_index == 0) ? this->used_num - 1 : this->current_index - 1;
    LOG_INFO("Moved to previous station, current index is now %d", this->current_index);
    return true;
}

/**
 * @brief    获得当前站点数量
 * 
 * @return   uint8_t   站点数量
 */
uint8_t StationRepo::Get_Station_Count()
{
    return this->used_num;
}

/**
 * @brief    当前站点链表是否为空
 * 
 * @return   true      链表为空
 * @return   false     链表不为空
 */
bool StationRepo::Is_Empty()
{
    return (this->used_num == 0);
}

/**
 * @brief    标记当前站点为已处理
 * 
 * @return   true      标记成功
 * @return   false     链表为空，标记失败
 */
bool StationRepo::Mark_Current_Processed()
{
    if(this->used_num == 0) return false;
    this->station_list[this->current_index].isProcessed = true;
    return true;
}

/**
 * @brief    当前站点是否已处理
 * 
 * @return   true      当前站点已处理
 * @return   false     当前站点未处理或链表为空
 */
bool StationRepo::Is_Current_Processed()
{
    if (this->used_num == 0)
    {
        LOG_WARN("Station list is empty, cannot check processing status");
        return false;
    }
    return this->station_list[this->current_index].isProcessed;
}

/**
 * @brief    更改指定索引站点的密码
 * 
 * @param    index              目标站点索引
 * @param    new_password       新密码字符串
 * @return   true               更改成功
 * @return   false              索引无效，更改失败
 */
bool StationRepo::Change_Station_Password(uint8_t index, String new_password)
{
    if (index >= this->used_num) 
    {
        LOG_WARN("Invalid index %d, cannot change station password", index);
        return false;
    }
    this->station_list[index].password = new_password;
    LOG_INFO("Password for station at index %d changed successfully", index);
    return true;
}

/**
 * @brief    向串口打印当前站点列表
 * 
 * @param    Serial    Arduino硬件串口对象
 */
void StationRepo::Print_Station_List(HardwareSerial& Serial)
{
    Serial.println("当前站点列表:");
    for (uint8_t i = 0; i < this->used_num; i++)
    {
        Serial.printf("%d: %s (%s) - 已处理: %s\n", 
                        i, 
                        this->station_list[i].name_ch.c_str(), 
                        this->station_list[i].ssid.c_str(),
                        this->station_list[i].isProcessed ? "是" : "否");
    }
}

/**
 * @brief    清理站点列表
 * 
 */
void StationRepo::Clear_Station_List()
{
    this->used_num = 0;
    this->current_index = 0;
    LOG_INFO("Station list cleared successfully");
}

/**
 * @brief    重置所有站点的处理状态为未处理
 * 
 */
void StationRepo::Reset_Processing_Status()
{
    for (uint8_t i = 0; i < this->used_num; i++)
    {
        this->station_list[i].isProcessed = false;
    }
    LOG_INFO("All station processing statuses have been reset to unprocessed");
}

/**
 * @brief    返回站点列表的JSON表示
 * 
 * @return   String    Json字符串
 * @details  返回的JSON格式如下：
 * {
 *   "target_station": "当前站点名称",
 *   "current_sta_status": "true"或"false",
 *   "station_list": [
 *     {
 *       "name": "站点名称",
 *       "name_ch": "站点中文名称",
 *      "ssid": "站点SSID",
 *      "ip": "站点IP地址",
 *      "isProcessed": "true"或"false"
 *    },
 *   ...
 *  ]
 * }
 */
String StationRepo::Get_StationList_JSON()
{
    JsonDocument doc;
    doc["target_station"] = Get_Current_Station_Chinese();
    doc["current_sta_status"] = Is_Current_Processed() ? "true" : "false";
    JsonArray stationArray = doc["station_list"].to<JsonArray>();
    for (uint8_t i = 0; i < this->used_num; i++)
    {
        JsonObject stationObj = stationArray.add<JsonObject>();
        stationObj["name"] = this->station_list[i].name;
        stationObj["name_ch"] = this->station_list[i].name_ch;
        stationObj["ssid"] = this->station_list[i].ssid;
        stationObj["ip"] = this->station_list[i].ip;
        stationObj["isProcessed"] = this->station_list[i].isProcessed ? "true" : "false";
        stationObj["isConnected"] = this->station_list[i].isConnnectd ? "true" : "false";
    }
    String jsonString;
    serializeJson(doc, jsonString);
    return jsonString;
}


/**
 * @brief    获取指定索引的站点信息引用
 * 
 * @param    index          目标站点索引
 * @return   Station_t&     站点信息引用，索引无效时返回静态无效对象引用
 */
Station_t& StationRepo::Get_Index_Station(uint8_t index)
{
    if (index >= used_num) 
    {
        Serial.println("Error: Index out of range.");
        static Station_t invalidStation("","","","",""); // 返回一个静态的无效对象
        return invalidStation;
    }
    return station_list[index];
}


/**
 * @brief    获得指定索引站点名称(中文或英文)
 * 
 * @param    index          目标站点索引
 * @param    is_chinese     是否返回中文名称，默认为false（英文名称）
 * @return   String         站点名称，索引无效时返回空字符串
 */
String StationRepo::Get_Index_Station_Name(uint8_t index,bool is_chinese)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station name", index);
        return String("");
    }
    
    if (is_chinese)
    {
        return this->station_list[index].name_ch;
    }
    else
    {
        return this->station_list[index].name;
    }
}

/**
 * @brief    获得指定索引站点SSID
 * 
 * @param    index     目标站点索引
 * @return   String    站点SSID，索引无效时返回空字符串
 */
String StationRepo::Get_Index_Station_SSID(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station SSID", index);
        return String("");
    }
    return this->station_list[index].ssid;
}

/**
 * @brief    获得指定索引站点密码
 * 
 * @param    index     目标站点索引
 * @return   String    站点密码，索引无效时返回空字符串
 */
String StationRepo::Get_Index_Station_Password(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station password", index);
        return String("");
    }
    return this->station_list[index].password;
}

/**
 * @brief    获得指定索引站点IP地址
 * 
 * @param    index     目标站点索引
 * @return   String    站点IP地址，索引无效时返回空字符串
 */
String StationRepo::Get_Index_Station_IP(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station IP", index);
        return String("");
    }
    return this->station_list[index].ip;
}

/**
 * @brief    获得指定索引站点上次连接的RSSI值
 * 
 * @param    index     目标站点索引
 * @return   int8_t    站点上次连接的RSSI值，索引无效时返回-100
 */
int8_t StationRepo::Get_Index_Station_LastRSSI(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station last RSSI", index);
        return -100;
    }
    return this->station_list[index].lastRSSI;
}

/**
 * @brief    获得指定索引站点上次访问时间戳
 * 
 * @param    index     目标站点索引
 * @return   uint32_t  站点上次访问时间戳，索引无效时返回0
 */
uint32_t StationRepo::Get_Index_Station_LastVisitTime(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station last visit time", index);
        return 0;
    }
    return this->station_list[index].lastVisitTime;
}

/**
 * @brief    获得指定索引站点访问次数
 * 
 * @param    index     目标站点索引
 * @return   int       站点访问次数，索引无效时返回0
 */
int StationRepo::Get_Index_Station_VisitCount(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station visit count", index);
        return 0;
    }
    return this->station_list[index].visitCount;
}

/**
 * @brief    判断指定索引站点是否已处理
 * 
 * @param    index     目标站点索引
 * @return   true      已处理
 * @return   false     未处理
 */
bool StationRepo::is_Processed(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station processed status", index);
        return false;
    }
    return this->station_list[index].isProcessed;
}

/**
 * @brief    判断指定索引站点是否已连接
 * 
 * @param    index     目标站点索引
 * @return   true      已连接
 * @return   false     未连接
 */
bool StationRepo::is_Connected(uint8_t index)
{
    if (index >= used_num)
    {
        LOG_WARN("Invalid index %d, cannot get station connected status", index);
        return false;
    }
    return this->station_list[index].isConnnectd;
}
