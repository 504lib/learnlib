import 'package:fluent_ui/fluent_ui.dart';
import 'dart:async';
import '../shellpage/shellpage.dart';
import '../MedicineDataProvider/MedicineDataProvider.dart';
import 'package:provider/provider.dart';

class Mainpage extends StatefulWidget {
  const Mainpage({super.key});
  @override
  State<Mainpage> createState() => _MainpageState();
}

class _MainpageState extends State<Mainpage> {
  Timer? _timer;
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback(
      (_) {
        final provider = context.read<Medicinedataprovider>();
        provider.fetchMedicineData();
        _timer = Timer.periodic(const Duration(seconds: 2), (timer) {
          provider.fetchMedicineData();
        });
      }
    );
  }
  @override
  void dispose() {
    _timer?.cancel();
    super.dispose();
  }
  @override
  Widget build(BuildContext context) {
    return ShellPage(
      child: Align(
        alignment: Alignment.topLeft,
        child: Padding(
          padding: const EdgeInsets.all(20.0),
          child: SingleChildScrollView(
            child: Consumer<Medicinedataprovider>(
              builder: (context, m, child) => Column(
                crossAxisAlignment: CrossAxisAlignment.start,
                children: [
                  const Text(
                    "药物管理系统",
                    style: TextStyle(
                      fontSize: 24,
                      fontWeight: FontWeight.bold,
                    ),
                  ),
                  const SizedBox(height: 20),
                  if(!m.connected)
                    Text(
                      "无法连接到药物管理系统，请检查网络连接。",
                      style: TextStyle(
                        color: Colors.red,
                        fontSize: 16,
                      ),
                    ),
                  Text("当前重量: ${m.currentWeight} g"),
                  Text("目标重量: ${m.targetWeight} g"),
                  Text("状态: ${m.systemStatus}"),
                  Text("药材: ${m.medicineType}"),
                ],
              ),
            )
          ),
        ),
      )
    );
  }
}