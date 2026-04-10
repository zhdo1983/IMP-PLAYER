#ifndef STATIONS_H  // 防止头文件重复包含
#define STATIONS_H  // 定义电台头文件宏

#include "globals.h"  // 包含全局变量头文件

// 获取有效电台数量  // 获取有效电台数量函数注释
int getValidStationCount() {  // 返回当前保存的有效电台数量
  int count = 0;  // 计数器初始化为0
  for (int i = 0; i < MAX_STATIONS; i++) {  // 遍历所有电台槽位
    if (stationNames[i] != "" && stationUrls[i] != "") count++;  // 如果名称和URL都有效则计数+1
  }
  return count;  // 返回有效电台数量
}

// 获取第n个有效电台在数组中的真实索引  // 获取有效电台索引函数注释
int getValidStationIndex(int n) {  // 将逻辑序号转换为数组实际索引
  int count = 0;  // 计数器
  for (int i = 0; i < MAX_STATIONS; i++) {  // 遍历所有电台槽位
    if (stationNames[i] != "" && stationUrls[i] != "") {  // 如果是有效电台
      if (count == n) return i;  // 找到第n个有效电台，返回其索引
      count++;  // 计数+1
    }
  }
  return -1;  // 未找到则返回-1
}

// 默认电台列表（内联填充）  // 填充默认电台函数注释
static void _fillDefaultStations() {  // 填充默认电台列表
  const char* names[] = {  // 默认电台名称数组
    "1.德意志广播电台 WDR Einslive",  // 德国电台
    "2.经典欧美 181.FM - Awesome 80's",  // 美国80年代电台
    "3.法国广播电台",  // 法国电台
    "4.俄罗斯 PADNO",  // 俄罗斯电台
    "5.德国 Hit-Radio FFH",  // 德国Hit电台
    "6.马来西亚 RTM Radio",  // 马来西亚电台
    "7.新加坡  88.3jia",  // 新加坡电台
    "8.天堂电台 Radio Paradise",  // 美国天堂电台
    "9.芝加哥 WZRD The Wizard 88.3 FM",  // 芝加哥FM电台
    "10.苏腊巴亚 smart FM88.9",  // 印尼电台
    "11.AFN Daegu - Korea",  // 韩国美军电台
    "12.Swiss Jazz (瑞士爵士乐)"  // 瑞士爵士电台
  };
  const char* urls[] = {  // 默认电台URL数组
    "http://wdr-1live-live.icecast.wdr.de/wdr/1live/live/mp3/128/stream.mp3",  // WDR Einslive
    "http://relay.181.fm:8010",  // 181.FM Awesome 80's
    "https://rfimonde64k.ice.infomaniak.ch/rfimonde-64.mp3",  // 法国广播
    "https://cdn.pifm.ru/mp3",  // 俄罗斯PIFM
    "http://mp3.ffh.de/radioffh/hqlivestream.mp3",  // FFH Hit Radio
    "http://22253.live.streamtheworld.com/RADIO_KLASIKAAC.aac",  // 马来西亚RTM
    "http://22283.live.streamtheworld.com/883JIA.mp3",  // 新加坡88.3
    "http://stream.radioparadise.com/mp3-128",  // Radio Paradise
    "http://wzrd.streamguys1.com/live",  // Chicago WZRD
    "https://streaming.brol.tech/rtfmlounge",  // 印尼Smart FM
    "http://playerservices.streamtheworld.com/api/livestream-redirect/AFNP_DGUAAC.aac",  // AFN Korea
    "http://stream.srg-ssr.ch/m/rsj/mp3_128"  // Swiss Jazz
  };
  for (int i = 0; i < 12; i++) {  // 遍历12个默认电台
    if (stationNames[i] == "") stationNames[i] = names[i];  // 如果名称为空则填充
    if (stationUrls[i]  == "") stationUrls[i]  = urls[i];  // 如果URL为空则填充
  }
}

// 从 NVS 加载电台  // 加载电台列表函数注释
void loadStations() {  // 从非易失性存储加载电台列表
  for (int i = 0; i < MAX_STATIONS; i++) {  // 遍历所有电台槽位
    stationNames[i] = preferences.getString(("station_name_" + String(i)).c_str(), "");  // 读取电台名称
    stationUrls[i]  = preferences.getString(("station_url_"  + String(i)).c_str(), "");  // 读取电台URL
  }
  if (stationNames[0] == "") _fillDefaultStations();  // 如果第一个电台为空则填充默认电台
}

// 保存单个电台到 NVS  // 保存电台函数注释
void saveStation(int i) {  // 将指定电台保存到NVS
  preferences.putString(("station_name_" + String(i)).c_str(), stationNames[i]);  // 保存电台名称
  preferences.putString(("station_url_"  + String(i)).c_str(), stationUrls[i]);  // 保存电台URL
}

// 删除单个电台  // 删除电台函数注释
void deleteStation(int i) {  // 删除指定位置的电台
  stationNames[i] = "";  // 清空电台名称
  stationUrls[i]  = "";  // 清空电台URL
  preferences.remove(("station_name_" + String(i)).c_str());  // 从NVS删除名称
  preferences.remove(("station_url_"  + String(i)).c_str());  // 从NVS删除URL
}

// 恢复出厂电台设置  // 恢复出厂设置函数注释
void factoryResetStations() {  // 恢复默认电台列表
  for (int i = 0; i < MAX_STATIONS; i++) deleteStation(i);  // 删除所有电台
  _fillDefaultStations();  // 填充默认电台
  for (int i = 0; i < 12; i++) saveStation(i);  // 保存默认电台到NVS
}

// 完全恢复出厂设置（清除所有NVS数据）  // 完全恢复出厂设置函数注释
void factoryResetAll() {  // 清除所有存储的配置信息
  Serial.println("[FactoryReset] Starting full factory reset...");  // 打印开始信息
  
  // 清除电台数据  // 清除电台数据
  for (int i = 0; i < MAX_STATIONS; i++) deleteStation(i);  // 删除所有电台
  _fillDefaultStations();  // 填充默认电台
  for (int i = 0; i < 12; i++) saveStation(i);  // 保存默认电台到NVS
  
  // 清除WiFi数据  // 清除WiFi数据
  for (int i = 0; i < 5; i++) {  // 遍历所有WiFi槽位
    preferences.remove(("wifi_ssid_" + String(i)).c_str());  // 删除SSID
    preferences.remove(("wifi_pass_" + String(i)).c_str());  // 删除密码
  }
  preferences.remove("ssid");  // 删除旧版SSID
  preferences.remove("password");  // 删除旧版密码
  
  // 清除Navidrome数据  // 清除Navidrome数据
  preferences.remove("nd_server");  // 删除服务器地址
  preferences.remove("nd_user");  // 删除用户名
  preferences.remove("nd_pass");  // 删除密码
  
  // 清除固件版本  // 清除固件版本
  preferences.remove("fw_ver");  // 删除版本号
  
  // 清除SD卡动画类型  // 清除SD卡动画类型
  preferences.remove("sdAnimType");  // 删除动画类型
  
  // 清除其他配置标志  // 清除其他配置标志
  preferences.remove("wifiConfigMode");  // 删除WiFi配置模式标志
  preferences.remove("restartFromSD");  // 删除SD重启标志
  preferences.remove("sdScanDone");  // 删除SD扫描完成标志
  preferences.remove("startInNaviMode");  // 删除Navi模式标志
  preferences.remove("stName");  // 删除上次电台名称
  preferences.remove("stUrl");  // 删除上次电台URL
  preferences.remove("rstSta");  // 删除重启标志
  
  Serial.println("[FactoryReset] Full factory reset completed!");  // 打印完成信息
}

// 添加电台到第一个空槽  // 添加新电台函数注释
bool addStationToStorage(const String& name, const String& url) {  // 添加新电台到第一个空位
  for (int i = 0; i < MAX_STATIONS; i++) {  // 遍历所有槽位
    if (stationNames[i] == "" && stationUrls[i] == "") {  // 找到空槽位
      stationNames[i] = name;  // 填充电台名称
      stationUrls[i]  = url;  // 填充电台URL
      saveStation(i);  // 保存到NVS
      return true;  // 返回成功
    }
  }
  return false;  // 没有空位则返回失败
}

#endif // STATIONS_H  // 结束头文件保护
