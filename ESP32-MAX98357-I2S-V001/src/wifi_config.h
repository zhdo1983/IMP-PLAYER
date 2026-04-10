#ifndef WIFI_CONFIG_H // 防止头文件重复包含
#define WIFI_CONFIG_H // 定义 WIFI_CONFIG_H 宏

#include "globals.h" // 引入全局变量和常量头文件
#include "stations.h" // 引入电台相关头文件

//========================== WiFi 多账号配置 ========================== // WiFi 多账号配置部分
// 最多保存 5 个 WiFi 账号，NVS key 格式：wifi_ssid_0 … wifi_ssid_4 // 最多保存 5 个 WiFi 账号说明
#define MAX_WIFI_COUNT 5 // 定义最大 WiFi 账号数量为 5

// 前向声明 // 前向声明函数
void connectToWiFi(); // 连接 WiFi 函数前向声明
void setupNTP();      // NTP时间同步前向声明
void handleStationsList(); // 电台列表处理函数前向声明
void handleStationUpdate(); // 电台更新处理函数前向声明
void handleStationDelete(); // 电台删除处理函数前向声明
void handleStationAdd(); // 电台添加处理函数前向声明
void handleWifiList(); // WiFi 列表处理函数前向声明
void handleWifiAdd(); // WiFi 添加处理函数前向声明
void handleWifiDelete(); // WiFi 删除处理函数前向声明
void handleWifiScan(); // WiFi 扫描处理函数前向声明
void handleNaviConfig(); // Navidrome配置(GET查/POST存，合并handler)
void handleNaviPing(); // Navidrome连接测试函数前向声明
void handleOtaConfig(); // OTA服务器URL配置(GET查/POST存)

// ---- 从 NVS 读取所有已保存 WiFi（返回实际条目数） ---- // 从 NVS 读取所有已保存 WiFi 函数的说明
int loadSavedWiFiList(String ssids[], String passes[]) { // 从 NVS 读取所有已保存 WiFi 的函数
  int count = 0; // 初始化计数变量
  for (int i = 0; i < MAX_WIFI_COUNT; i++) { // 遍历所有 WiFi 槽位
    String s = preferences.getString(("wifi_ssid_" + String(i)).c_str(), ""); // 读取 SSID
    String p = preferences.getString(("wifi_pass_" + String(i)).c_str(), ""); // 读取密码
    ssids[i] = s; // 存储 SSID 到数组
    passes[i] = p; // 存储密码到数组
    if (s.length() > 0) count++; // 如果 SSID 不为空则计数加 1
  }
  return count; // 返回实际保存的 WiFi 数量
}

// ---- 保存单条 WiFi 到 NVS ---- // 保存单条 WiFi 到 NVS 函数的说明
void saveWiFiEntry(int idx, const String& ssid, const String& pass) { // 保存单条 WiFi 到 NVS 的函数
  preferences.putString(("wifi_ssid_" + String(idx)).c_str(), ssid); // 保存 SSID 到 NVS
  preferences.putString(("wifi_pass_" + String(idx)).c_str(), pass); // 保存密码到 NVS
}

// ---- 删除单条 WiFi（将后续条目前移，保持紧凑） ---- // 删除单条 WiFi 函数的说明
void deleteWiFiEntry(int idx) { // 删除单条 WiFi 的函数
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT]; // 创建临时数组
  loadSavedWiFiList(ssids, passes); // 加载现有 WiFi 列表

  if (idx < 0 || idx >= MAX_WIFI_COUNT) return; // 如果索引无效则返回

  // 前移 // 注释：前移操作
  for (int i = idx; i < MAX_WIFI_COUNT - 1; i++) { // 将后续条目向前移动
    ssids[i]  = ssids[i + 1]; // SSID 向前移动
    passes[i] = passes[i + 1]; // 密码向前移动
  }
  ssids[MAX_WIFI_COUNT - 1]  = ""; // 最后一个槽位置空
  passes[MAX_WIFI_COUNT - 1] = ""; // 最后一个密码置空

  for (int i = 0; i < MAX_WIFI_COUNT; i++) { // 遍历所有槽位
    preferences.putString(("wifi_ssid_" + String(i)).c_str(), ssids[i]); // 保存 SSID 到 NVS
    preferences.putString(("wifi_pass_" + String(i)).c_str(), passes[i]); // 保存密码到 NVS
  }
}

// ---- 添加一条 WiFi（写入第一个空槽，重复SSID则覆盖） ---- // 添加一条 WiFi 函数的说明
bool addWiFiEntry(const String& ssid, const String& pass) { // 添加一条 WiFi 的函数
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT]; // 创建临时数组
  loadSavedWiFiList(ssids, passes); // 加载现有 WiFi 列表

  // 先检查是否已存在相同 SSID，若存在则更新密码 // 注释：检查重复
  for (int i = 0; i < MAX_WIFI_COUNT; i++) { // 遍历所有槽位
    if (ssids[i] == ssid) { // 如果找到相同 SSID
      saveWiFiEntry(i, ssid, pass); // 更新密码
      return true; // 返回成功
    }
  }
  // 写入第一个空槽 // 注释：写入空槽
  for (int i = 0; i < MAX_WIFI_COUNT; i++) { // 遍历所有槽位
    if (ssids[i].length() == 0) { // 如果找到空槽
      saveWiFiEntry(i, ssid, pass); // 保存 WiFi
      return true; // 返回成功
    }
  }
  return false; // 已满 // 返回 false 表示已满
}

// ---- 更新配置模式后的显示 ---- // 更新配置模式后显示函数的说明
void updateDisplayAfterConfig() { // 更新配置模式后显示的函数
  strcpy(stationString, "Reconnecting..."); // 复制提示信息到电台字符串
  strcpy(infoString, "Please wait"); // 复制提示信息到信息字符串
  if (strlen(lastHost) > 0) { // 如果有上次播放的主机
    audio.stopSong(); // 停止当前歌曲
    audio.connecttohost(lastHost); // 重新连接到主机
    strcpy(stationString, "Reconnecting to"); // 复制提示信息
    strcpy(infoString, lastHost); // 复制主机名到信息字符串
  }
}

// ---- 重置配置模式 ---- // 重置配置模式函数的说明
void resetConfigMode() { // 重置配置模式的函数
  if (!configMode) return; // 如果不在配置模式则返回
  configMode = false; // 关闭配置模式
  WiFi.softAPdisconnect(true); // 断开软 AP 连接
  dnsServer.stop(); // 停止 DNS 服务器
  server.stop(); // 停止 Web 服务器

  // 只要有任意一条已保存 WiFi 就尝试连接 // 注释：检查已保存 WiFi
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT]; // 创建临时数组
  int cnt = loadSavedWiFiList(ssids, passes); // 加载已保存的 WiFi 列表
  if (cnt > 0) connectToWiFi(); // 如果有保存的 WiFi 则连接

  updateDisplayAfterConfig(); // 更新显示
}

// ---- Web 页面：根路径 ---- // Web 页面根路径函数的说明
void handleRoot() { // 处理根路径请求的函数
  String html = // HTML 页面字符串
    "<!DOCTYPE html><html><head><title>设备配置</title><meta charset=\"UTF-8\">"
    "<meta name=\"viewport\" content=\"width=device-width,initial-scale=1\">"
    "<style>"
    "body{font-family:Arial,sans-serif;background:#f0f0f0;margin:0;padding:0;padding-bottom:80px;}"
    ".container{max-width:480px;margin:30px auto;padding:20px;background:white;"
    "border-radius:10px;box-shadow:0 0 10px rgba(0,0,0,.1);}"
    "h1{text-align:center;color:#333;font-size:1.3em;}"
    "h2{color:#444;font-size:1.1em;border-bottom:2px solid #007bff;padding-bottom:4px;}"
    ".form-group{margin-bottom:12px;}"
    "label{display:block;margin-bottom:4px;font-size:.9em;color:#555;}"
    "input[type=text],input[type=password]{"
    "width:100%;padding:8px;border:1px solid #ddd;border-radius:4px;box-sizing:border-box;}"
    "button{padding:6px 12px;background:#007bff;color:white;border:none;"
    "border-radius:4px;cursor:pointer;font-size:.9em;}"
    "button:hover{background:#0056b3;}"
    "button.del{background:#dc3545;} button.del:hover{background:#b02a37;}"
    ".status{margin-top:10px;padding:8px;border-radius:4px;font-size:.9em;}"
    ".success{background:#d4edda;color:#155724;}"
    ".error{background:#f8d7da;color:#721c24;}"
    "table{width:100%;border-collapse:collapse;margin-top:10px;font-size:.85em;}"
    "th,td{border:1px solid #ddd;padding:5px;text-align:left;}"
    "th{background:#f5f5f5;}"
    ".add-form{margin-top:12px;padding:10px;background:#f9f9f9;border-radius:6px;}"
    ".add-form input{margin-bottom:6px;}"
    ".section{margin-top:24px;}"
    ".restart-box{position:fixed;bottom:0;left:0;right:0;padding:12px 16px;"
    "background:#e7f3ff;border-top:1px solid #b8d4f0;text-align:center;"
    "z-index:999;box-shadow:0 -2px 8px rgba(0,0,0,.1);}"
    ".tag{display:inline-block;padding:2px 6px;border-radius:3px;"
    "font-size:.75em;background:#28a745;color:white;margin-left:4px;}"
    "</style></head><body>"
    "<div class=\"container\">"
    "<h1>&#128246; ESP32 网络收音机 配置</h1>"

    // ========== WiFi 账号管理 ========== // WiFi 账号管理部分
    "<div class=\"section\">"
    "<h2>WiFi 账号管理</h2>"
    "<p style=\"font-size:.85em;color:#666;\">最多保存 5 个 WiFi，设备启动时自动选择信号最强的连接。</p>"
    "<table id=\"wifiTable\">"
    "<tr><th>#</th><th>SSID</th><th>操作</th></tr>"
    "</table>"
    "<div class=\"add-form\">"
    "<div class=\"form-group\" style=\"position:relative;\">"
    "<label>新增 WiFi 名称 (SSID):</label>"
    "<div style=\"display:flex;gap:6px;align-items:center;\">"
    "<input type=\"text\" id=\"newWifiSSID\" placeholder=\"手动输入或点击右侧扫描\" style=\"flex:1;\""
    " autocomplete=\"off\" oninput=\"filterDropdown()\" onfocus=\"showDropdown()\">"
    "<button onclick=\"scanWifi()\" id=\"scanBtn\" style=\"white-space:nowrap;background:#17a2b8;padding:8px 10px;\">&#128246; 扫描</button>"
    "</div>"
    "<div id=\"wifiDropdown\" style=\"display:none;position:absolute;left:0;right:0;top:100%;"
    "background:white;border:1px solid #ccc;border-radius:4px;box-shadow:0 4px 12px rgba(0,0,0,.15);"
    "max-height:220px;overflow-y:auto;z-index:9999;margin-top:2px;\"></div>"
    "<div id=\"scanStatus\" style=\"font-size:.8em;color:#888;margin-top:4px;\"></div>"
    "</div>"
    "<div class=\"form-group\"><label>密码:</label>"
    "<input type=\"password\" id=\"newWifiPass\" placeholder=\"WiFi 密码（无密码留空）\"></div>"
    "<button onclick=\"addWifi()\">&#43; 添加 WiFi</button>"
    "<span id=\"wifiAddStatus\" style=\"margin-left:8px;font-size:.85em;\"></span>"
    "</div></div>"

        // ========== Navidrome 配置 ========== // Navidrome音乐服务器配置区域
    "<div class=\"section\">"
    "<h2>&#127925; Navidrome 音乐服务器</h2>"
    "<p style=\"font-size:.85em;color:#666;\">配置 Navidrome/Subsonic 服务器，设备将随机串流播放您的音乐库。</p>"
    "<div class=\"add-form\">"
    "<div class=\"form-group\"><label>服务器地址 (IP:端口 如 192.168.1.10:4533):</label>"
    "<input type=\"text\" id=\"naviServer\" placeholder=\"192.168.1.10:4533\"></div>"
    "<div class=\"form-group\"><label>用户名:</label>"
    "<input type=\"text\" id=\"naviUser\" placeholder=\"admin\"></div>"
    "<div class=\"form-group\"><label>密码 (留空则保持原密码):</label>"
    "<input type=\"password\" id=\"naviPass\" placeholder=\"&#8226;&#8226;&#8226;&#8226;&#8226;\" autocomplete=\"new-password\"></div>"
    "<div style=\"display:flex;gap:8px;align-items:center;flex-wrap:wrap;\">"
    "<button onclick=\"saveNaviConfig()\">&#10003; 保存配置</button>"
    "<button onclick=\"testNaviPing()\" style=\"background:#17a2b8;\">&#128268; 测试连接</button>"
    "<span id=\"naviStatus\" style=\"font-size:.85em;\"></span>"
    "</div>"
    "<div id=\"naviCfgInfo\" style=\"margin-top:8px;font-size:.8em;color:#666;\"></div>"
    "</div></div>"

    // ========== OTA 升级服务器配置 ========== //
    "<div class=\"section\">"
    "<h2>&#128640; OTA 固件升级服务器</h2>"
    "<p style=\"font-size:.85em;color:#666;\">自定义固件版本检查和下载地址。两项均留空可恢复内置默认地址。</p>"
    "<div class=\"add-form\">"
    "<div class=\"form-group\"><label>版本检查 URL（留空使用默认）:</label>"
    "<input type=\"text\" id=\"otaVerUrl\" placeholder=\"https://your-server/version.txt\"></div>"
    "<div class=\"form-group\"><label>固件下载 URL（留空使用默认）:</label>"
    "<input type=\"text\" id=\"otaFwUrl\" placeholder=\"https://your-server/firmware.bin\"></div>"
    "<div id=\"otaCustomTag\" style=\"font-size:.8em;margin-bottom:8px;\"></div>"
    "<div style=\"display:flex;gap:8px;align-items:center;flex-wrap:wrap;\">"
    "<button onclick=\"saveOtaConfig()\">&#10003; 保存地址</button>"
    "<button onclick=\"resetOtaConfig()\" style=\"background:#6c757d;\">&#8635; 恢复默认</button>"
    "<span id=\"otaStatus\" style=\"font-size:.85em;\"></span>"
    "</div></div></div>"

    // ========== 电台管理 ========== // 电台管理部分
    "<div class=\"section\">"
    "<h2>电台管理</h2>"
    "<div class=\"add-form\">"
    "<div class=\"form-group\"><label>新电台名称:</label>"
    "<input type=\"text\" id=\"addName\" placeholder=\"Station Name\"></div>"
    "<div class=\"form-group\"><label>新电台 URL:</label>"
    "<input type=\"text\" id=\"addUrl\" placeholder=\"http://...\"></div>"
    "<button onclick=\"addStation()\">&#43; 添加电台</button>"
    "<span id=\"addStatus\" style=\"margin-left:8px;font-size:.85em;\"></span>"
    "</div>"
    "<table id=\"stationsTable\">"
    "<tr><th>名称</th><th>URL</th><th>操作</th></tr>"
    "</table></div>"


    // ========== 关闭容器 / 保存并重启悬浮栏 ========== //
    "</div>"  // 关闭 .container
    "<div class=\"restart-box\">"
    "<button onclick=\"saveAndRestart()\" "
    "style=\"padding:10px 32px;background:#28a745;font-size:1em;\">&#10003; 保存并重启</button>"
    "<div id=\"restartStatus\" style=\"margin-top:4px;font-size:.85em;\"></div>"
    "</div>"
    // ========== JavaScript ========== // JavaScript 部分
    "<script>"
    // ---- Navidrome 配置JS函数 ---- // Navidrome配置相关JavaScript
    "async function loadNaviConfig(){"
    "try{const r=await fetch('/navi/config');const d=await r.json();"
    "var srvEl=document.getElementById('naviServer');"
    "var usrEl=document.getElementById('naviUser');"
    "if(d.hasServer){srvEl.value='';srvEl.placeholder='\u9ed8\u8ba4\u5730\u5740\u5df2\u52a0\u8f7d\uff0c\u53ef\u76f4\u63a5\u4fee\u6539';}"
    "if(d.hasUser){usrEl.value='';usrEl.placeholder='\u5df2\u8bbe\u7f6e\uff0c\u7559\u7a7a\u4e0d\u4fee\u6539';}"
    "var info=document.getElementById('naviCfgInfo');"
    "info.textContent=d.configured?'\u2713 \u5df2\u914d\u7f6e':'\u5c1a\u672a\u914d\u7f6e';"
    "}catch(e){}}"

    "async function saveNaviConfig(){"
    "const sv=document.getElementById('naviServer').value.trim();"
    "const us=document.getElementById('naviUser').value.trim();"
    "const pw=document.getElementById('naviPass').value;"
    "const st=document.getElementById('naviStatus');"
    "if(!us){st.textContent='\u7528\u6237\u540d\u4e0d\u80fd\u4e3a\u7a7a';st.style.color='red';return;}"
    "st.textContent='\u4fdd\u5b58\u4e2d...';st.style.color='#666';"
    "const params=new URLSearchParams();params.append('server',sv);params.append('user',us);params.append('pass',pw);"
    "const r=await fetch('/navi/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:params.toString()});"
    "const d=await r.json();"
    "if(d.ok){st.textContent='\u2713 \u5df2\u4fdd\u5b58';st.style.color='green';loadNaviConfig();}"
    "else{st.textContent='\u4fdd\u5b58\u5931\u8d25:'+JSON.stringify(d);st.style.color='red';}}"

    "async function testNaviPing(){"
    "const st=document.getElementById('naviStatus');"
    "st.textContent='\u6d4b\u8bd5\u4e2d...';st.style.color='#666';"
    "try{const r=await fetch('/navi/ping');const d=await r.json();"
    "if(d.ok){st.textContent='\u2713 '+d.msg;st.style.color='green';}"
    "else{st.textContent='\u2717 '+d.msg;st.style.color='red';}"
    "}catch(e){st.textContent='\u2717 \u8bf7\u6c42\u5931\u8d25';st.style.color='red';}}"

    // ---- WiFi 扫描 ---- // WiFi 扫描函数
    "var _scanResults=[];"  // 缓存扫描结果

    // 渲染下拉列表（根据输入过滤）
    "function renderDropdown(filter){"
    "var dd=document.getElementById('wifiDropdown');"
    "if(_scanResults.length===0){dd.style.display='none';return;}"
    "var kw=(filter||'').toLowerCase();"
    "var items=_scanResults.filter(function(x){return x.ssid.toLowerCase().indexOf(kw)>=0;});"
    "if(items.length===0){dd.style.display='none';return;}"
    "var html='';"
    "for(var i=0;i<items.length;i++){"
    "var it=items[i];"
    "var bar=it.rssi>=-60?'▂▄▆█':it.rssi>=-70?'▂▄▆_':it.rssi>=-80?'▂▄__':'▂___';"
    "var lock=it.enc?'🔒 ':'';"
    // 用 data-ssid 属性传递原始 SSID，避免 onclick 字符串转义问题
    "html+='<div data-ssid=\"'+it.ssid.replace(/\"/g,'&quot;')+'\" '"
    "+'onclick=\"pickSSID(this)\" '"
    "+'style=\"padding:10px 12px;cursor:pointer;border-bottom:1px solid #f0f0f0;\"'"
    "+'onmousedown=\"event.preventDefault()\">'  "  // 阻止 input blur 导致下拉先消失
    "+'<span style=\"font-weight:bold;\">'+escHtml(it.ssid)+'</span> '"
    "+'<span style=\"color:#888;font-size:.8em;float:right;\">'+lock+bar+' '+it.rssi+'dBm</span>'"
    "+'</div>';"
    "}"
    "dd.innerHTML=html;"
    "dd.style.display='block';}"

    "function pickSSID(el){"
    "document.getElementById('newWifiSSID').value=el.getAttribute('data-ssid');"
    "document.getElementById('wifiDropdown').style.display='none';"
    "document.getElementById('newWifiPass').focus();}"  // 自动跳到密码框

    "function showDropdown(){"
    "if(_scanResults.length>0) renderDropdown(document.getElementById('newWifiSSID').value);}"

    "function filterDropdown(){"
    "renderDropdown(document.getElementById('newWifiSSID').value);}"

    // 点击页面其他地方关闭下拉
    "document.addEventListener('click',function(e){"
    "var dd=document.getElementById('wifiDropdown');"
    "var inp=document.getElementById('newWifiSSID');"
    "if(dd&&!dd.contains(e.target)&&e.target!==inp) dd.style.display='none';});"

    "async function scanWifi(){"
    "var btn=document.getElementById('scanBtn');"
    "var st=document.getElementById('scanStatus');"
    "var dd=document.getElementById('wifiDropdown');"
    "btn.disabled=true;btn.textContent='扫描中...';"
    "st.textContent='正在扫描周边 WiFi，约需 3-5 秒...';st.style.color='#888';"
    "dd.style.display='none';"
    "try{"
    "var r=await fetch('/wifi/scan');"
    "if(!r.ok) throw new Error('HTTP '+r.status);"
    "var list=await r.json();"
    "_scanResults=list;"
    "if(list.length===0){"
    "st.textContent='未扫描到任何 WiFi';st.style.color='#c00';"
    "}else{"
    "st.textContent='✓ 找到 '+list.length+' 个 WiFi，请从下拉中选择';st.style.color='green';"
    "renderDropdown('');"  // 展开下拉
    "document.getElementById('newWifiSSID').focus();"
    "}"
    "}catch(e){"
    "st.textContent='扫描失败: '+e.message;st.style.color='red';"
    "}finally{"
    "btn.disabled=false;btn.textContent='📶 重新扫描';}}"

    // ---- WiFi 列表 ---- // WiFi 列表函数
    "async function loadWifi(){"
    "const r=await fetch('/wifi/list');const list=await r.json();"
    "var h='<tr><th>#</th><th>SSID</th><th>操作</th></tr>';"
    "for(var i=0;i<list.length;i++){"
    "var w=list[i];"
    "h+='<tr><td>'+(i+1)+'</td><td>'+escHtml(w.ssid)+"
    "'<span class=\"tag\">已保存</span></td>'+"
    "'<td><button class=\"del\" onclick=\"deleteWifi('+w.idx+')\">删除</button></td></tr>';}"
    "if(list.length===0)h+='<tr><td colspan=\"3\" style=\"color:#999;text-align:center;\">暂无保存的 WiFi</td></tr>';"
    "document.getElementById('wifiTable').innerHTML=h;}"

    "async function addWifi(){"
    "const ssid=document.getElementById('newWifiSSID').value.trim();"
    "const pass=document.getElementById('newWifiPass').value;"
    "const st=document.getElementById('wifiAddStatus');"
    "if(!ssid){st.innerText='SSID 不能为空';st.style.color='red';return;}"
    "const r=await fetch('/wifi/add',{method:'POST',"
    "body:new URLSearchParams({ssid,pass})});"
    "if(r.ok){st.innerText='✓ 已添加';st.style.color='green';"
    "document.getElementById('newWifiSSID').value='';"
    "document.getElementById('newWifiPass').value='';loadWifi();}"
    "else{st.innerText='✗ 失败（已达上限5条）';st.style.color='red';}}"

    "async function deleteWifi(idx){"
    "if(!confirm('确认删除此 WiFi 账号？'))return;"
    "await fetch('/wifi/delete',{method:'POST',body:new URLSearchParams({idx})});"
    "loadWifi();}"

    // ---- 电台列表 ---- // 电台列表函数
    "async function loadStations(){"
    "const res=await fetch('/stations');const stations=await res.json();"
    "var h='<tr><th>名称</th><th>URL</th><th>操作</th></tr>';"
    "for(var i=0;i<stations.length;i++){"
    "var s=stations[i];"
    "h+='<tr><td><input value=\"'+escAttr(s.name)+'\" id=\"name'+s.id+'\"></td>'+"
    "'<td><input value=\"'+escAttr(s.url)+'\" id=\"url'+s.id+'\"></td>'+"
    "'<td><button onclick=\"updateStation('+s.id+')\">保存</button> '+"
    "'<button class=\"del\" onclick=\"deleteStation('+s.id+')\">删除</button></td></tr>';}"
    "document.getElementById('stationsTable').innerHTML=h;}"

    "async function updateStation(id){"
    "const name=document.getElementById('name'+id).value;"
    "const url=document.getElementById('url'+id).value;"
    "await fetch('/station/update',{method:'POST',body:new URLSearchParams({id,name,url})});"
    "loadStations();}"

    "async function deleteStation(id){"
    "if(!confirm('确认删除此电台？'))return;"
    "await fetch('/station/delete',{method:'POST',body:new URLSearchParams({id})});"
    "loadStations();}"

    "async function addStation(){"
    "const name=document.getElementById('addName').value.trim();"
    "const url=document.getElementById('addUrl').value.trim();"
    "const st=document.getElementById('addStatus');"
    "if(!name||!url){st.innerText='请填写名称和URL';st.style.color='red';return;}"
    "const r=await fetch('/station/add',{method:'POST',body:new URLSearchParams({name,url})});"
    "if(r.ok){st.innerText='✓ 已添加';st.style.color='green';"
    "document.getElementById('addName').value='';"
    "document.getElementById('addUrl').value='';loadStations();}"
    "else{st.innerText='✗ 失败（已满）';st.style.color='red';}}"

    // ---- 重启 ---- // 重启函数
    "async function saveAndRestart(){"
    "const s=document.getElementById('restartStatus');"
    "s.innerText='正在保存并重启...';"
    "const r=await fetch('/saveRestart',{method:'POST'});"
    "s.innerText=r.ok?'✓ 设备正在重启，请稍候...':'✗ 保存失败';}"

    // ---- 工具函数 ---- // 工具函数
    "function escHtml(s){return s.replace(/&/g,'&amp;').replace(/</g,'&lt;').replace(/>/g,'&gt;');}"
    "function escAttr(s){return s.replace(/\"/g,'&quot;');}"

    // ---- 初始化 ---- // 初始化函数
    // ---- OTA 配置JS函数 ---- //
    "async function loadOtaConfig(){"
    "try{const r=await fetch('/ota/config');const d=await r.json();"
    "document.getElementById('otaVerUrl').value=d.ver_url||'';"
    "document.getElementById('otaFwUrl').value=d.fw_url||'';"
    "var tag=document.getElementById('otaCustomTag');"
    "if(d.customized){"
    "tag.innerHTML='<span style=\"color:#e67e00;\">&#9888; 已使用自定义地址</span>';"
    "}else{"
    "tag.innerHTML='<span style=\"color:#28a745;\">&#10003; 使用内置默认地址</span>';"
    "}}"
    "catch(e){}}"

    "async function saveOtaConfig(){"
    "const vv=document.getElementById('otaVerUrl').value.trim();"
    "const ff=document.getElementById('otaFwUrl').value.trim();"
    "const st=document.getElementById('otaStatus');"
    "st.textContent='\u4fdd\u5b58\u4e2d...';st.style.color='#666';"
    "const p=new URLSearchParams();p.append('ver_url',vv);p.append('fw_url',ff);"
    "const r=await fetch('/ota/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});"
    "const d=await r.json();"
    "if(d.ok){st.textContent='\u2713 \u5df2\u4fdd\u5b58';st.style.color='green';loadOtaConfig();}"
    "else{st.textContent='\u2717 \u5931\u8d25';st.style.color='red';}}"

    "async function resetOtaConfig(){"
    "if(!confirm('\u786e\u8ba4\u6062\u590d\u9ed8\u8ba4 OTA \u5730\u5740\uff1f'))return;"
    "const st=document.getElementById('otaStatus');"
    "st.textContent='\u6062\u590d\u4e2d...';st.style.color='#666';"
    "const p=new URLSearchParams();p.append('ver_url','');p.append('fw_url','');"
    "const r=await fetch('/ota/config',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:p.toString()});"
    "const d=await r.json();"
    "if(d.ok){st.textContent='\u2713 '+d.msg;st.style.color='green';"
    "document.getElementById('otaVerUrl').value='';"
    "document.getElementById('otaFwUrl').value='';loadOtaConfig();}"
    "else{st.textContent='\u2717 \u5931\u8d25';st.style.color='red';}}"

    "loadWifi();loadStations();loadNaviConfig();loadOtaConfig();"
    "</script></body></html>";

  server.send(200, "text/html", html); // 发送 HTML 响应
}

// ---- Web 处理器：保存 WiFi（兼容旧接口，写到 slot 0） ---- // Web 处理器保存 WiFi 函数的说明
void handleConfig() { // 处理 WiFi 配置的函数
  String newSSID_  = server.arg("ssid"); // 获取 SSID 参数
  String newPass_  = server.arg("password"); // 获取密码参数
  if (newSSID_.length() > 0) { // 如果 SSID 不为空
    addWiFiEntry(newSSID_, newPass_); // 添加 WiFi 条目
    server.send(200, "text/html", "WiFi 配置已保存!<br>请点击「保存并重启」按钮"); // 发送成功响应
  } else { // 如果 SSID 为空
    server.send(400, "text/html", "错误: SSID不能为空"); // 发送错误响应
  }
}

// ---- Web 处理器：保存并重启 ---- // Web 处理器保存并重启函数的说明
void handleSaveRestart() { // 处理保存并重启的函数
  // 无论从哪个配置页面保存，重启后均回到默认播放界面
  // 不自动进入 Navidrome 或 SD 模式，由用户自行选择
  naviNeedsConfig = false;
  preferences.putBool("startInNaviMode", false); // 确保重启后不自动进入 Navidrome 模式
  preferences.putBool("restartFromSD",   false); // 确保重启后不自动进入 SD 模式
  preferences.putBool("wifiConfigMode",  false); // 清除WiFi配置模式标志，避免重启后再次进入配置模式
  configPortalExiting = true; // 标记配置门户退出
  menuState = MENU_SD_EXITING; // 设置菜单状态
  sdExitingInProgress = true; // 标记 SD 退出进行中
  sdExitStartTime = millis(); // 记录退出开始时间
  configMode = false; // 关闭配置模式
  WiFi.softAPdisconnect(true); // 断开软 AP 连接
  dnsServer.stop(); // 停止 DNS 服务器
  server.send(200, "text/html", "设备正在重启..."); // 发送响应
}

// ---- 启动配置门户 ---- // 启动配置门户函数的说明
void startConfigPortal() { // 启动配置门户的函数
  configMode = true; // 开启配置模式
  configStartTime = millis(); // 记录配置开始时间
  loadStations(); // 加载电台列表

  WiFi.softAP(AP_SSID, AP_PASS); // 启动软 AP
  delay(500); // 延迟 500 毫秒

  IPAddress apIP(192, 168, 3, 3); // 设置 AP IP 地址
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // 配置软 AP
  dnsServer.start(53, "*", apIP); // 启动 DNS 服务器

  // 路由已在main.cpp setup()中统一注册，此处不重复调用

  strcpy(stationString, "Config Mode:"); // 复制配置模式提示
  strcpy(infoString, "Connect to: " AP_SSID); // 复制连接提示
  Serial.print("AP IP: "); Serial.println(WiFi.softAPIP()); // 打印 AP IP 地址
}

// ---- 非阻塞 WiFi 连接启动（仅添加 AP 并发起第一次 run，立刻返回） ----
// 调用方应将 menuState 设为 MENU_WIFI_CONNECTING，由 loop() 轮询结果。
// 返回值：true = 已加载到 AP 并开始连接；false = 无可用 WiFi，已直接进入配置门户
bool startConnectToWiFi() {
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT];
  int cnt = loadSavedWiFiList(ssids, passes);

  // 兼容旧版单账号 key（迁移一次）
  if (cnt == 0) {
    String oldSSID = preferences.getString("ssid", "");
    String oldPass = preferences.getString("password", "");
    if (oldSSID.length() > 0) {
      addWiFiEntry(oldSSID, oldPass);
      preferences.remove("ssid");
      preferences.remove("password");
      cnt = loadSavedWiFiList(ssids, passes);
    }
  }

  if (cnt == 0) {
    Serial.println("[WiFi] No saved AP, starting config portal");
    startConfigPortal();
    return false;
  }

  WiFi.mode(WIFI_STA);
  delay(10);
  for (int i = 0; i < MAX_WIFI_COUNT; i++) {
    if (ssids[i].length() > 0) {
      wifiMulti.addAP(ssids[i].c_str(), passes[i].c_str());
      Serial.print("[WiFi] Added AP: "); Serial.println(ssids[i]);
    }
  }
  // 不调用 WiFi.scanNetworks()：扫描会为每个发现的 AP 分配约 200 字节缓冲区
  // （14个AP ≈ 2.8KB），更严重的是造成堆碎片，使 maxAllocHeap 从 ~100KB 降到 ~41KB，
  // 导致 Navidrome TLS 握手无法获得足够的连续内存块。
  // wifiMulti.run() 自身会在连接时扫描并匹配信号最强的已保存AP，无需手动预扫描。
  Serial.println("[WiFi] Connecting with wifiMulti (no pre-scan)...");
  wifiMulti.run();
  return true;
}

// ---- 阻塞版 connectToWiFi（供 restartFromSD 等已有代码路径使用） ----
// 注意：此函数在 OLED 任务已运行后调用，阻塞时间较短（2 秒）可接受
void connectToWiFi() {
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT];
  int cnt = loadSavedWiFiList(ssids, passes);

  if (cnt == 0) {
    String oldSSID = preferences.getString("ssid", "");
    String oldPass = preferences.getString("password", "");
    if (oldSSID.length() > 0) {
      addWiFiEntry(oldSSID, oldPass);
      preferences.remove("ssid");
      preferences.remove("password");
      cnt = loadSavedWiFiList(ssids, passes);
    }
  }

  if (cnt == 0) {
    startConfigPortal();
    return;
  }

  WiFi.mode(WIFI_STA);
  delay(10);  // 短暂延迟让WiFi驱动稳定
  for (int i = 0; i < MAX_WIFI_COUNT; i++) {
    if (ssids[i].length() > 0) {
      wifiMulti.addAP(ssids[i].c_str(), passes[i].c_str());
      Serial.print("[WiFi] Added AP: "); Serial.println(ssids[i]);
    }
  }

  Serial.println("[WiFi] Connecting with wifiMulti...");
  int attempts = 0;
  while (wifiMulti.run() != WL_CONNECTED && attempts < 4) {  // 最多等待 2 秒（4×500ms）
    delay(500); attempts++;
    Serial.print(".");
    esp_task_wdt_reset();
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    WiFi.scanDelete();  // 释放 wifiMulti.run() 内部扫描产生的缓冲区
    strcpy(stationString, "Connected: ");
    strncat(stationString, WiFi.SSID().c_str(), MAX_INFO_LENGTH - strlen(stationString) - 1);
    if (strlen(lastHost) > 0) strcpy(infoString, "Reconnecting to stream");
    else                      strcpy(infoString, "Ready to play");
    Serial.print("[WiFi] Connected to: "); Serial.println(WiFi.SSID());
  } else {
    // 连接失败：显示失败提示，由 loop() 里的超时逻辑决定是否进配置门户
    strcpy(stationString, "WiFi连接失败");
    strcpy(infoString, "请检查账号密码");
    Serial.println("[WiFi] Connection failed in connectToWiFi()");
  }
}

// ---- loop() 中轮询 WiFi 连接状态（配合 MENU_WIFI_CONNECTING 使用） ----
// 首次调用时执行 WiFi 初始化（包含阻塞扫描），之后轮询连接状态
// 连接成功 → 初始化音频，进入正常播放状态
// 连接超时/失败 → 显示错误画面 3 秒，然后自动触发配置门户
void pollWiFiConnection() {
  if (menuState != MENU_WIFI_CONNECTING) return;

  static bool wifiInitialized = false;

  // ---- 首次调用：若 WiFi 已连接则直接跳过扫描和重连，避免 scanNetworks() 造成堆碎片 ----
  if (!wifiInitialized) {
    wifiInitialized = true;

    if (WiFi.status() == WL_CONNECTED) {
      // WiFi 已连接（例如从 SD 播放器退出后由 MENU_SD_EXITING 清理流程保留了连接）
      // 跳过 startConnectToWiFi()，直接进入连接成功处理逻辑
      Serial.println("[WiFi] Already connected, skipping scan");
    } else {
      Serial.println("[WiFi] Initializing connection...");
      bool wifiStarted = startConnectToWiFi();  // 包含阻塞式网络扫描
      if (!wifiStarted) {
        strcpy(stationString, "无已保存WiFi");
        strcpy(infoString, "正在进入配置...");
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        startConfigPortal();
        menuState = MENU_NONE;
        wifiInitialized = false;
        return;
      }
      strcpy(stationString, "正在连接WiFi...");
      strcpy(infoString, "请稍候");
      menuStartTime = millis();
    }
  }

  // ---- 轮询 wifiMulti 连接状态 ----
  wifiMulti.run();

  if (WiFi.status() == WL_CONNECTED) {
    wifiInitialized = false;
    Serial.print("[WiFi] Connected to: "); Serial.println(WiFi.SSID());

    // 连接成功后立即释放 WiFi 扫描结果缓冲区（如果之前有残留）。
    // wifiMulti.run() 内部会做一次扫描，scanDelete() 归还这部分内存，
    // 提升 maxAllocHeap，确保后续 Navidrome TLS 握手有足够的连续内存块。
    WiFi.scanDelete();
    Serial.printf("[WiFi] Post-connect heap: free=%uKB maxAlloc=%uKB\n",
                  ESP.getFreeHeap() / 1024, ESP.getMaxAllocHeap() / 1024);

    strcpy(stationString, "Connected: ");
    strncat(stationString, WiFi.SSID().c_str(), MAX_INFO_LENGTH - strlen(stationString) - 1);
    strcpy(infoString, "Ready to play");

    audio.setPinout(I2S_BCLK, I2S_LRC, I2S_DOUT);
    audio.setVolume(encoderPos);
    loadStations();

    if (otaWiFiConnect) {
      otaWiFiConnect = false;
      menuState      = MENU_OTA_CHECK;
      menuIndex      = 0;
      menuStartTime  = millis();
      otaCheckStartTime = millis();
    } else if (naviWiFiConnect) {
      naviWiFiConnect = false;
      menuState = MENU_NAVI_PLAYER;
      menuStartTime = millis();
      naviState = NAVI_CONNECTING;
      strncpy(stationString, "Navidrome", MAX_INFO_LENGTH - 1);
      strncpy(infoString, "连接中...", MAX_INFO_LENGTH - 1);
      naviStartRequested = true;  // 请求在主任务中执行 naviStart()
    } else if (alarmWiFiConnect) {
      alarmWiFiConnect = false;
      audio.stopSong();        // 确保音频已停止（防止WiFi连接中途音频仍在运行）
      audioLoopPaused = true;  // 闹钟界面不需要音频循环
      setupNTP();  // 启动NTP时间同步
      menuState = MENU_ALARM_CLOCK;
      menuIndex = 0;
      menuStartTime = millis();
      digitalWrite(I2S_SD_MODE, 0);  // 关闭功放省电
      strcpy(stationString, "音乐闹钟");
      strcpy(infoString, "NTP同步中...");
    } else {
      // safeStopAudioSystem()（SD退出清理时调用）会将 audioLoopPaused 设为 true。
      // 进入网络流播放前必须恢复，否则 loop() 中 audio.loop() 永远不执行，没有声音。
      audioLoopPaused = false;
      digitalWrite(I2S_SD_MODE, 1);

      // 设置连接状态标志，防止 audio_info 回调在收到 "Connect to new host" 时
      // 误触发 aggressiveMemoryCleanup + audio.stopSong()，从而中断尚未建立的连接。
      // stream ready 时 audio_info 会清除此标志并将 menuState 切回 MENU_NONE。
      stationConnectingInProgress = true;
      stationConnectStartTime     = millis();
      stationConnectRetryCount    = 0;
      connectingStationName       = stationNames[7];
      connectingStationUrl        = stationUrls[7];
      stationConnectFailed        = false;
      menuState = MENU_STATION_CONNECTING;
      menuIndex = 0;
      menuStartTime = millis();
      stationConnectAnimationAngle = random(0, 12) * 30;
      audio.connecttohost(stationUrls[7].c_str());
      strcpy(stationString, stationNames[7].c_str());
      strcpy(infoString, "Connecting...");
    }
    return;
  }

  // ---- 超时检测 ----
  unsigned long elapsed = millis() - menuStartTime;
  if (elapsed > 30000) {  // 30 秒仍未连上
    wifiInitialized = false;  // 重置初始化标志
    Serial.println("[WiFi] Connection timeout, entering config portal");

    // 先在屏幕上显示失败信息 3 秒（由 OLED 任务渲染，此处仅更新字符串）
    strcpy(stationString, "WiFi 无法连接");
    strcpy(infoString,    "密码错误或无信号");

    // 小延时让 OLED 任务有机会渲染这一帧
    vTaskDelay(3000 / portTICK_PERIOD_MS);  // 注意：此函数在 loop() 任务中调用，delay 可用

    // 进入配置门户
    startConfigPortal();
    menuState = MENU_NONE;  // configMode=true 后 OLED 会显示配网界面
  }
}

// ---- NTP 时间同步 ---- // NTP 时间同步函数的说明
void setupNTP() { // 设置 NTP 时间同步的函数
  configTime(8 * 3600, 0, "ntp.aliyun.com", "ntp1.aliyun.com", "pool.ntp.org"); // 配置时区和 NTP 服务器
}

// ---- Web API：WiFi 列表（返回 JSON，密码不回传） ---- // WiFi 列表 API 函数的说明
void handleWifiList() { // 处理 WiFi 列表请求的函数
  String ssids[MAX_WIFI_COUNT], passes[MAX_WIFI_COUNT]; // 创建临时数组
  loadSavedWiFiList(ssids, passes); // 加载已保存的 WiFi 列表

  String json = "["; // 初始化 JSON 字符串
  bool first = true; // 初始化首标志
  for (int i = 0; i < MAX_WIFI_COUNT; i++) { // 遍历所有 WiFi 槽位
    if (ssids[i].length() > 0) { // 如果 SSID 不为空
      if (!first) json += ","; // 如果不是第一个则添加逗号
      // 转义 SSID 中的引号 // 注释：转义引号
      String escaped = ssids[i]; // 复制 SSID
      escaped.replace("\"", "\\\""); // 转义引号
      json += "{\"idx\":" + String(i) + ",\"ssid\":\"" + escaped + "\"}"; // 添加 JSON 条目
      first = false; // 设为非首标志
    }
  }
  json += "]"; // 添加结束括号
  server.send(200, "application/json", json); // 发送 JSON 响应
}

// ---- Web API：新增 WiFi ---- // 新增 WiFi API 函数的说明
void handleWifiAdd() { // 处理新增 WiFi 请求的函数
  String ssid = server.arg("ssid"); // 获取 SSID 参数
  String pass = server.arg("pass"); // 获取密码参数
  if (ssid.length() == 0) { // 如果 SSID 为空
    server.send(400, "text/plain", "SSID不能为空"); // 发送错误响应
    return; // 返回
  }
  if (addWiFiEntry(ssid, pass)) { // 如果添加成功
    Serial.print("[WiFi] Saved new AP: "); Serial.println(ssid); // 打印成功信息
    server.send(200, "text/plain", "OK"); // 发送成功响应
  } else { // 如果添加失败
    server.send(400, "text/plain", "已达上限（最多5条）"); // 发送错误响应
  }
}

// ---- Web API：删除 WiFi ---- // 删除 WiFi API 函数的说明
void handleWifiDelete() { // 处理删除 WiFi 请求的函数
  int idx = server.arg("idx").toInt(); // 获取索引参数
  if (idx >= 0 && idx < MAX_WIFI_COUNT) { // 如果索引有效
    deleteWiFiEntry(idx); // 删除 WiFi 条目
    Serial.print("[WiFi] Deleted AP at index: "); Serial.println(idx); // 打印删除信息
    server.send(200, "text/plain", "OK"); // 发送成功响应
  } else { // 如果索引无效
    server.send(400, "text/plain", "参数错误"); // 发送错误响应
  }
}

// ---- Web API：扫描周边 WiFi（返回 JSON 列表，供前端下拉菜单使用） ----
void handleWifiScan() {
  // 异步扫描，最多等待 6 秒
  int n = WiFi.scanNetworks(false, true);  // false=同步, true=显示隐藏SSID
  if (n == WIFI_SCAN_FAILED || n < 0) n = 0;

  // 按信号强度排序（简单冒泡）
  for (int i = 0; i < n - 1; i++) {
    for (int j = i + 1; j < n; j++) {
      if (WiFi.RSSI(j) > WiFi.RSSI(i)) {
        // 交换（WiFi库无swap，用SSID比较标记即可，直接输出时排序）
        // 注：Arduino WiFi库不支持直接swap，改为输出时按RSSI降序插入排序
      }
    }
  }

  // 构造已排序的索引数组（插入排序，按 RSSI 降序）
  int idx[32];
  int cnt = min(n, 32);
  for (int i = 0; i < cnt; i++) idx[i] = i;
  for (int i = 1; i < cnt; i++) {
    int key = idx[i];
    int j = i - 1;
    while (j >= 0 && WiFi.RSSI(idx[j]) < WiFi.RSSI(key)) {
      idx[j + 1] = idx[j];
      j--;
    }
    idx[j + 1] = key;
  }

  String json = "[";
  for (int i = 0; i < cnt; i++) {
    int k = idx[i];
    String ssid = WiFi.SSID(k);
    if (ssid.length() == 0) continue;           // 跳过空 SSID
    ssid.replace("\\", "\\\\");
    ssid.replace("\"", "\\\"");
    int rssi = WiFi.RSSI(k);
    bool enc  = (WiFi.encryptionType(k) != WIFI_AUTH_OPEN);
    if (i > 0) json += ",";
    json += "{\"ssid\":\"" + ssid + "\",\"rssi\":" + String(rssi)
          + ",\"enc\":" + (enc ? "true" : "false") + "}";
  }
  json += "]";

  WiFi.scanDelete();  // 释放扫描结果内存
  server.send(200, "application/json", json);
}

// ---- Web API：电台列表 ---- // 电台列表 API 函数的说明
void handleStationsList() { // 处理电台列表请求的函数
  String json = "["; // 初始化 JSON 字符串
  for (int i = 0; i < MAX_STATIONS; i++) { // 遍历所有电台槽位
    if (stationNames[i] != "") { // 如果电台名称不为空
      if (json.length() > 1) json += ","; // 如果不是第一个则添加逗号
      String escapedName = stationNames[i]; escapedName.replace("\"", "\\\""); // 转义名称中的引号
      String escapedUrl  = stationUrls[i];  escapedUrl.replace("\"", "\\\""); // 转义 URL 中的引号
      json += "{\"id\":" + String(i) + ",\"name\":\"" + escapedName + "\",\"url\":\"" + escapedUrl + "\"}"; // 添加 JSON 条目
    }
  }
  json += "]"; // 添加结束括号
  server.send(200, "application/json", json); // 发送 JSON 响应
}

// ---- Web API：更新电台 ---- // 更新电台 API 函数的说明
void handleStationUpdate() { // 处理更新电台请求的函数
  int    id   = server.arg("id").toInt(); // 获取 ID 参数
  String name = server.arg("name"); // 获取名称参数
  String url  = server.arg("url"); // 获取 URL 参数
  if (id >= 0 && id < MAX_STATIONS && name.length() > 0 && url.length() > 0) { // 如果参数有效
    stationNames[id] = name; stationUrls[id] = url; // 更新电台信息
    saveStation(id); // 保存电台
    server.send(200, "text/plain", "OK"); // 发送成功响应
  } else { // 如果参数无效
    server.send(400, "text/plain", "参数错误"); // 发送错误响应
  }
}

// ---- Web API：删除电台 ---- // 删除电台 API 函数的说明
void handleStationDelete() { // 处理删除电台请求的函数
  int id = server.arg("id").toInt(); // 获取 ID 参数
  if (id >= 0 && id < MAX_STATIONS) { // 如果 ID 有效
    deleteStation(id); // 删除电台
    server.send(200, "text/plain", "OK"); // 发送成功响应
  } else { // 如果 ID 无效
    server.send(400, "text/plain", "参数错误"); // 发送错误响应
  }
}

// ---- Web API：新增电台 ---- // 新增电台 API 函数的说明
void handleStationAdd() { // 处理新增电台请求的函数
  String name = server.arg("name"); // 获取名称参数
  String url  = server.arg("url"); // 获取 URL 参数
  if (name.length() > 0 && url.length() > 0) { // 如果参数有效
    if (addStationToStorage(name, url)) server.send(200, "text/plain", "OK"); // 如果添加成功则发送成功响应
    else                               server.send(400, "text/plain", "No empty slot"); // 否则发送错误响应
  } else { // 如果参数无效
    server.send(400, "text/plain", "参数错误"); // 发送错误响应
  }
}


// ===================================================================
// Web API：OTA 服务器 URL 配置  GET /ota/config  POST /ota/config
//
// GET  → 返回当前 URL（JSON）
// POST → 接收 ver_url / fw_url 参数，写入 NVS 并立即生效
//        留空某项则保留原值；两项均留空则恢复默认值
// ===================================================================
void handleOtaConfig() {
  if (server.method() == HTTP_GET) {
    // 判断是否为自定义URL（NVS中有存储值）
    // 不向前端暴露默认URL明文，只返回自定义部分和是否自定义的标志
    String nvsVer = preferences.getString(OTA_NVS_KEY_VER_URL, "");
    String nvsFw  = preferences.getString(OTA_NVS_KEY_FW_URL,  "");
    bool customized = (nvsVer.length() > 0 || nvsFw.length() > 0);
    // ver_url / fw_url 只返回 NVS 中的自定义值，默认值不下发到前端
    String escaped_ver = nvsVer; escaped_ver.replace("\"", "\\\"");
    String escaped_fw  = nvsFw;  escaped_fw.replace("\"", "\\\"");
    String json = "{\"ver_url\":\"" + escaped_ver + "\","
                + "\"fw_url\":\""  + escaped_fw  + "\","
                + "\"customized\":" + (customized ? "true" : "false") + "}";
    server.send(200, "application/json", json);
    return;
  }

  // POST：更新 URL
  String newVer = server.arg("ver_url"); newVer.trim();
  String newFw  = server.arg("fw_url");  newFw.trim();

  // 两项均为空 → 恢复编译期默认值并清除 NVS 记录
  if (newVer.length() == 0 && newFw.length() == 0) {
    preferences.remove(OTA_NVS_KEY_VER_URL);
    preferences.remove(OTA_NVS_KEY_FW_URL);
    OTA_VERSION_URL  = String(OTA_DEFAULT_VER_URL);
    OTA_FIRMWARE_URL = String(OTA_DEFAULT_FW_URL);
    Serial.println(F("[OTA] URLs reset to defaults."));
    server.send(200, "application/json", "{\"ok\":true,\"msg\":\"已恢复默认URL\"}");
    return;
  }

  // 只更新非空项，留空项保持原值
  if (newVer.length() == 0) newVer = OTA_VERSION_URL;
  if (newFw.length()  == 0) newFw  = OTA_FIRMWARE_URL;

  saveOtaUrls(newVer, newFw);
  server.send(200, "application/json", "{\"ok\":true}");
}

#endif // WIFI_CONFIG_H // 结束头文件保护