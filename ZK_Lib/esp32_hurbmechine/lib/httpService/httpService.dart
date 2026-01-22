import 'package:flutter/material.dart';
import 'package:http/http.dart' as http;

class HttpService{
  final String _baseUrl = "http://192.168.4.1";

  Future<http.Response> getRequest(String endpoint) async {
    final url = Uri.parse('$_baseUrl$endpoint');
    final response = await http.get(url);
    return response;
  }

  Future<http.Response> postRequest(String endpoint, Map<String, dynamic> data) async {
    final url = Uri.parse('$_baseUrl$endpoint');
    final response = await http.post(url, body: data);
    return response;
  }

  Future<String> getDataFromServer([String endpoint = "/api/status"]) async {
    final response = await getRequest(endpoint);
    if (response.statusCode == 200) {
      return response.body;
    } else {
      return 'Error: ${response.statusCode}';
    }
  }

  Future<bool> sendJoinQueue(String userId, int medicineType, int targetWeight) async {
    try {
      final uri = Uri.parse('$_baseUrl/api/join_queue').replace(
        queryParameters: {
          'user_id': userId,
          'weight': targetWeight.toString(),
          'medicine': medicineType.toString(),
        },
      );
      final response = await http.get(uri);
      return response.statusCode == 200;
    } catch (e) {
      debugPrint("sendJoinQueue error: $e");
      return false;
    }
  }
  Future<bool> sendLeaveQueue(String userId, int medicineType) async {
    try {
      final uri = Uri.parse('$_baseUrl/api/leave_queue').replace(
        queryParameters: {
          'user_id': userId,
          'medicine': medicineType.toString(),
        },
      );
      final response = await http.get(uri);
      return response.statusCode == 200;
    } catch (e) {
      debugPrint("sendLeaveQueue error: $e");
      return false;
    }
  }
}