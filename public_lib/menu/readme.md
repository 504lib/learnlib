# menu

基于 u8g2 的菜单框架，提供“主界面/子菜单/功能/参数/开关”等类型，支持动画选中框与参数编辑模式，适用于嵌入式 OLED 菜单场景。

## 功能特性
- 多种菜单项类型：子菜单、功能项、整型参数、枚举参数、开关项、主界面。
- 参数编辑模式：进入后仅编辑数值/枚举/开关，退出时提交。
- 菜单树结构：父子/兄弟链式连接，便于构建多级菜单。
- 动画选中框：支持目标位置与尺寸的平滑过渡。

## 平台与依赖
- 显示驱动：u8g2。
- 目标平台宏：
	- MENU_USE_CMSIS_OS2（默认）
	- MENU_USE_BARE_METAL
	- MENU_USE_CUSTOM（需自定义原子操作宏）
- 依赖：标准 C 库、HAL（裸机模式）、CMSIS-RTOS2/FreeRTOS（RTOS 模式）。

## 主要文件
- menu.h：对外接口、类型定义与平台宏配置。
- menu.c：菜单逻辑、绘制与动画实现。

## 快速开始
1. 在工程中包含 menu.h，并选择目标平台宏（如 MENU_USE_CMSIS_OS2）。
2. 按需创建菜单节点（子菜单、功能项、参数项、开关项等）。
3. 使用 Link_Parent_Child/Link_next_sibling 连接菜单树。
4. 调用 menu_data_init 初始化根节点。
5. 在渲染循环中调用 show_menu，并周期性调用 update_animation。
6. 将按键事件映射到 navigate_up/navigate_down/navigate_enter/navigate_back。

## API 概览
- menu_data_init：初始化菜单数据。
- create_*_item：创建不同类型菜单项。
- Link_Parent_Child / Link_next_sibling：连接菜单树。
- show_menu：绘制当前菜单。
- navigate_*：处理上/下/确认/返回。
- update_animation：更新选中框动画。

## 注意事项
- MENU_NODE 决定静态节点池大小，节点数量超过会返回 NULL。
- 参数项进入编辑模式后，值会存放在临时变量中；退出编辑时提交。
- 主界面项（MENU_TYPE_MAIN）可设置专用回调进行自定义渲染。

## 许可证
- 以仓库根目录许可证为准。
