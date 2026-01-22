import 'package:fluent_ui/fluent_ui.dart';
import 'dart:async';
import '../shellpage/shellpage.dart';
import '../MedicineDataProvider/MedicineDataProvider.dart';
import 'package:provider/provider.dart';
import '../cardDisplay/stateCards.dart';
import '../QueuePanel/QueueePanel.dart';
import '../httpService/httpService.dart';

class Mainpage extends StatefulWidget {
  const Mainpage({super.key});
  @override
  State<Mainpage> createState() => _MainpageState();
}

class _MainpageState extends State<Mainpage> {
  Timer? _timer;
  Future<void> _showLeaveDialog(BuildContext context) async {
  final provider = context.read<Medicinedataprovider>();
  final options = provider.availableLeaveCodes;
  if (options.isEmpty) {
    await showDialog(
      context: context,
      builder: (_) => ContentDialog(
        title: const Text('无可退出的队列'),
        content: const Text('当前没有已加入的队列记录。'),
        actions: [
          Button(
            child: const Text('确定'),
            onPressed: () => Navigator.pop(context),
          ),
        ],
      ),
    );
    return;
  }

    String? selected = options.first;

    await showDialog(
      context: context,
      builder: (dialogCtx) {
        return StatefulBuilder(builder: (ctx, setState) {
          return ContentDialog(
            title: const Text('选择要退出的队列'),
            content: ComboBox<String>(
              value: selected,
              items: options
                  .map((c) => ComboBoxItem<String>(value: c, child: Text(c)))
                  .toList(),
              onChanged: (v) => setState(() => selected = v),
            ),
          actions: [
            Button(
              child: const Text('取消'),
              onPressed: () => Navigator.pop(dialogCtx),
            ),
            FilledButton(
              child: const Text('确定'),
              onPressed: () async {
                if (selected == null) return;
                late BuildContext loadingCtx;
                showDialog(
                  context: context,
                  barrierDismissible: false,
                  builder: (ctx) {
                    loadingCtx = ctx;
                    return const ContentDialog(
                      title: Text('请稍候'),
                      content: Row(
                        children: [
                          ProgressRing(),
                          SizedBox(width: 12),
                          Text('正在退出队列...'),
                        ],
                      ),
                    );
                  },
                );

                bool ok = false;
                try {
                  final medType = provider.getMedicineForCode(selected!);
                  ok = await _http
                      .sendLeaveQueue(selected!, medType)
                      .timeout(const Duration(seconds: 5));
                } catch (_) {}

                // 回读同步（会根据服务端状态清理本地）
                await provider.syncFromServer();

                if (Navigator.canPop(loadingCtx)) Navigator.pop(loadingCtx);
                if (Navigator.canPop(dialogCtx)) Navigator.pop(dialogCtx);
                if (!context.mounted) return;

                await showDialog(
                  context: context,
                  builder: (_) => ContentDialog(
                    title: Text(ok ? '已请求退出' : '请求失败'),
                    content: const Text('队列状态已与服务端同步。'),
                    actions: [
                      Button(
                        child: const Text('确定'),
                        onPressed: () => Navigator.pop(context),
                      ),
                    ],
                  ),
                );
              },
            ),
          ],
        );
        });
      },
    );
  } final _http = HttpService();


  Future<void> _showOrderDialog(BuildContext context) async {
    final provider = context.read<Medicinedataprovider>();
    final weightController = TextEditingController(text: "100");
    int medicine = 0; // 默认当前类型
    const medMap = {0: "麻黄", 1: "桂枝"};
    await showDialog(
      context: context,
      builder: (dialogCtx) {
        return ContentDialog(
          title: const Text('选择药品与重量'),
          content: Column(
            crossAxisAlignment: CrossAxisAlignment.start,
            mainAxisSize: MainAxisSize.min,
            children: [
              const Text("重量 (g)"),
              const SizedBox(height: 6),
              TextBox(
                controller: weightController,
                keyboardType: TextInputType.number,
              ),
              const SizedBox(height: 12),
              const Text("药品"),
              const SizedBox(height: 6),
              ComboBox<int>(
                value: medicine,
                items: medMap.entries
                    .map(
                      (e) => ComboBoxItem<int>(
                        value: e.key,
                        child: Text(e.value),
                      ),
                    )
                    .toList(),
                onChanged: (v) => medicine = v ?? 0,
              ),
            ],
          ),
          actions: [
            Button(
              child: const Text('取消'),
              onPressed: () => Navigator.pop(dialogCtx),
            ),
            FilledButton(
              child: const Text('确定'),
              onPressed: () async {
                final code = provider.generateClientCode();
                final w = int.tryParse(weightController.text.trim()) ?? 100;

                late BuildContext loadingCtx;
                showDialog(
                  context: context,
                  barrierDismissible: false,
                  builder: (ctx) {
                    loadingCtx = ctx;
                    return const ContentDialog(
                      title: Text('请稍候'),
                      content: Row(
                        children: [
                          ProgressRing(),
                          SizedBox(width: 12),
                          Text('正在提交请求...'),
                        ],
                      ),
                    );
                  },
                );

                bool ok = false;
                try {
                  ok = await _http
                      .sendJoinQueue(code, medicine, w)
                      .timeout(const Duration(seconds: 5));
                } catch (_) {}

                // 记录本地加入（用于可选退出映射）
                provider.recordLocalJoin(code, medicine);

                // 直接回读校验，以服务端为准
                await provider.syncFromServer();

                if (Navigator.canPop(loadingCtx)) Navigator.pop(loadingCtx);
                if (Navigator.canPop(dialogCtx)) Navigator.pop(dialogCtx);
                if (!context.mounted) return;

                await showDialog(
                  context: context,
                  builder: (_) => ContentDialog(
                    title: Text(ok ? '提交完成' : '提交失败'),
                    content: Text(ok ? '已尝试加入队列，并已与服务端同步。' : '请求失败或超时。'),
                    actions: [
                      Button(
                        child: const Text('确定'),
                        onPressed: () => Navigator.pop(context),
                      ),
                    ],
                  ),
                );
              },
            ),
          ],
        );
      },
    );
  }
  @override
  void initState() {
    super.initState();
    WidgetsBinding.instance.addPostFrameCallback(
      (_) {
        final provider = context.read<Medicinedataprovider>();
        provider.fetchMedicineData();
        _timer = Timer.periodic(const Duration(milliseconds: 200), (timer) {
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
            child: Column(
              children: [
                Center(
                  child: Text("药物管理",style: TextStyle(fontSize: 50,fontWeight: FontWeight.w700,letterSpacing: 1.2)),
                ),
                const SizedBox(height: 30),
                stateCards(),

                const SizedBox(height: 30),
                QueuePanel(),
                const SizedBox(height: 30),
                Wrap(
                  children: [
                    FilledButton(
                        child: const Text("下单/配药"),
                        onPressed: () => _showOrderDialog(context),
                    ),
                    const SizedBox(width: 12),
                    FilledButton(
                      child: const Text("退出队列"),
                      onPressed: () => _showLeaveDialog(context),
                    ),
                  ],
                )
              ],
            ),
          ),
        )
      )
    );
  }
}