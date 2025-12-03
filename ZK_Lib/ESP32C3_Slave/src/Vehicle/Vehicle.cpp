#include "Vehicle.hpp"


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