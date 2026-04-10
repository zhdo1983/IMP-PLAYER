#ifndef MEMORY_MONITOR_H
#define MEMORY_MONITOR_H

#include "globals.h"
#include <esp_task_wdt.h>

// 内存监控类
class MemoryMonitor {
private:
    uint32_t lastFreeHeap = 0;
    uint32_t lastMaxAllocHeap = 0;
    uint32_t lastMinFreeHeap = 0;
    unsigned long lastPrintTime = 0;
    const unsigned long PRINT_INTERVAL = 60000;  // 60秒打印一次

public:
    // 打印内存状态
    void printMemoryStatus() {
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t freeHeap = ESP.getFreeHeap();
        uint32_t minFreeHeap = ESP.getMinFreeHeap();
        uint32_t maxAllocHeap = ESP.getMaxAllocHeap();

        Serial.printf("=== Memory Status ===\n");
        Serial.printf("Total Heap:     %6u KB\n", totalHeap / 1024);
        Serial.printf("Free Heap:      %6u KB", freeHeap / 1024);
        if (lastFreeHeap > 0) {
            int diff = (int)freeHeap - (int)lastFreeHeap;
            Serial.printf(" (%+d KB)", diff / 1024);
        }
        Serial.printf("\n");
        Serial.printf("Min Free Heap:  %6u KB\n", minFreeHeap / 1024);
        Serial.printf("Max Alloc Heap: %6u KB", maxAllocHeap / 1024);
        if (lastMaxAllocHeap > 0) {
            int diff = (int)maxAllocHeap - (int)lastMaxAllocHeap;
            Serial.printf(" (%+d KB)", diff / 1024);
        }
        Serial.printf("\n");
        Serial.printf("WiFi: %s (est. %d KB)\n", WiFi.isConnected() ? "ON" : "OFF",
                     WiFi.isConnected() ? 40 : 0);
        Serial.printf("Audio: %s (est. %d KB)\n", (audio.inBufferFilled() > 0) ? "PLAYING" : "IDLE",
                     audio.inBufferFilled() > 0 ? 18 : 32);
        Serial.printf("====================\n");

        lastFreeHeap = freeHeap;
        lastMaxAllocHeap = maxAllocHeap;
        lastMinFreeHeap = minFreeHeap;
        lastPrintTime = millis();
    }

    // 定期打印内存状态
    void periodicPrint() {
        if (millis() - lastPrintTime > PRINT_INTERVAL) {
            printMemoryStatus();
        }
    }

    // 在关键操作前检查内存
    bool checkMemoryBeforeAction(const char* action, uint32_t required) {
        uint32_t available = ESP.getMaxAllocHeap();

        if (available < required) {
            Serial.printf("[MEM] %s needs %u KB, only %u KB available!\n",
                         action, required / 1024, available / 1024);
            Serial.println("[MEM] Aggressive memory cleanup triggered...");
            aggressiveMemoryCleanup();

            available = ESP.getMaxAllocHeap();
            Serial.printf("[MEM] After cleanup: %u KB available\n", available / 1024);

            return available >= required;
        }

        return true;
    }

    // 获取内存使用率
    float getMemoryUsagePercent() {
        uint32_t totalHeap = ESP.getHeapSize();
        uint32_t freeHeap = ESP.getFreeHeap();
        if (totalHeap == 0) return 0.0f;
        return 100.0f * (1.0f - (float)freeHeap / (float)totalHeap);
    }

    // 获取最大可分配内存
    uint32_t getMaxAllocHeap() {
        return ESP.getMaxAllocHeap();
    }

    // 获取空闲堆内存
    uint32_t getFreeHeap() {
        return ESP.getFreeHeap();
    }

    // 获取最小空闲堆内存
    uint32_t getMinFreeHeap() {
        return ESP.getMinFreeHeap();
    }
};

// 全局内存监控实例
extern MemoryMonitor memMonitor;

// 轻量级内存清理（用于频繁调用）
void lightMemoryCleanup() {
    // 只释放最必要的资源
    void* testBlocks[5];
    int blockSizes[] = {512, 1024, 2048, 4096, 8192};

    for (int i = 0; i < 5; i++) {
        testBlocks[i] = malloc(blockSizes[i]);
        if (testBlocks[i]) {
            memset(testBlocks[i], 0xAA, blockSizes[i]);
            free(testBlocks[i]);
            testBlocks[i] = NULL;
        }
    }
}

// 预防性内存清理（在关键操作前调用）
void preventiveMemoryCleanup() {
    Serial.println("[MEM] Preventive memory cleanup...");

    // 停止音频但不重启
    if (audio.inBufferFilled() > 0) {
        audio.stopSong();
        delay(100);
    }

    // 执行两轮清理
    for (int round = 0; round < 2; round++) {
        void* testBlocks[10];
        int blockSizes[] = {256, 512, 1024, 2048, 4096, 8192, 16384, 8192, 4096, 2048};

        for (int i = 0; i < 10; i++) {
            testBlocks[i] = malloc(blockSizes[i]);
            if (testBlocks[i]) memset(testBlocks[i], 0xAA, blockSizes[i]);
        }
        delay(2);

        for (int i = 0; i < 10; i++) {
            if (testBlocks[i]) {
                free(testBlocks[i]);
                testBlocks[i] = NULL;
            }
        }
        delay(1);
    }

    ESP.getFreeHeap();
    Serial.printf("[MEM] Cleanup complete. Max alloc: %u KB\n",
                 ESP.getMaxAllocHeap() / 1024);
}

#endif // MEMORY_MONITOR_H
