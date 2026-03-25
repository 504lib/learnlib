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
  double get temperature => _temperature;
  double get humidity => _humidity;
  double get mq4 => _mq4;
  double get tempratureThreshold => _tempratureThreshold;
  double get humidityThreshold => _humidityThreshold;
  double get mq4Threshold => _mq4Threshold;
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
}