import 'package:flutter/material.dart';
import 'package:animations/animations.dart';
import 'CardStatus.dart';

class CardStatusCard extends StatelessWidget {
  final IconData icon;
  final String title;
  final Widget? child;
  final ValueNotifier<String> valueNotifier;
  const CardStatusCard({
    Key? key,
    required this.icon,
    required this.title,
    required this.valueNotifier,
    this.child,
  }) : super(key: key);

  @override
  Widget build(BuildContext context) {
    return Padding(
      padding: const EdgeInsets.symmetric(vertical: 8, horizontal: 16),
      child: OpenContainer(
        transitionType: ContainerTransitionType.fadeThrough,
        openColor: Theme.of(context).scaffoldBackgroundColor,
        closedColor: Theme.of(context).cardColor,
        closedElevation: 4,
        closedShape: RoundedRectangleBorder(
          borderRadius: BorderRadius.circular(24),
        ),
        openBuilder: (context, _) => CardStatusDetailPage(
          title: title,
          icon: icon,
          settingsWidget: child,
          valueNotifier: valueNotifier,
        ),
        closedBuilder: (context, openContainer) => SizedBox(
          width: 120,
          height: 120,
          child: GestureDetector(
            onTap: openContainer,
            child: Card(
              shape: RoundedRectangleBorder(
                borderRadius: BorderRadius.circular(24),
              ),
              elevation: 0,
              color: Colors.transparent,
              child: Center(
                child: Column(
                  mainAxisAlignment: MainAxisAlignment.center,
                  children: [
                    Icon(icon, size: 36, color: Theme.of(context).primaryColor),
                    const SizedBox(height: 8),
                    Text(title, style: const TextStyle(fontSize: 18, fontWeight: FontWeight.bold)),
                    const SizedBox(height: 4),
                    ValueListenableBuilder(
                      valueListenable: valueNotifier,
                      builder: (BuildContext context, dynamic value, Widget? child) {
                        return Text(value, style: const TextStyle(fontSize: 16, color: Colors.black54));
                      },
                    ),
                  ],
                ),
              ),
            ),
          ),
        ),
      ),
    );
  }
}