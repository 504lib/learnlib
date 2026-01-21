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
}