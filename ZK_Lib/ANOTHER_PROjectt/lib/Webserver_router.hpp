#ifndef __WEBSERVER_ROUTER_HPP
#define __WEBSERVER_ROUTER_HPP

#include <Arduino.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

static const char WIFI_CONFIG_HTML[] PROGMEM = R"HTML(
<!doctype html>
<html lang="zh-CN">
<head>
	<meta charset="utf-8" />
	<meta name="viewport" content="width=device-width, initial-scale=1" />
	<title>AP 配网控制台</title>
	<style>
		:root {
			--bg: #0f1720;
			--bg2: #1a2330;
			--card: #1f2b3a;
			--line: #31465d;
			--text: #d7e3ef;
			--muted: #90a7bd;
			--accent: #22a16f;
			--accent2: #177451;
			--warn: #db9b24;
			--danger: #d24f4f;
			--ok: #1aa36e;
			--shadow: 0 16px 34px rgba(0, 0, 0, 0.35);
		}

		* { box-sizing: border-box; }
		body {
			margin: 0;
			min-height: 100vh;
			color: var(--text);
			background:
				radial-gradient(1200px 500px at 90% -20%, #2b3b50 0%, transparent 70%),
				linear-gradient(135deg, #0b121a, var(--bg));
			font-family: "Segoe UI", "Noto Sans SC", sans-serif;
			display: grid;
			place-items: center;
			padding: 20px;
		}

		.shell {
			width: min(760px, 100%);
			display: grid;
			gap: 16px;
		}

		.panel {
			background: linear-gradient(160deg, #1d2938 0%, #1a2432 100%);
			border: 1px solid var(--line);
			border-radius: 14px;
			box-shadow: var(--shadow);
			overflow: hidden;
		}

		.head {
			padding: 16px 18px;
			border-bottom: 1px solid var(--line);
			display: flex;
			align-items: center;
			justify-content: space-between;
			gap: 10px;
			background: linear-gradient(90deg, #1e2c3d, #203247);
		}

		h1 {
			margin: 0;
			font-size: 18px;
			letter-spacing: 0.8px;
		}

		.tag {
			font-size: 12px;
			padding: 4px 10px;
			border-radius: 999px;
			border: 1px solid var(--line);
			color: var(--muted);
			background: #162131;
		}

		.body { padding: 16px 18px 20px; }

		.grid {
			display: grid;
			grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
			gap: 12px;
			margin-bottom: 12px;
		}

		.item {
			border: 1px solid var(--line);
			border-radius: 10px;
			padding: 10px 12px;
			background: var(--bg2);
		}

		.item .k {
			font-size: 12px;
			color: var(--muted);
			margin-bottom: 4px;
		}

		.item .v {
			font-size: 14px;
			word-break: break-all;
		}

		.status {
			display: inline-flex;
			align-items: center;
			gap: 8px;
			padding: 6px 10px;
			border-radius: 999px;
			font-size: 12px;
			border: 1px solid var(--line);
			background: #162131;
			color: var(--muted);
			margin-bottom: 12px;
		}

		.dot {
			width: 8px;
			height: 8px;
			border-radius: 999px;
			background: var(--warn);
			box-shadow: 0 0 8px rgba(219, 155, 36, 0.7);
		}

		.status.ok .dot {
			background: var(--ok);
			box-shadow: 0 0 8px rgba(26, 163, 110, 0.8);
		}

		.status.err .dot {
			background: var(--danger);
			box-shadow: 0 0 8px rgba(210, 79, 79, 0.8);
		}

		.form {
			display: grid;
			gap: 10px;
			grid-template-columns: repeat(auto-fit, minmax(220px, 1fr));
		}

		label {
			font-size: 12px;
			color: var(--muted);
			display: block;
			margin-bottom: 6px;
		}

		input {
			width: 100%;
			border-radius: 9px;
			border: 1px solid var(--line);
			background: #111a26;
			color: var(--text);
			padding: 9px 10px;
			font-size: 14px;
			outline: none;
		}

		.actions {
			margin-top: 14px;
			display: flex;
			gap: 10px;
			flex-wrap: wrap;
		}

		button {
			border: none;
			border-radius: 9px;
			padding: 10px 14px;
			color: #fff;
			cursor: pointer;
			font-weight: 600;
			letter-spacing: 0.2px;
		}

		.primary {
			background: linear-gradient(180deg, var(--accent), var(--accent2));
			box-shadow: 0 8px 16px rgba(23, 116, 81, 0.35);
		}

		.ghost {
			background: #243548;
			border: 1px solid var(--line);
			color: var(--text);
		}

		.danger {
			background: #6d2525;
			border: 1px solid #884040;
		}

		.msg {
			margin-top: 12px;
			font-size: 13px;
			color: var(--muted);
			min-height: 20px;
		}
	</style>
</head>
<body>
	<main class="shell">
		<section class="panel">
			<div class="head">
				<h1>AP 局域网配网控制台</h1>
				<span class="tag" id="tick">同步中</span>
			</div>
			<div class="body">
				<div class="status" id="statusPill">
					<span class="dot"></span>
					<span id="statusText">正在读取连接状态...</span>
				</div>

				<div class="grid">
					<div class="item"><div class="k">AP SSID</div><div class="v" id="apSsid">--</div></div>
					<div class="item"><div class="k">AP IP</div><div class="v" id="apIp">--</div></div>
					<div class="item"><div class="k">目标 WiFi</div><div class="v" id="targetSsid">--</div></div>
					<div class="item"><div class="k">STA IP</div><div class="v" id="staIp">--</div></div>
				</div>

				<form id="wifiForm">
					<div class="form">
						<div>
							<label for="ssid">路由器 SSID</label>
							<input id="ssid" name="ssid" placeholder="请输入 WiFi 名称" required />
						</div>
						<div>
							<label for="pass">路由器密码</label>
							<input id="pass" name="pass" type="password" placeholder="请输入 WiFi 密码" required />
						</div>
					</div>
					<div class="actions">
						<button class="primary" type="submit">提交并重新配网</button>
						<button class="ghost" type="button" id="refreshBtn">刷新状态</button>
						<button class="danger" type="button" id="clearBtn">断连并清除凭证</button>
					</div>
				</form>

				<div class="msg" id="msg"></div>
			</div>
		</section>
	</main>

	<script>
		const $ = (id) => document.getElementById(id);

		function setMessage(text) {
			$("msg").textContent = text;
		}

		function renderStatus(data) {
			$("apSsid").textContent = data.ap_ssid || "--";
			$("apIp").textContent = data.ap_ip || "--";
			$("targetSsid").textContent = data.target_ssid || "--";
			$("staIp").textContent = data.sta_ip || "--";

			const pill = $("statusPill");
			pill.classList.remove("ok", "err");
			if (data.connected) {
				pill.classList.add("ok");
				$("statusText").textContent = "已连接到路由器，可正常联网";
			} else if (data.configured) {
				$("statusText").textContent = "已配置凭证，正在尝试连接";
			} else {
				pill.classList.add("err");
				$("statusText").textContent = "未配置路由器凭证";
			}
			$("tick").textContent = new Date().toLocaleTimeString();
		}

		async function loadStatus() {
			const r = await fetch("/wifi_status");
			const data = await r.json();
			renderStatus(data);
		}

		$("wifiForm").addEventListener("submit", async (e) => {
			e.preventDefault();
			const fd = new FormData(e.target);
			const res = await fetch("/connect_ssid", { method: "POST", body: fd });
			const txt = await res.text();
			setMessage(txt);
			await loadStatus();
		});

		$("refreshBtn").addEventListener("click", loadStatus);

		$("clearBtn").addEventListener("click", async () => {
			const res = await fetch("/clear_wifi", { method: "POST" });
			const txt = await res.text();
			setMessage(txt);
			await loadStatus();
		});

		loadStatus();
		setInterval(loadStatus, 2000);
	</script>
</body>
</html>
)HTML";

#endif // !1