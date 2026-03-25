import 'dart:async';
import 'dart:ffi';
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
        title: "Flutter Demo Home Page",
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
      title: 'Flutter Demo',
      theme: ThemeData(
        colorScheme: ColorScheme.fromSeed(seedColor: Colors.white60),
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
  bool _LED_Toggle = false;
  Timer? timer;
  final HttpToOneNet httpToOneNet = HttpToOneNet(
    productId: '82aJXC88GG',
    deviceName: "DHT11",
    token: "version=2022-05-01&res=products%2F82aJXC88GG&et=2058153538&method=md5&sign=k8cGxQXiEUnM4qfuownJcw%3D%3D"
  );
  late final ValueNotifier<String> tempNotifier; 
  late final ValueNotifier<String> humidityNotifier;
  @override
  void initState() {
    super.initState();
    final dataProvider = Provider.of<DataProvider>(context, listen: false);
    tempNotifier = ValueNotifier(dataProvider.temperature.toString());
    humidityNotifier = ValueNotifier(dataProvider.humidity.toString());
    timer = Timer.periodic(const Duration(milliseconds: 500), (t) async{
        double tempVal = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "temp");
        double humidityVal = await dataProvider.getDataFromOneNet(httpToOneNet: httpToOneNet, field: "humi");
        double mq4Val = await dataProvider.getDataFromOneNet(httpToOneNet:httpToOneNet, field: "mq4");
        if(!mounted) return;
        Provider.of<DataProvider>(context, listen: false).temperature = tempVal;
        Provider.of<DataProvider>(context, listen: false).humidity = humidityVal;
        Provider.of<DataProvider>(context, listen: false).mq4 = mq4Val;
        tempNotifier.value = "${tempVal.toString()}℃";
        humidityNotifier.value = "${humidityVal.toString()}%";
        setState(() {
      });
    });
  }
  @override
  void dispose() {
    timer?.cancel();
    super.dispose();
  }
  void _onPressed(){
    httpToOneNet.updataPropertyData().then((success){
      if(success)
      {
        httpToOneNet.propertyData?.forEach((key, value) {
          debugPrint('Property: $key, Value: $value');
        });
      }
      else
      {
        debugPrint("数据更新失败\n");
      }
    });
      // testconnect();
  }
  void _tesHttpControl()
  {
     httpToOneNet.sendControlCommand(field: "bulb", value: _LED_Toggle).then((success){
      if(success)
      {
        debugPrint("控制命令发送成功\n");
        _LED_Toggle = !_LED_Toggle;
      }
      else
      {
        debugPrint("控制命令发送失败\n");
      }
     });
  }
  @override
  Widget build(BuildContext context) {
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
                        Text("温度阈值设置"),
                        Text("当前温度阈值: ${Provider.of<DataProvider>(context).tempratureThreshold}℃"),
                        TextField(
                          keyboardType: TextInputType.number,
                          onSubmitted: (value) async{
                            double? threshold = double.tryParse(value);
                            if (threshold != null) {
                              Provider.of<DataProvider>(context, listen: false).tempratureThreshold = threshold;
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
                    valueNotifier: ValueNotifier("0.0%"),
                    child: Column(
                      children: [
                        Text("湿度阈值设置"),
                        TextField(
                          keyboardType: TextInputType.number,
                          onSubmitted: (value) async{
                            double? threshold = double.tryParse(value);
                            if (threshold != null) {
                              Provider.of<DataProvider>(context, listen: false).humidityThreshold = threshold;
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
            )
          ],
        ),
      ),
    );
  }
}
