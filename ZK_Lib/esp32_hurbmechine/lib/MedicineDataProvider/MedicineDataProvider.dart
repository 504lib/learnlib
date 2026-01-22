import '../httpService/httpService.dart';
import 'package:flutter/foundation.dart';
import 'dart:convert';
import 'package:http/http.dart' as http;
import 'dart:math';
///
///json格式如下
///{
//   "system": {
//     "current_weight": "12.3",
//     "target_weight": 100,
//     "system_state": 0,        // 0等待,1运行,2完成
//     "medicine_type": 0,       // 0麻黄,1桂枝
//     "motor_ready": true,
//     "queue_paused": false
//   },
//   "queue": {
//     "queue_count": 2,
//     "current_user": "USER_ABC",
//     "queue_list": [
//       "USER_ABC (麻黄 - 100g)",
//       "USER_DEF (桂枝 - 80g)"
//     ]
//   }
// }
class Medicinedataprovider extends ChangeNotifier {
  String currentWeight = "0.0";              // 当前药物重量
  String targetWeight = "0.0";               // 目标药物重量
  String systemStatus = "等待中";            // 系统状态
  String medicineType = "未知";              // 药物类型
  bool isMotorRunning = false;              // 电机运行状态
  bool isQueuePaused = false;               // 队列暂停状态

  // 服务端队列中的 code 列表（仅来自 queue_list）
  List<String> localJoined = [];

  // 本客户端本地生成并提交过的 code 以及其药品类型映射
  final List<String> myCodes = [];
  final Map<String, int> myCodeMedicine = {};

  List<String> medicineQueue = [];         // 药物队列
  String? currentUser;                       // 当前用户         

  bool connected = true;

  Future<void> syncFromServer() async {
    await fetchMedicineData();
  }

  Future<bool> fetchMedicineData() async {
    bool isCanConnect = true;
    try{
      final http.Response response = await HttpService().getRequest("/api/status").timeout(const Duration(seconds: 3));
      if(response.statusCode != 200){
        isCanConnect = false;  
        return false;
      }
      final Map<String, dynamic> data = json.decode(response.body);
      final Map<String,dynamic> systemData = (data['system'] as Map?)?.cast<String,dynamic>() ?? {};
      final Map<String,dynamic> queueData = (data['queue'] as Map?)?.cast<String,dynamic>() ?? {};
      currentWeight = systemData['current_weight']?.toString() ?? "0.0";
      targetWeight = systemData['target_weight']?.toString() ?? "0.0";
      systemStatus = _getSystemStatusText(systemData['system_state'] ?? -1);
      medicineType = _getMedicineTypeText(systemData['medicine_type'] ?? -1);
      isMotorRunning = systemData['motor_ready'] ?? false;
      isQueuePaused = systemData['queue_paused'] ?? false;

      currentUser = (queueData['current_user'] as String?);
      medicineQueue = (queueData['queue_list'] as List?)?.cast<String>() ?? [];

      _refreshLocalJoinedFromQueue();

      return true;
    }catch(e){
      debugPrint("Error fetching medicine data: $e");
      isCanConnect = false;
      return false;
    }finally{
      connected = isCanConnect;
      notifyListeners();
    }
  }

  void _refreshLocalJoinedFromQueue() {
    final List<String> next = [];
    for (final entry in medicineQueue) {
      final code = _extractCode(entry);
      if (code.isNotEmpty && !next.contains(code)) {
        next.add(code);
      }
    }
    localJoined = next;
  }

  String _extractCode(String entry) {
    final idx = entry.indexOf(' ');
    if (idx > 0) return entry.substring(0, idx);
    final bracket = entry.indexOf('(');
    if (bracket > 0) return entry.substring(0, bracket).trim();
    return entry.trim();
  }
  String _getSystemStatusText(int statusCode) {
    switch (statusCode) {
      case 0:
        return "等待中";
      case 1:
        return "运行中";
      case 2:
        return "完成";
      default:
        return "未知状态";
    }
  }
  String _getMedicineTypeText(int typeCode) {
    switch (typeCode) {
      case 0:
        return "麻黄";
      case 1:
        return "桂枝";
      default:
        return "未知药物";
    }
  }
  String generateClientCode() {
    final rand = Random();
    final buf = StringBuffer();
    for (var i = 0; i < 5; i++) {
      buf.write(rand.nextInt(36).toRadixString(36));
    }
    final clientCode = "USER_${buf.toString().toUpperCase()}";
    notifyListeners();
    return clientCode;
  }

  void recordLocalJoin(String code, int medicineType) {
    if (!myCodes.contains(code)) myCodes.add(code);
    myCodeMedicine[code] = medicineType;
    notifyListeners();
  }

  List<String> get availableLeaveCodes =>
      myCodes.where((c) => localJoined.contains(c)).toList(growable: false);

  int getMedicineForCode(String code) => myCodeMedicine[code] ?? 0;
}