import 'dart:convert';
import 'package:flutter/widgets.dart';
import 'package:http/http.dart' as http;

void testconnect() async
{
  final url = Uri.parse('http://iot-api.heclouds.com/thingmodel/query-device-property?product_id=82aJXC88GG&device_name=DHT11');
  final header = {
    'Authorization': 'version=2022-05-01&res=products%2F82aJXC88GG&et=2058153538&method=md5&sign=k8cGxQXiEUnM4qfuownJcw%3D%3D',
    'Content-Type': 'application/json',
  };

  final response = await http.get(url, headers: header);
  if (response.statusCode == 200) {
    debugPrint('received data: ${response.body}\n');
    final Map<String, dynamic> data = Map<String, dynamic>.from(jsonDecode(response.body));
    final int code = data['code'] ?? -1;
    debugPrint('received data: ${response.body}\n');
    if(code == 0)
    {
      final dynamic propertiesData = data['data'] ?? {};
      if (propertiesData is! List) {
        debugPrint('Unexpected data format: properties should be a list');
        return;
      }
      for (var property in propertiesData) {
        final String propertyName = property['identifier'] ?? 'Unknown';
        final dynamic propertyValue = property['value'] ?? 'Unknown';
        debugPrint('Property: $propertyName, Value: $propertyValue');
      }
    }
    else if(code == 10421)
    {
      debugPrint("设备离线\n");
    }
    else if(code == 10410)
    {
      debugPrint("设备不存在\n");
    }
    else
    {
      debugPrint('Error fetching properties. Code: $code, Message: ${data['message'] ?? 'No message'}');
    }
  }
}

class HttpToOneNet{
    final String productId;
    final String deviceName;
    final String token;
    Map<String,dynamic>? propertyData;
    HttpToOneNet({required this.productId, required this.deviceName,required this.token});

    String get _urlStr => 'http://iot-api.heclouds.com/thingmodel/query-device-property?product_id=$productId&device_name=$deviceName';
    Map<String, String> get header => {
      'Authorization': token,
      'Content-Type': 'application/json',
    };
    String _buildControlBody({
      required String productId,
      required String deviceName,
      required Map<String, dynamic> params,
    }) {
      return jsonEncode({
        'product_id': productId,
        'device_name': deviceName,
        'params': params,
      });
    }
    Future<bool> updataPropertyData() async{
      final url = Uri.parse(_urlStr);
      try {
        final response = await http.get(url, headers: header);
        if (response.statusCode == 200) {
          // debugPrint('received data: ${response.body}\n');
          propertyData = Map<String,dynamic>.from(jsonDecode(response.body));
          final int code = propertyData?['code'] ?? -1;
          // debugPrint("code: $code\n");
          if(code != 0)
          {
            debugPrint('Error fetching properties. Code: $code, Message: ${propertyData?['message'] ?? 'No message'}');
            return false;
          }
          final dynamic rawData = propertyData?['data'] ?? {};
          if (rawData is! List) {
            debugPrint('Unexpected data format: properties should be a list');
            return false;
          }
          // debugPrint('Raw properties data: $rawData\n');  
          final List<dynamic> propertiesData = rawData;
          for(var item in propertiesData)
          {
            final String identifier = item['identifier'] ?? 'Unknown';
            final dynamic value = item['value'] ?? 'Unknown';
            propertyData?[identifier] = value;
          }
          return true;
        } else {
          debugPrint('Failed to fetch properties. Status code: ${response.statusCode}');
          return false;
        }
      } catch (e) {
        debugPrint('Error fetching properties: $e');
        return false;
      }
    }
    Future<Map<String, dynamic>?> fetchProperties({required String field}) async {
      if(propertyData != null)
      {
        return propertyData;
      }
      return null;
    }
    Future<bool> sendControlCommand({required String field, required dynamic value}) async
    {
      if (value is! double && value is! int && value is! String && value is! bool) {
        debugPrint("Unsupported value type: {value.runtimeType}");
        return Future.value(false);
      }
      final body = _buildControlBody(productId: productId, deviceName: deviceName, params: {field: value});
      final url = Uri.parse('http://iot-api.heclouds.com/thingmodel/set-device-property');
      final response = await http.post(url, headers: header, body: body);
      if (response.statusCode == 200) {
        try {
          final Map<String, dynamic> respData = Map<String, dynamic>.from(jsonDecode(response.body));
          final int code = respData['code'] ?? -1;
          if (code == 0) {
            debugPrint("Control command sent successfully");
            return true;
          } else {
            debugPrint("Control command failed. Code: $code, Message: {respData['msg'] ?? 'No message'}");
            return false;
          }
        } catch (e) {
          debugPrint("Failed to parse control response: $e");
          return false;
        }
      } else {
        debugPrint("Failed to send control command. Status code: {response.statusCode}");
        return false;
      }
    }
      /// 直接访问OneNet接口，实时获取指定bool字段的值
      Future<bool?> fetchBoolFieldDirect({required String field}) async {
        final url = Uri.parse(_urlStr);
        try {
          final response = await http.get(url, headers: header);
          if (response.statusCode == 200) {
            final Map<String, dynamic> data = Map<String, dynamic>.from(jsonDecode(response.body));
            final int code = data['code'] ?? -1;
            if (code != 0) {
              return null;
            }
            final dynamic rawData = data['data'] ?? {};
            if (rawData is! List) {
              debugPrint('Unexpected data format: properties should be a list');
              return null;
            }
            for (var item in rawData) {
              if (item['identifier'] == field) {
                final dynamic value = item['value'];
                if (value is bool) {
                  return value;
                } else if (value is String) {
                  return value.toLowerCase() == 'true';
                } else {
                  debugPrint('Unexpected value type for $field: {value.runtimeType}');
                  return null;
                }
              }
            }
            debugPrint('Field $field not found in properties');
            return null;
          } else {
            debugPrint('Failed to fetch properties. Status code: {response.statusCode}');
            return null;
          }
        } catch (e) {
          debugPrint('Error fetching properties: $e');
          return null;
        }
      }

 }