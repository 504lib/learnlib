import 'package:provider/provider.dart';
import '../MedicineDataProvider/MedicineDataProvider.dart';
import 'cardDisplay.dart';
import 'package:fluent_ui/fluent_ui.dart';

class stateCards extends StatefulWidget {
  const stateCards({super.key});

  @override
  State<stateCards> createState() => _stateCardsState();
}

class _stateCardsState extends State<stateCards> {
  @override
  Widget build(BuildContext context) {
    return Consumer<Medicinedataprovider>(
      builder: (context, m, child) {
         return m.connected
            ? Center(
                child: LayoutBuilder(
                  builder: (context, constraints) {
                    // 根据可用宽度决定列数
                    final maxW = constraints.maxWidth;
                    final columns = maxW < 520
                        ? 1
                        : (maxW < 860 ? 2 : 3);
                    const spacing = 12.0; // 更紧凑
                    final rawWidth = (maxW - spacing * (columns - 1)) / columns;
                    // 控制卡片边长在 140~220 之间
                    final itemWidth = rawWidth.clamp(120.0, 150.0);
                    final cards = [
                      StatCard(title: "当前重量", value: "${m.currentWeight}g"),
                      StatCard(title: "目标重量", value: "${m.targetWeight}g"),
                      StatCard(title: "状态", value: m.systemStatus),
                      StatCard(title: "药材", value: m.medicineType),
                    ];

                    return Wrap(
                      spacing: spacing,
                      runSpacing: spacing,
                      children: cards
                          .map((c) => SizedBox.square(
                                dimension: itemWidth,
                                child: c,
                              )
                          )
                          .toList(),
                    );
                  },
                ),
              )
         : Center(
          child: InfoBar(
                  title: const Text("无法连接到设备"),
                  content: const Text("请检查设备是否连接正常"),
                  severity: InfoBarSeverity.error,
                ),
        );
      },
    );
  }
}