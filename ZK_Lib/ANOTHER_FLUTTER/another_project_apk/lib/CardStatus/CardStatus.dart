import 'package:flutter/material.dart';

class CardStatusDetailPage extends StatefulWidget {
  final String title;
  final IconData icon;
  final Widget? settingsWidget;
  final ValueNotifier<String> valueNotifier;

  const CardStatusDetailPage({
    Key? key,
    required this.title,
    required this.icon,
    required this.valueNotifier,
    this.settingsWidget,
  }) : super(key: key);

  @override
  State<CardStatusDetailPage> createState() => _CardStatusDetailPageState();
}

class _CardStatusDetailPageState extends State<CardStatusDetailPage> {

  @override
  void initState() {
    super.initState();
    // 这里可以添加监听逻辑，比如监听 Provider 或其他数据源
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(title: Text(widget.title)),
      body: Column(
        children: [
          const SizedBox(height: 32),
          Row(
            children: [
              Hero(
                tag: 'card-hero-${widget.title}',
                child: Icon(widget.icon, size: 50, color: Theme.of(context).primaryColor),
              ),
              ValueListenableBuilder<String>(
                valueListenable:  widget.valueNotifier,
                builder: (BuildContext context, dynamic value, Widget? child) {
                  return  Text('当前值: $value', style: const TextStyle(fontSize: 20));
                },
              ),
            ],
          ),
          const SizedBox(height: 24),
          widget.settingsWidget != null ? Padding(
            padding: const EdgeInsets.all(16.0),
            child: widget.settingsWidget!,
          ) : const SizedBox.shrink(),
        ],
      ),
    );
  }
}