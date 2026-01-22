import 'package:fluent_ui/fluent_ui.dart';
import '../MedicineDataProvider/MedicineDataProvider.dart';
import 'package:provider/provider.dart';

class QueuePanel extends StatelessWidget {
  @override
  Widget build(BuildContext context) {
    final m = context.watch<Medicinedataprovider>();
    return Container(
      width: double.infinity,
      padding: const EdgeInsets.all(16),
      decoration: BoxDecoration(
        color: FluentTheme.of(context).micaBackgroundColor,
        borderRadius: BorderRadius.circular(8),
        border: Border.all(color: Colors.grey.withValues(alpha: 0.06)),
      ),
      child: Column(
        crossAxisAlignment: CrossAxisAlignment.start,
        children: [
          const Text(
            "队列",
            style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
          ),
          const SizedBox(height: 8),
          Row(
            children: [
              const Text("当前用户：", style: TextStyle(color: Color(0x99000000))),
              InfoLabel(label: m.currentUser ?? "-", child: const SizedBox.shrink()),
              const Spacer(),
              Text("队列数量：${m.medicineQueue.length}",
                  style: const TextStyle(color: Color(0x99000000))),
            ],
          ),
          const SizedBox(height: 12),
          ConstrainedBox(
            constraints: const BoxConstraints(maxHeight: 240),
            child: ListView.separated(
              shrinkWrap: true,
              itemCount: m.medicineQueue.length,
              physics: const NeverScrollableScrollPhysics(),
              separatorBuilder: (_, __) => const SizedBox(height: 8),
              itemBuilder: (context, index) {
                final item = m.medicineQueue[index];
                return Container(
                  padding: const EdgeInsets.symmetric(horizontal: 12, vertical: 10),
                  decoration: BoxDecoration(
                    color: Colors.white.withValues(alpha: 0.9),
                    borderRadius: BorderRadius.circular(6),
                    border: Border.all(color: Colors.grey.withValues(alpha: 0.08)),
                  ),
                  child: Text(item, style: const TextStyle(fontSize: 14)),
                );
              },
            ),
          ),
        ],
      ),
    );
  }
}