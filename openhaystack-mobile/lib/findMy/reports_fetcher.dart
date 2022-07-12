import 'dart:convert';

import 'package:http/http.dart' as http;
import 'package:pointycastle/api.dart';

class ReportsFetcher {
  static const _seemooEndpoint = "http://192.168.1.168/getAndDecryptReports";

  /// Fetches the location reports corresponding to the given hashed advertisement
  /// key.
  /// Throws [Exception] if no answer was received.
  static Future<List> fetchLocationReports(String hashedAdvertisementKey, String base64privateKey) async {
    print("Advertisement Key: ${hashedAdvertisementKey}");
    print("Private Key: ${base64privateKey}");
    // print("fetching reports for $hashedAdvertisementKey");
    final response = await http.post(Uri.parse(_seemooEndpoint),
        headers: <String, String>{
          "Content-Type": "application/json",
        },
        body: jsonEncode(<String, dynamic>{
          "ids": [{"id":hashedAdvertisementKey, "privateKey": base64privateKey}],
        }));
    print("StatusCode: ${response.statusCode}");

    if (response.statusCode == 200) {
      final resultt = await jsonDecode(response.body);
      // print(resultt);
      return resultt;
    } else {
      throw Exception("Failed to fetch location reports with statusCode:${response.statusCode}\n\n Response:\n${response.body}");
    }
  }
}
