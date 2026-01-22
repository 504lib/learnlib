import 'package:go_router/go_router.dart';
import 'package:provider/provider.dart';
import 'package:fluent_ui/fluent_ui.dart';
import 'mainpage/mainpage.dart';
import 'settingpage/settingpage.dart';
import 'MedicineDataProvider/MedicineDataProvider.dart';



void main(){
  runApp(
    ChangeNotifierProvider(
      create: (context) => Medicinedataprovider(),
      child: const MyApp(),
    )
  );
}

final GoRouter _router = GoRouter(
  routes: [
    GoRoute(
      path: "/",
      builder: (context, state) =>  const Mainpage(),
    ),
    GoRoute(
      path: "/settings",
      builder: (context, state) => const Settingpage(),
    )
  ]
);
class MyApp extends StatelessWidget {
  const MyApp({super.key});

  @override
  Widget build(BuildContext context) {
    return FluentApp.router(
      routerConfig: _router,
      title: '药物管理系统',
      theme: FluentThemeData(
        accentColor: Colors.blue,
        visualDensity: VisualDensity.standard,
      ),
      debugShowCheckedModeBanner: false,
    );
  }
}
