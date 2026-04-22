import 'dart:async';
import 'package:another_project_apk/Data/Data.dart';
import 'package:another_project_apk/http_to_onenet/http_to_onenet.dart';
import 'package:flutter/material.dart';
import 'package:another_project_apk/CardStatus/CardStatusCard.dart';
import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';

final GoRouter _goRouter = GoRouter(
  routes: [
     GoRoute(
      path: "/",
      builder: (context, state) => const MyHomePage(
        title: "智能家居",
      ),
    )
  ],
);

void main() {
  runApp(
    ChangeNotifierProvider(
      create: (_) => DataProvider(),
      child: const MyApp(),
    )
  );
}



class MyApp extends StatelessWidget {
  const MyApp({super.key});

  // This widget is the root of your application.
  @override
  Widget build(BuildContext context) {
    return MaterialApp.router(
      title: '智能家居',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: const Color.fromARGB(255, 45, 45, 120)),
      ),
      routerConfig: _goRouter,
    );
  }
}

class MyHomePage extends StatefulWidget {
  const MyHomePage({super.key, required this.title});
  final String title;

  @override
  State<MyHomePage> createState() => _MyHomePageState();
}

class _MyHomePageState extends State<MyHomePage> {
  Timer? timer;
  final HttpToOneNet httpToOneNet = HttpToOneNet(
    productId: '82aJXC88GG',
    deviceName: "DHT11",
    token: "version=2022-05-01&res=products%2F82aJXC88GG&et=2058153538&method=md5&sign=k8cGxQXiEUnM4qfuownJcw%3D%3D"
  );
  late final ValueNotifier<String> tempNotifier; 
  late final ValueNotifier<String> humidityNotifier;
  late final ValueNotifier<String> mq4Notifier;
  late final ValueNotifier<String> motorOnNotifier;
  late final ValueNotifier<String> bulbOnNotifier;  
  late final ValueNotifier<String> lightValNotifier;
  bool isUserMotorOn = false;
  final List<Map<String, dynamic>> motorOptions = [
    {'label': '开', 'value': true, 'icon': Icons.toggle_on},
    {'label': '关', 'value': false, 'icon': Icons.toggle_off},
  ];
  final List<Map<String, dynamic>> bulbOptions = [
    {'label': '开', 'value': true, 'icon': Icons.light_mode},
    {'label': '关', 'value': false, 'icon': Icons.light_mode_outlined},
  ];
  bool isUserBulbOn = false;
  @override
  void initState() {
    super.initState();
    final dataProvider = Provider.of<DataProvider>(context, listen: false);
    tempNotifier = ValueNotifier(dataProvider.temperature.toString());
    humidityNotifier = ValueNotifier(dataProvider.humidity.toString());
    mq4Notifier = ValueNotifier(dataProvider.mq4.toString());
    motorOnNotifier = ValueNotifier(dataProvider.motorOn ? "ON" : "OFF");
    bulbOnNotifier = ValueNotifier(dataProvider.bulbOn ? "ON" : "OFF");
    lightValNotifier = ValueNotifier(dataProvider.lightValToString(dataProvider.lightVal));
    timer = Timer.periodic(const Duration(milliseconds: 500), (t) async{
        double tempVal = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "temp");
        double humidityVal = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "humi");
        double mq4Val = await dataProvider.getDataFromOneNet(httpToOneNet:httpToOneNet, field: "mq4");
        double tempThreshold = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "temp_threshold");
        double humidityThreshold = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "humi_threshold");
        double mq4Threshold = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "mq4_threshold");
        int lightVal = await dataProvider.getIntDataFromOneNet(httpToOneNet: httpToOneNet, field: "light_degree");
        bool isMotorOn = await httpToOneNet.fetchBoolFieldDirect(field: "Motor") ?? false;
        bool isBulbOn = await dataProvider.getBoolDataFromOneNet(httpToOneNet: httpToOneNet, field: "bulb");
        debugPrint("LightVal status: $lightVal \n");
        if(!mounted) return;
        Provider.of<DataProvider>(context, listen: false).motorOn = isMotorOn;
        Provider.of<DataProvider>(context, listen: false).bulbOn = isBulbOn;
        Provider.of<DataProvider>(context, listen: false).temperature = tempVal;
        Provider.of<DataProvider>(context, listen: false).humidity = humidityVal;
        Provider.of<DataProvider>(context, listen: false).mq4 = mq4Val;
        Provider.of<DataProvider>(context, listen: false).tempratureThreshold = tempThreshold;
        Provider.of<DataProvider>(context, listen: false).humidityThreshold = humidityThreshold;
        Provider.of<DataProvider>(context, listen: false).mq4Threshold = mq4Threshold;
        Provider.of<DataProvider>(context, listen: false).lightVal = lightVal;
        tempNotifier.value = "${tempVal.toString()}℃";
        humidityNotifier.value = "${humidityVal.toString()}%";
        mq4Notifier.value = "${mq4Val.toString()}ppm";
        motorOnNotifier.value = isMotorOn ? "开" : "关"; 
        bulbOnNotifier.value = isBulbOn ? "开" : "关";
        lightValNotifier.value = dataProvider.lightValToString(lightVal);
        setState(() {
      });
    });
  }
  @override
  void dispose() {
    timer?.cancel();
    super.dispose();
  }
  @override
  Widget build(BuildContext context) {
    FlutterError.onError = (FlutterErrorDetails details) {
      debugPrint(details.toString());
    };
    // This method is rerun every time setState is called, for instance as done
    // by the _incrementCounter method above.
    //
    // The Flutter framework has been optimized to make rerunning build methods
    // fast, so that you can just rebuild anything that needs updating rather
    // than having to individually change instances of widgets.
    return Scaffold(
      appBar: AppBar(
        backgroundColor: Theme.of(context).colorScheme.inversePrimary,
        title: Text(widget.title),
      ),
      body: Center(
        child: Column(
          mainAxisAlignment: MainAxisAlignment.start,
          children: [
            SizedBox(height: 50),
            Row(
              mainAxisAlignment: MainAxisAlignment.start, // 靠左对齐
              children: [
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.thermostat,
                    title: "温度",
                    valueNotifier: tempNotifier,
                    child: Column(
                      children: [
                        Text("温度阈值设置",style: TextStyle(fontSize: 32),),
                        Consumer<DataProvider>(
                          builder: (context, dataProvider, _) {
                            return Text("当前温度阈值: ${dataProvider.tempratureThreshold}℃",style: TextStyle(fontSize: 16),);
                          },
                        ),
                        TextField(
                          keyboardType: TextInputType.number,
                          onSubmitted: (value) async{
                            double? threshold = double.tryParse(value);
                            if (threshold != null) {
                              bool success =await httpToOneNet.sendControlCommand(field: "temp_threshold", value: threshold);
                              if(success)
                              {
                                debugPrint("温度阈值设置成功\n");
                                setState(() {
                                });
                              }
                              else
                              {
                                debugPrint("温度阈值设置失败\n");
                              }
                            } else {
                              debugPrint("无效的输入，请输入一个数字。\n");
                            }
                          },
                          decoration: InputDecoration(
                            hintText: "请输入温度阈值",
                            border: OutlineInputBorder(),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
                SizedBox(width: 20),
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.water_drop,
                    title: "湿度",
                    valueNotifier: humidityNotifier,
                    child: Column(
                      children: [
                        Text("湿度阈值设置",style: TextStyle(fontSize: 32)),
                        Consumer<DataProvider>(
                          builder: (context, dataProvider, _) {
                            return Text("当前湿度阈值: ${Provider.of<DataProvider>(context).humidityThreshold}%",style: TextStyle(fontSize: 16),);
                          },
                        ),
                        TextField(
                          keyboardType: TextInputType.number,
                          onSubmitted: (value) async{
                            double? threshold = double.tryParse(value);
                            if (threshold != null) {
                              // Provider.of<DataProvider>(context, listen: false).humidityThreshold = threshold;
                              bool success =await httpToOneNet.sendControlCommand(field: "humi_threshold", value: threshold);
                              if(success)
                              {
                                debugPrint("湿度阈值设置成功\n");
                              }
                              else
                              {
                                debugPrint("湿度阈值设置失败\n");
                              }
                            } else {
                              debugPrint("无效的输入，请输入一个数字。\n");
                            }
                          },
                          decoration: InputDecoration(
                            hintText: "请输入湿度阈值",
                            border: OutlineInputBorder(),
                          ),
                        ),
                      ],
                    ),
                  ),
                ),
              ],
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.cloud,
                    title: "空气质量",
                    valueNotifier: mq4Notifier,
                    child: Column(
                      children: [
                        Text("PPM阈值设置",style: TextStyle(fontSize: 32)),
                        Consumer<DataProvider>(
                          builder: (context, dataProvider, _) {
                            return Text("当前ppm阈值: ${Provider.of<DataProvider>(context).mq4Threshold}%",style: TextStyle(fontSize: 16),);
                          },
                        ),
                        TextField(
                          keyboardType: TextInputType.number,
                          onSubmitted: (value) async{
                            double? threshold = double.tryParse(value);
                            if (threshold != null) {
                              // Provider.of<DataProvider>(context, listen: false).humidityThreshold = threshold;
                              bool success =await httpToOneNet.sendControlCommand(field: "mq4_threshold", value: threshold);
                              if(success)
                              {
                                debugPrint("PPM阈值设置成功\n");
                              }
                              else
                              {
                                debugPrint("PPM阈值设置失败\n");
                              }
                            } else {
                              debugPrint("无效的输入，请输入一个数字。\n");
                            }
                          },
                          decoration: InputDecoration(
                            hintText: "请输入湿度阈值",
                            border: OutlineInputBorder(),
                          ),
                        ),
                      ],
                    ),
                  )
                ),
              ],
            ),
            Row(
              mainAxisAlignment: MainAxisAlignment.start,
              children: [
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.air,
                    title: "风扇状态",
                    valueNotifier: motorOnNotifier,
                    child: Column(
                      children: [
                        Text("风扇设置",style: TextStyle(fontSize: 32)),
                        Consumer<DataProvider>(
                          builder: (context, dataProvider, _) {
                            return Text("当前风扇状态: ${Provider.of<DataProvider>(context).motorOn ? "开" : "关"}",style: TextStyle(fontSize: 16),);
                          },
                        ),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text("风扇开关", style: TextStyle(fontSize: 18)),
                            Consumer<DataProvider>(
                              builder: (context, dataProvider, _) {
                                return Row(
                                  mainAxisSize: MainAxisSize.min,
                                  children: motorOptions.map((option) {
                                    final bool selected = isUserMotorOn == option['value'];
                                    return Padding(
                                      padding: const EdgeInsets.symmetric(horizontal: 4.0),
                                      child: ElevatedButton.icon(
                                        style: ElevatedButton.styleFrom(
                                          backgroundColor: selected ? Theme.of(context).colorScheme.primary : Colors.grey[300],
                                          foregroundColor: selected ? Colors.white : Colors.black87,
                                          minimumSize: const Size(60, 36),
                                          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                                        ),
                                        icon: Icon(option['icon'], size: 22),
                                        label: Text(option['label'], style: TextStyle(fontSize: 16)),
                                        onPressed: selected
                                            ? null
                                            : () async {
                                                bool value = option['value'];
                                                bool success = await httpToOneNet.sendControlCommand(field: "Motor", value: value);
                                                setState(() {});
                                                if (success) {
                                                  debugPrint("风扇开关设置成功\n");
                                                  debugPrint("当前风扇状态: $value \n");
                                                  isUserMotorOn = value;
                                                  setState(() {});
                                                } else {
                                                  debugPrint("风扇开关设置失败\n");
                                                }
                                              },
                                      ),
                                    );
                                  }).toList(),
                                );
                              },
                            ),
                          ]
                        ),
                      ],
                    ),
                  )
                ),
              ],
            ),
            Row(
              children: [
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.lightbulb,
                    title: "灯泡状态",
                    valueNotifier: bulbOnNotifier,
                    child: Column(
                      children: [
                        Text("灯泡设置",style: TextStyle(fontSize: 32)),
                        Consumer<DataProvider>(
                          builder: (context, dataProvider, _) {
                            return Text("当前灯泡状态: ${Provider.of<DataProvider>(context).bulbOn ? "开" : "关"}",style: TextStyle(fontSize: 16),);
                          },
                        ),
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Text("灯泡开关", style: TextStyle(fontSize: 18)),
                            Consumer<DataProvider>(
                              builder: (context, dataProvider, _) {
                                return Row(
                                  mainAxisSize: MainAxisSize.min,
                                  children: bulbOptions.map((option) {
                                    final bool selected = isUserBulbOn == option['value'];
                                    return Padding(
                                      padding: const EdgeInsets.symmetric(horizontal: 4.0),
                                      child: ElevatedButton.icon(
                                        style: ElevatedButton.styleFrom(
                                          backgroundColor: selected ? Theme.of(context).colorScheme.primary : Colors.grey[300],
                                          foregroundColor: selected ? Colors.white : Colors.black87,
                                          minimumSize: const Size(60, 36),
                                          padding: const EdgeInsets.symmetric(horizontal: 8, vertical: 4),
                                        ),
                                        icon: Icon(option['icon'], size: 22),
                                        label: Text(option['label'], style: TextStyle(fontSize: 16)),
                                        onPressed: selected
                                            ? null
                                            : () async {
                                                bool value = option['value'];
                                                bool success = await httpToOneNet.sendControlCommand(field: "bulb", value: value);
                                                setState(() {});
                                                if (success) {
                                                  debugPrint("灯泡开关设置成功\n");
                                                  debugPrint("当前灯泡状态: $value \n");
                                                  isUserBulbOn = value;
                                                  setState(() {});
                                                } else {
                                                  debugPrint("灯泡开关设置失败\n");
                                                }
                                              },
                                      ),
                                    );
                                  }).toList(),
                                );
                              },
                            ),
                          ],
                        ),
                      ],
                    ),
                  )
                ),
              ],
            ),
            Row(
              children: [
                Expanded(
                  child: CardStatusCard(
                    icon: Icons.light_mode,
                    title: "亮度状态",
                    valueNotifier: lightValNotifier,
                    child: Column(
                    ),
                  )
                ),
              ],
            )
          ],
        ),
      ),
    );
  }
}
