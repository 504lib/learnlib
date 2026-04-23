import "package:another_project_apk/http_to_onenet/http_to_onenet.dart";
import "package:flutter/rendering.dart";
import 'package:flutter/material.dart';

class DataProvider extends ChangeNotifier{
  double _temperature = 0.0;
  double _humidity = 0.0;
  double _mq4 = 0.0;
  double _tempratureThreshold = 0.0;
  double _humidityThreshold = 0.0;
  double _mq4Threshold = 0.0;
  bool _motorOn = false;
  bool _bulbOn = false;
  int _lightVal = 0;
  bool get bulbOn => _bulbOn;
  bool get motorOn => _motorOn;
  double get temperature => _temperature;
  double get humidity => _humidity;
  double get mq4 => _mq4;
  double get tempratureThreshold => _tempratureThreshold;
  double get humidityThreshold => _humidityThreshold;
  double get mq4Threshold => _mq4Threshold;
  int get lightVal => _lightVal;
  DataProvider();

  set temperature(double value) {
    _temperature = value;
    notifyListeners();
  }
  set humidity(double value) {
    _humidity = value;
    notifyListeners();
  }
  set mq4(double value) {
    _mq4 = value;
    notifyListeners();
  }
  set tempratureThreshold(double value) {
    _tempratureThreshold = value;
    notifyListeners();
  }
  set humidityThreshold(double value) {
    _humidityThreshold = value;
    notifyListeners();
  }
  set mq4Threshold(double value) {
    _mq4Threshold = value;
    notifyListeners();
  }
  set motorOn(bool value) {
    _motorOn = value;
    notifyListeners();
  }
  set bulbOn(bool value) {
    _bulbOn = value;
    notifyListeners();
  }
  set lightVal(int value) {
    _lightVal = value;
    notifyListeners();
  }
  Future<double> getDataFromOneNet({required HttpToOneNet httpToOneNet,required String field}) async {
    bool ok = await httpToOneNet.updataPropertyData();
    if(!ok)
    {
      debugPrint("数据更新失败\n");
      return 0.0;
    }
    var value = await httpToOneNet.fetchProperties(field: field);
    var temp = value?[field];
    // debugPrint("Fetched temp: $temp\n");
    if (temp is num) {
      return temp.toDouble();
    }
    else if(temp is String) {
      return double.tryParse(temp) ?? 0.0;
    } else {
      debugPrint("Unexpected temp value type: ${temp.runtimeType}");
      return 0.0;
    }
  }
  Future<bool> getBoolDataFromOneNet({required HttpToOneNet httpToOneNet, required String field}) async {
    bool ok = await httpToOneNet.updataPropertyData();
    if(!ok)
    {
      debugPrint("数据更新失败\n");
      return false;
    }
    var value = await httpToOneNet.fetchProperties(field: field);
    var temp = value?[field];
    // debugPrint("Fetched bool temp: $temp\n");
    if (temp is bool) {
      return temp;
    } else if (temp is String) {
      return temp.toLowerCase() == 'true';
    } else {
      debugPrint("Unexpected value type for boolean: ${temp.runtimeType}");
      return false;
    }
  }
  Future<int> getIntDataFromOneNet({required HttpToOneNet httpToOneNet, required String field}) async {
    bool ok = await httpToOneNet.updataPropertyData();
    if(!ok)
    {
      debugPrint("数据更新失败\n");
      return 0;
    }
    var value = await httpToOneNet.fetchProperties(field: field);
    var temp = value?[field];
    // debugPrint("Fetched int temp: $temp\n");
    if (temp is num) {
      return temp.toInt();
    } else if (temp is String) {
      return int.tryParse(temp) ?? 0;
    } else {
      debugPrint("Unexpected value type for int: ${temp.runtimeType}");
      return 0;
    }
  }
  Future<bool> fetchStatusWithRetry(HttpToOneNet httpToOneNet,{required String field,int retry = 3}) async {
    for (int i = 0; i < retry; i++) {
      await Future.delayed(Duration(milliseconds: 200));
      final res = await httpToOneNet.fetchBoolFieldDirect(field: field);
      if (res != null) return res;
    }
    return false; // 或返回上一次的状态
  }
  String lightValToString(int lightVal) {
    switch (lightVal) {
      case 0:
        return "亮度过大";
      case 1:
        return "明亮";
      case 2:
        return "较暗";
      case 3:
        return "亮度过小";
      default:
        return "未知亮度";
    }
  }
}