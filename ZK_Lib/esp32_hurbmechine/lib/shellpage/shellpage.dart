import "package:fluent_ui/fluent_ui.dart";
import "package:go_router/go_router.dart";


class ShellPage extends StatelessWidget {
  const ShellPage({super.key,required this.child});

  int selectedIndexForUri(String uri) {
    switch (uri) {
      case '/':
        return 0;
      case '/settings':
        return 1;
      default:
        return 0;
    }
  }

  final Widget child;
  @override
  Widget build(BuildContext context) {
    final int selectedIndex = selectedIndexForUri(GoRouterState.of(context).uri.toString());
    return NavigationView(
      appBar: NavigationAppBar(
        title: const Text("Fluent UI with GoRouter"),
        automaticallyImplyLeading: false
      ),
      paneBodyBuilder: (item, body) => child,
      pane:NavigationPane(
        size: const NavigationPaneSize(openMinWidth: 100,openMaxWidth: 300),
        selected: selectedIndex,
        items: [
          PaneItem(
            icon: const Icon(FluentIcons.home),
            body:  SizedBox.shrink(),
            onTap: () => GoRouter.of(context).go("/"),
            title: Text("主页")
          ),
          PaneItem(
            icon: const Icon(FluentIcons.settings),
            body:  SizedBox.shrink(),
            onTap: () => GoRouter.of(context).go("/settings"),
            title: Text("设置")
          ),
        ]
      )
    );
  }
}