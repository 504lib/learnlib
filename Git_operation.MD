# Git 协作流程指南 (VSCode 版)
---

[TOC]

---

## 前言

---

本文档将引导你完成从代码提交到创建 Pull Request (PR)，再到合并后清理分支的完整协作流程。所有操作均可在 VSCode 中完成。

---

## 准备工作

+ 确保克隆了`learnlib`远程库
+ 确保配置了`ssh`的密钥，并提供给`github`
+ 确保 VScode 中登录 GitHub 账号

![插入图片：VSCode Git侧边栏截图，显示已登录状态](.\ZK_Lib\Photo\login_done.png)

第一步：创建功能分支
永远不要在 main 或 develop 分支上直接进行开发。

确保你位于主分支的最新状态：

bash
git checkout main
git pull origin main
创建并切换到一个新的功能分支：

bash
git checkout -b feat/你的功能简述
分支命名规范：类型/描述，例如：

feat/oled-menu (新功能)

fix/motor-pid (修复问题)

docs/readme-update (文档更新)

[插入图片：VSCode 左下角创建新分支的动画]

第二步：开发与提交代码
在你的分支上进行代码修改。

使用 VSCode 的源代码管理功能暂存（Stage）更改：

点击文件旁边的 + 号，或右键选择“暂存更改”

[插入图片：VSCode 暂存更改的界面]

编写规范的提交信息：

第一行：类型(模块): 简短描述 (50字符内)

第二行：空行

第三行：详细描述 (为什么修改，解决了什么问题)

示例：

text
feat(oled): add main menu interface

- Implement three-level menu structure
- Add scroll animation effect
- Fix display flickering issue
提交更改：按 Ctrl+Enter (Mac: Cmd+Enter)

将分支推送到远程仓库：

点击源代码管理视图中的"同步更改"按钮，或

在终端中执行：git push origin feat/你的分支名

[插入图片：VSCode 推送分支的界面]

第三步：创建 Pull Request (PR)
推送分支后，VSCode 通常会显示一个通知，点击 "Create Pull Request"

如果没有通知，可以转到 GitHub 网站操作

[插入图片：VSCode 创建PR的提示通知]

填写 PR 模板：

标题：清晰描述 PR 内容

描述：按照模板要求填写完整信息

Reviewers：选择至少一名审查者

Assignees：将自己设为负责人

点击 "Create" 创建 PR

[插入图片：GitHub PR 创建页面]

第四步：处理代码审查
审查者可能会提出修改意见，请及时查看 PR 页面或邮箱通知

根据反馈在本地进行修改：

bash
git checkout feat/你的分支名
# 进行代码修改
git add .
git commit -m "fix: 根据反馈修复xx问题"
git push origin feat/你的分支名
新的提交会自动添加到现有 PR 中

所有问题解决后，审查者会批准(Approved) PR

第五步：合并后清理分支
当 PR 被合并后，你需要清理本地和远程的分支。

1. 删除远程分支 (在 GitHub 上)
PR 合并后，GitHub 会显示一个 "Delete branch" 按钮，点击它删除远程分支。

[插入图片：GitHub 上删除分支的按钮]

2. 同步本地仓库并删除本地分支
切换回主分支并获取最新更新：

bash
git checkout main
git pull --prune origin main  # --prune 参数会同步删除远程已不存在的分支引用
删除已合并的本地分支：

bash
git branch -d feat/你的分支名
如果提示分支未完全合并，但确认可以删除，使用强制删除：

bash
git branch -D feat/你的分支名  # 注意是大写 D
(可选) 列出并清理所有已合并到当前分支的分支：

bash
# 查看已合并的分支
git branch --merged

# 删除所有已合并到main的分支（除了main本身）
git branch --merged | grep -v "\*" | grep -v "main" | xargs -n 1 git branch -d
[插入图片：VSCode 终端中执行清理命令的示例]

常见问题排查
问题：执行 git pull --prune 后，VSCode 仍显示旧分支
解决方案：VSCode 的分支列表可能需要手动刷新：

按 F1 打开命令面板

输入并执行 Git: Refresh

问题：PR 创建按钮没有出现
解决方案：

确认你已成功推送分支：git push origin your-branch-name

确认你已在 VSCode 中登录 GitHub

或者直接访问 GitHub 网站创建 PR

问题：分支删除失败
解决方案：

确保你不在要删除的分支上：先 git checkout main

确保分支已完全合并，或使用 git branch -D 强制删除

