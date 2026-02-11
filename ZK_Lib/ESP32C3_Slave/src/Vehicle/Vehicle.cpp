#include "Vehicle.hpp"
#include "../StaticAllocator/StaticAllocator.hpp"


String Vehicle_Info::Get_Vehicle_Plate()
{
    return this->vehicle.Plate;
}
String Vehicle_Info::Get_Vehicle_SSID()
{
    return this->vehicle.SSID;
}
String Vehicle_Info::Get_Vehicle_Password()
{
    return this->vehicle.Password;
}
String Vehicle_Info::Get_Vehicle_StationServer()
{
    return this->vehicle.StationServer;
}
Rounter Vehicle_Info::Get_Vehicle_Rounter()
{
    return this->vehicle.currentRoute;  
}
VehicleStatus Vehicle_Info::Get_Vehicle_Status()
{
    return this->vehicle.status;
}
String Vehicle_Info::Get_Status_Str(Vehicle_t vehicle)
{
    switch (vehicle.status)
    {
    case VehicleStatus::STATUS_SCANNING:
        return "SCANNING";
    case VehicleStatus::STATUS_IDLE:
        return "IDLE";
    case VehicleStatus::STATUS_GROPE:
        return "GROPE";
    case VehicleStatus::STATUS_CONNECTING:
        return "CONNECTING";
    case VehicleStatus::STATUS_CONNECTED:
        return "CONNECTED";
    case VehicleStatus::STAUS_DISCONNECTED:
        return "DISCONNECTED";
    case VehicleStatus::STATUS_WAITING:
        return "waiting";
    case VehicleStatus::STATUS_ARRIVING:
        return "arriving";
    case VehicleStatus::STATUS_LEAVING:
        return "leaving";
    default:
        return "UNKNOWN";
    }
}


String Vehicle_Info::Get_Status_Str(VehicleStatus status)
{
    switch (status)
    {
    case VehicleStatus::STATUS_SCANNING:
        return "SCANNING";
    case VehicleStatus::STATUS_IDLE:
        return "IDLE";
    case VehicleStatus::STATUS_GROPE:
        return "GROPE";
    case VehicleStatus::STATUS_CONNECTING:
        return "CONNECTING";
    case VehicleStatus::STATUS_CONNECTED:
        return "CONNECTED";
    case VehicleStatus::STAUS_DISCONNECTED:
        return "DISCONNECTED";
    case VehicleStatus::STATUS_WAITING:
        return "waiting";
    case VehicleStatus::STATUS_ARRIVING:
        return "arriving";
    case VehicleStatus::STATUS_LEAVING:
        return "leaving";
    default:
        return "UNKNOWN";
    }
}

bool Vehicle_Info::Update_Vehicle_Status(VehicleStatus new_status)
{
    if (this->vehicle.status != new_status)
    {
        this->vehicle.last_status = this->vehicle.status;
        this->vehicle.status = new_status;
        LOG_INFO("Vehicle_Info: 车辆状态变更: %s -> %s\n", Get_Status_Str(this->vehicle.last_status).c_str(), Get_Status_Str(this->vehicle.status).c_str());
        return true;
    }
    return false;
}

String Vehicle_Info::Vehiicle_Json()
{
    JsonDocument doc;
    doc["Plate"] = this->vehicle.Plate;
    doc["Router"] = static_cast<int>(this->vehicle.currentRoute);
    doc["Status"] = Get_Status_Str(this->vehicle.status);
    String json_str;
    serializeJson(doc, json_str);
    return json_str;
}


size_t Vehicle_Info::Get_Vehicle_Plate(char* buffer, size_t buffer_size)
{
    LOG_ASSERT(this->vehicle.Plate != nullptr);
    StaticBufferError err = check_pool(buffer, buffer_size, strlen(this->vehicle.Plate) + 1);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取车牌号失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", strlen(this->vehicle.Plate) + 1, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    buffer = strncpy(buffer, this->vehicle.Plate, buffer_size - 1);
    buffer[buffer_size - 1] = '\0'; // 确保字符串以null结尾
    return strlen(buffer);
}

size_t Vehicle_Info::Get_Vehicle_SSID(char* buffer, size_t buffer_size)
{
    LOG_ASSERT(this->vehicle.SSID != nullptr);
    StaticBufferError err = check_pool(buffer, buffer_size, strlen(this->vehicle.SSID) + 1);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取ssid号失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", strlen(this->vehicle.SSID) + 1, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    buffer = strncpy(buffer, this->vehicle.SSID, buffer_size - 1);
    buffer[buffer_size - 1] = '\0'; // 确保字符串以null结尾
    return strlen(buffer);
}

size_t Vehicle_Info::Get_Vehicle_Password(char* buffer, size_t buffer_size)
{
    LOG_ASSERT(this->vehicle.Password != nullptr);
    StaticBufferError err = check_pool(buffer, buffer_size, strlen(this->vehicle.Password) + 1);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取密码失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", strlen(this->vehicle.Password) + 1, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    buffer = strncpy(buffer, this->vehicle.Password, buffer_size - 1);
    buffer[buffer_size - 1] = '\0'; // 确保字符串以null结尾
    return strlen(buffer);
}

size_t Vehicle_Info::Get_Vehicle_StationServer(char* buffer, size_t buffer_size)
{
    LOG_ASSERT(this->vehicle.StationServer != nullptr);
    StaticBufferError err = check_pool(buffer, buffer_size, strlen(this->vehicle.StationServer) + 1);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取IP失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", strlen(this->vehicle.StationServer) + 1, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    buffer = strncpy(buffer, this->vehicle.StationServer, buffer_size - 1);
    buffer[buffer_size - 1] = '\0'; // 确保字符串以null结尾
    return strlen(buffer);
}

size_t Vehicle_Info::Get_Status_Str(char* buffer, size_t buffer_size, VehicleStatus status)
{
    StaticBufferError err = check_pool(buffer, buffer_size, MAX_VEHICLE_STATUS_STRING_LENGTH);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取状态失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", MAX_VEHICLE_STATUS_STRING_LENGTH, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    switch (status)
    {
    case VehicleStatus::STATUS_SCANNING:
        return snprintf(buffer, buffer_size, "SCANNING");
    case VehicleStatus::STATUS_IDLE:
        return snprintf(buffer, buffer_size, "IDLE");
    case VehicleStatus::STATUS_GROPE:
        return snprintf(buffer, buffer_size, "GROPE");
    case VehicleStatus::STATUS_CONNECTING:
        return snprintf(buffer, buffer_size, "CONNECTING");
    case VehicleStatus::STATUS_CONNECTED:
        return snprintf(buffer, buffer_size, "CONNECTED");
    case VehicleStatus::STAUS_DISCONNECTED:
        return snprintf(buffer, buffer_size, "DISCONNECTED");
    case VehicleStatus::STATUS_WAITING:
        return snprintf(buffer, buffer_size, "waiting");
    case VehicleStatus::STATUS_ARRIVING:
        return snprintf(buffer, buffer_size, "arriving");
    case VehicleStatus::STATUS_LEAVING:
        return snprintf(buffer, buffer_size, "leaving");
    default:
        return snprintf(buffer, buffer_size, "UNKNOWN");
    }
}

size_t Vehicle_Info::Get_Status_Str(char* buffer, size_t buffer_size, Vehicle_t vehicle)
{
    return Get_Status_Str(buffer, buffer_size, vehicle.status);
}

size_t Vehicle_Info::Vehiicle_Json(char* buffer, size_t buffer_size)
{
    StaticBufferError err = check_pool(buffer, buffer_size, MAX_VEHICLE_JSON_LENGTH);
    if (err != StaticBufferError::OK)
    {
        LOG_WARN("Vehicle_Info: 获取车辆JSON失败，缓冲区大小不足。所需大小: %d, 提供大小: %d", MAX_VEHICLE_JSON_LENGTH, buffer_size);
        LOG_ASSERT(false); // 断言失败，提示开发者修正缓冲区大小
        return 0;
    }
    static uint8_t json_buffer[MAX_VEHICLE_JSON_LENGTH]; // 静态分配JSON缓冲区
    StaticAllocator allocate(json_buffer, sizeof(json_buffer)); // 使用静态分配器

    JsonDocument doc(&allocate);

    char status_str[32];
    Get_Status_Str(status_str, sizeof(status_str), this->vehicle.status);
    doc["Plate"] = this->vehicle.Plate;
    doc["Router"] = static_cast<int>(this->vehicle.currentRoute);
    doc["Status"] = status_str;
    return serializeJson(doc, buffer, buffer_size);
}