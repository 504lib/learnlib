import "dart:convert";
import "dart:io";
import "dart:async";

import "package:flutter/foundation.dart";
import "package:mqtt_client/mqtt_client.dart";
import "package:mqtt_client/mqtt_server_client.dart";

class MqttConnect {
  final String broker;
  final int port;
  final String productIdentifier;
  final String deviceIdentifier;
  final String token;
  final MqttServerClient client;
  StreamSubscription<List<MqttReceivedMessage<MqttMessage>>>? _updatesSubscription;
  bool _subscribed = false;

  MqttConnect(this.broker, this.port, this.productIdentifier, this.deviceIdentifier, this.token)
    : client = MqttServerClient(broker, deviceIdentifier) {
    // mqtt_client defaults to MQTT 3.1; OneNET expects MQTT 3.1.1.
    client.setProtocolV311();
    client.keepAlivePeriod = 30;
    client.connectTimeoutPeriod = 10000;

    // OneNET mqtts host expects TLS.
    final useTls = broker.startsWith("mqtts.");
    client.secure = useTls;
    if (useTls) {
      client.securityContext = SecurityContext.defaultContext;
      client.onBadCertificate = (Object _) => true;
    }
  }

  String get onenetTopicPropPost => "\$sys/$productIdentifier/$deviceIdentifier/thing/property/post";
  String get onenetTopicPropSet => "\$sys/$productIdentifier/$deviceIdentifier/thing/property/set";
  String get onenetTopicPropPostReply => "\$sys/$productIdentifier/$deviceIdentifier/thing/property/reply";
  String get onenetTopicPropSetReply => "\$sys/$productIdentifier/$deviceIdentifier/thing/property/set_reply";

  Map<String, dynamic> buildParamTemplate({
    required int id,
    Map<String, dynamic>? params,
    String version = "1.0",
  }) {
    return {
      "id": id,
      "version": version,
      "params": params ?? <String, dynamic>{},
    };
  }

  Iterable<int> get connectionPorts sync* {
    yield port;
    if (broker.startsWith("mqtts.") && port == 1883) {
      yield 8883;
    }
  }

  Future<bool> connect() async {
    if (client.connectionStatus?.state == MqttConnectionState.connected) {
      return true;
    }

    client.logging(on: true);
    final connMess = MqttConnectMessage()
        .withClientIdentifier(deviceIdentifier)
        .startClean();
    Object? lastError;

    for (final candidatePort in connectionPorts) {
      client.port = candidatePort;
      client.connectionMessage = connMess;
      debugPrint('MQTT::connecting to $broker:$candidatePort, clientId=$deviceIdentifier');
      try {
        // OneNET commonly expects username=productIdentifier and password=token.
        await client.connect(productIdentifier, token).timeout(const Duration(seconds: 10));
      } on Exception catch (e) {
        lastError = e;
        debugPrint('MQTT::connect exception - $e');
        client.disconnect();
      } on Object catch (e) {
        lastError = e;
        debugPrint('MQTT::connect error - $e');
        client.disconnect();
      }

      if (client.connectionStatus?.state == MqttConnectionState.connected) {
        debugPrint('MQTT::connected');
        return true;
      }

      debugPrint('MQTT::connect failed, state=${client.connectionStatus?.state}');
      client.disconnect();
    }

    if (lastError != null) {
      debugPrint('MQTT::all connection attempts failed, lastError=$lastError');
    }
    return false;
  }

  Future<bool> connectAndSubscribe({void Function(Map<String, dynamic>)? onSetCommand}) async {
    final connected = await connect();
    if (!connected) {
      return false;
    }
    subscribeToDevice(onSetCommand: onSetCommand);
    return true;
  }

  Future<String> sendToDevice(String name, String value) async{
    client.logging(on: true);
    debugPrint('MQTT::send flow connecting...');
    final connected = await connect();
    if (connected) {
      debugPrint('MQTT::send flow connected');
      final builder = MqttClientPayloadBuilder();
      builder.addString(jsonEncode(buildParamTemplate(id: 1, params: {name: value})));
      client.publishMessage(onenetTopicPropPost, MqttQos.atLeastOnce, builder.payload!);
      debugPrint('MQTT::message published to $onenetTopicPropPost');
      return "发送成功";
    }
    debugPrint('MQTT::send flow connect failed, state=${client.connectionStatus?.state}');
    client.disconnect();
    return "连接失败";
  }

  void subscribeToDevice({void Function(Map<String, dynamic>)? onSetCommand}) {
    if (_subscribed) {
      return;
    }

    client.subscribe(onenetTopicPropSet, MqttQos.atLeastOnce);
    client.subscribe(onenetTopicPropPostReply, MqttQos.atLeastOnce);
    _subscribed = true;

    _updatesSubscription ??= client.updates?.listen(
      (List<MqttReceivedMessage<MqttMessage>> messages) {
        for (final message in messages) {
          final payloadMessage = message.payload as MqttPublishMessage;
          final payload = MqttPublishPayload.bytesToStringAsString(payloadMessage.payload.message);
          debugPrint('MQTT::received message: topic=${message.topic}, payload=$payload');

          if (message.topic == onenetTopicPropSet) {
            final decoded = _tryDecodeJsonObject(payload);
            if (decoded == null) {
              _publishSetReply(id: 0, code: 400, msg: 'invalid json');
              continue;
            }

            onSetCommand?.call(decoded);
            final dynamic idValue = decoded['id'];
            final int id = int.tryParse('$idValue') ?? 0;
            _publishSetReply(id: id, code: 200, msg: 'success');
          }
        }
      },
      onError: (Object e) {
        debugPrint('MQTT::updates stream error - $e');
      },
      onDone: () {
        debugPrint('MQTT::updates stream closed');
        _updatesSubscription = null;
        _subscribed = false;
      },
    );
  }

  Map<String, dynamic>? _tryDecodeJsonObject(String text) {
    try {
      final dynamic result = jsonDecode(text);
      if (result is Map<String, dynamic>) {
        return result;
      }
      return null;
    } on FormatException {
      return null;
    }
  }

  void _publishSetReply({required int id, required int code, required String msg}) {
    final payload = {
      'id': '$id',
      'code': code,
      'msg': msg,
    };
    final builder = MqttClientPayloadBuilder();
    builder.addString(jsonEncode(payload));
    client.publishMessage(onenetTopicPropSetReply, MqttQos.atLeastOnce, builder.payload!);
    debugPrint('MQTT::set reply published to $onenetTopicPropSetReply: ${jsonEncode(payload)}');
  }
}