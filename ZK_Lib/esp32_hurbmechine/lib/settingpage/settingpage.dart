import '../shellpage/shellpage.dart';
import 'package:fluent_ui/fluent_ui.dart';

class Settingpage extends StatefulWidget {
  const Settingpage({super.key});

  @override
  State<Settingpage> createState() => _SettingpageState();
}

class _SettingpageState extends State<Settingpage> {
  @override
  Widget build(BuildContext context) {
    return ShellPage(
      child: const Center(
        child: Text("Setting Page"),
      ),
    );
  }
}