// SPDX-License-Identifier: GPL-3.0-only
/*
 * Copyright (c) 2008-2023 100askTeam : Dongshan WEI <weidongshan@100ask.net> 
 * Discourse:  https://forums.100ask.net
 */
 
/*  Copyright (C) 2008-2023 深圳百问网科技有限公司
 *  All rights reserved
 *
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 可以用于商业用途, 但百问网不承担任何后果！
 * 
 * 本程序遵循GPL V3协议, 请遵循协议
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 本程序所用开发板 : Linux开发板
 * 百问网官方淘宝   : https://100ask.taobao.com
 * 联系我们(E-mail) : weidongshan@100ask.net
 *
 *          版权所有，盗版必究。
 *  
 * 修改历史     版本号           作者        修改内容
 *-----------------------------------------------------
 * 2025.03.20      v01         百问科技      创建文件
 *-----------------------------------------------------
 */
#include <cfg.h>
#include <ipc_udp.h>
#include <cJSON.h>
#include <pthread.h>
#include <string.h>
#include "lv_100ask_xz_ai_main.h"
#include "lang_config.h"

#define UI_TEXT_MAX_LEN    1024
#define UI_STATE_MAX_LEN   64
#define UI_EMOTION_MAX_LEN 32

// 定义设备状态枚举类型
typedef enum DeviceState {
    kDeviceStateUnknown,
    kDeviceStateStarting,
    kDeviceStateWifiConfiguring,
    kDeviceStateIdle,
    kDeviceStateConnecting,
    kDeviceStateListening,
    kDeviceStateSpeaking,
    kDeviceStateUpgrading,
    kDeviceStateActivating,
    kDeviceStateFatalError
} DeviceState;

// 静态变量，用于存储IPC端点
static p_ipc_endpoint_t g_ipc_ep;
static pthread_mutex_t g_ui_update_lock = PTHREAD_MUTEX_INITIALIZER;

struct pending_ui_update_s
{
    bool state_valid;
    bool text_valid;
    bool emotion_valid;
    bool wifi_valid;
    char state[UI_STATE_MAX_LEN];
    char text[UI_TEXT_MAX_LEN];
    char emotion[UI_EMOTION_MAX_LEN];
    int wifi;
};

static struct pending_ui_update_s g_pending_ui_update;

static void apply_pending_ui_update(lv_timer_t *timer)
{
    struct pending_ui_update_s pending;

    LV_UNUSED(timer);

    memset(&pending, 0, sizeof(pending));

    pthread_mutex_lock(&g_ui_update_lock);
    pending = g_pending_ui_update;
    memset(&g_pending_ui_update, 0, sizeof(g_pending_ui_update));
    pthread_mutex_unlock(&g_ui_update_lock);

    if (pending.state_valid)
        {
            SetStateString(pending.state);
        }

    if (pending.text_valid)
        {
            SetText(pending.text);
        }

    if (pending.emotion_valid)
        {
            SetEmotion(pending.emotion);
        }

    if (pending.wifi_valid)
        {
            SetWifi(pending.wifi);
        }
}

// 将设备状态转换为本地字符串
static const char* ConvertToLocalString(DeviceState state)
{
    switch (state) {
        case kDeviceStateUnknown:
            return UNKNOWN_STATUS;
        case kDeviceStateStarting:
            return INITIALIZING;
        case kDeviceStateWifiConfiguring:
            return NETWORK_CFG;
        case kDeviceStateIdle:
            return STANDBY;
        case kDeviceStateConnecting:
            return CONNECTING;
        case kDeviceStateListening:
            return LISTENING;
        case kDeviceStateSpeaking:
            return SPEAKING;
        case kDeviceStateUpgrading:
            return UPGRADING;
        case kDeviceStateActivating:
            return ACTIVATION;
        case kDeviceStateFatalError:
            return ERROR_STR;
    }

    return "未知状态";
}

/*
 * 处理从IPC接收到的UI数据。
 * 处理的数据格式:
 * 1. 状态: {"state": 0}等, 取值对应DeviceState的取值
 * 2. 要显示的文本: {"text": "你好"}
 * 3. 要显示的emotion: {"emotion": "happy"}, 有这些取值:
 *           "neutral","happy","laughing","funny","sad","angry","crying","loving",
 *           "embarrassed","surprised","shocked","thinking","winking","cool","relaxed",
 *           "delicious","kissy","confident","sleepy","silly","confused"
 * 4. WIFI强度: {"wifi": 100}
 * 5. 电量: {"battery": 100}
 *
 * @param buffer 包含JSON格式数据的字符串缓冲区
 * @param size 缓冲区的大小
 * @param user_data 用户数据指针（未使用）
 * @return 0 表示成功，-1 表示解析错误
 */
static int process_ui_data(char *buffer, size_t size, void *user_data)
{
    cJSON *json;
    const char *state_str;

    // 解析JSON数据
    json = cJSON_Parse(buffer);
    if (!json) {
        LV_LOG_USER("cJSON_Parse err: %s ", buffer);
        return -1;
    }

    // 获取状态字段
    cJSON *state = cJSON_GetObjectItem(json, "state");
    if (state) {
        state_str = ConvertToLocalString((DeviceState)state->valueint);
        pthread_mutex_lock(&g_ui_update_lock);
        strlcpy(g_pending_ui_update.state, state_str,
                sizeof(g_pending_ui_update.state));
        g_pending_ui_update.state_valid = true;
        if (state->valueint == kDeviceStateSpeaking)
            {
                strlcpy(g_pending_ui_update.emotion, "laughing",
                        sizeof(g_pending_ui_update.emotion));
                g_pending_ui_update.emotion_valid = true;
            }
        if (state->valueint == kDeviceStateListening)
            {
                strlcpy(g_pending_ui_update.emotion, "happy",
                        sizeof(g_pending_ui_update.emotion));
                g_pending_ui_update.emotion_valid = true;
            }
        pthread_mutex_unlock(&g_ui_update_lock);
    }

    // 获取文本字段
    cJSON *text = cJSON_GetObjectItem(json, "text");
    if (cJSON_IsString(text) && text->valuestring) {
        pthread_mutex_lock(&g_ui_update_lock);
        strlcpy(g_pending_ui_update.text, text->valuestring,
                sizeof(g_pending_ui_update.text));
        g_pending_ui_update.text_valid = true;
        pthread_mutex_unlock(&g_ui_update_lock);
    }

    // 获取WIFI强度字段（未处理）
    cJSON *wifi = cJSON_GetObjectItem(json, "wifi");
    if (wifi) {
        pthread_mutex_lock(&g_ui_update_lock);
        g_pending_ui_update.wifi = wifi->valueint;
        g_pending_ui_update.wifi_valid = true;
        pthread_mutex_unlock(&g_ui_update_lock);
    }

    // 获取电量字段（未处理）
    cJSON *battery = cJSON_GetObjectItem(json, "battery");
    if (battery) {
        // 处理电量
    }

    // 释放JSON对象
    cJSON_Delete(json);

    return 0;
}

int SendState(char *state)
{
    return g_ipc_ep->send(g_ipc_ep, state, strlen(state)+1);
}

/*
 * 初始化UI系统。
 * 创建IPC端点，用于接收和处理UI数据。
 *
 * @return 0 表示成功，-1 表示创建IPC端点失败
 */
int ui_system_init(void)
{
    // 创建UDP IPC端点
    g_ipc_ep = ipc_endpoint_create_udp(UI_PORT_DOWN, UI_PORT_UP, process_ui_data, NULL);
    if (!g_ipc_ep) {
        LV_LOG_ERROR("Failed to create IPC endpoint\n");
        return -1;
    }

    lv_timer_create(apply_pending_ui_update, 50, NULL);
    return 0;
}
