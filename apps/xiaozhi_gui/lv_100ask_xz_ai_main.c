/**
 ******************************************************************************
 * @file    lv_100ask_xz_ai_main.c
 * @author  百问科技
 * @version V1.0
 * @date    2025-3-17
 * @brief	100ask XiaoZhi AI base on LVGL
 ******************************************************************************
 * Change Logs:
 * Date           Author          Notes
 * 2025-3-17     zhouyuebiao     First version
 * 2025-11-7     zhouyuebiao     
 ******************************************************************************
 * @attention
 *
 * Copyright (C) 2008-2025 深圳百问网科技有限公司<https://www.100ask.net/>
 * All rights reserved
 *
 * 代码配套的视频教程：
 *      B站：   https://www.bilibili.com/video/BV1WE421K75k
 *      百问网：https://fnwcn.xetslk.com/s/39njGj
 *      淘宝：  https://detail.tmall.com/item.htm?id=779667445604
 *
 * 本程序遵循MIT协议, 请遵循协议！
 * 免责声明: 百问网编写的文档, 仅供学员学习使用, 可以转发或引用(请保留作者信息),禁止用于商业用途！
 * 免责声明: 百问网编写的程序, 仅供学习参考，假如被用于商业用途, 但百问网不承担任何后果！
 *
 * 百问网学习平台   : https://www.100ask.net
 * 百问网交流社区   : https://forums.100ask.net
 * 百问网LVGL文档   : https://lvgl.100ask.net
 * 百问网官方B站    : https://space.bilibili.com/275908810
 * 百问网官方淘宝   : https://100ask.taobao.com
 * 百问网微信公众号 ：百问科技 或 baiwenkeji
 * 联系我们(E-mail):  support@100ask.net 或 fae_100ask@163.com
 *
 *                             版权所有，盗版必究。
 ******************************************************************************
 */

/*********************
 *      INCLUDES
 *********************/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <pthread.h>

#include "lv_100ask_xz_ai_main.h"
#include "ui_system.h"
#include <lvgl/src/font/lv_binfont_loader.h>

static pthread_mutex_t lvgl_mutex;
static int g_soft_stanby = 0;

/*********************
 *      DEFINES
 *********************/
LV_FONT_DECLARE(font_awesome_20_4);
LV_FONT_DECLARE(font_awesome_30_4);
// https://gitee.com/weidongshan/lv_100ask_linux_desktop/blob/stm32mp157/v8/lv_100ask_app/src/stm32mp157_app/stm32mp157_set_wlan/src/set_wlan.c
/**********************
 *      TYPEDEFS
 **********************/
typedef struct _lv_100ask_xz_ai {
	lv_obj_t  * state_bar_img_wifi;
	lv_obj_t  * state_bar_label_state;
	lv_obj_t  * state_bar_img_battery;
	lv_obj_t  * img_emoji;
	lv_obj_t  * label_chat;
    lv_obj_t  * btn_set_wifi;
} T_lv_100ask_xz_ai, *PT_lv_100ask_xz_ai;

// 符号结构体
typedef struct {
    const char* name;
    const char* utf8_string;
} font_awesome_symbol_t;

/**********************
 *  STATIC PROTOTYPES
 **********************/

static void init_style(void);
static lv_obj_t * lv_100ask_wifi_page_init(lv_obj_t *parent);
static void btn_set_wifi_event_cb(lv_event_t * e);
static void wifi_list_btn_event_cb(lv_event_t * e);
static void ta_event_cb(lv_event_t * e);

static const char* font_awesome_get_utf8(const char* name);
static void screen_onclicked_event_cb(lv_event_t * e);
static void lv_100ask_xz_ai_main_deinit(void);
static int save_wifi_config(const char *ssid, const char *password);
static int load_text_font(void);
static void unload_text_font(void);
static const lv_font_t *get_text_font(void);

/**********************
 *  STATIC VARIABLES
 **********************/
static PT_lv_100ask_xz_ai g_pt_lv_100ask_xz_ai;

static lv_style_t g_style_chat_font;
static lv_style_t g_style_state_font;

#define XIAOZHI_TEXT_FONT_FILE "/etc/fonts/font_puhui_20_4.bin"
static lv_font_t *g_text_font;

static const font_awesome_symbol_t font_awesome_symbols[] = {
    {"neutral", "\xef\x96\xa4"},
    {"happy", "\xef\x84\x98"},
    {"laughing", "\xef\x96\x9b"},
    {"funny", "\xef\x96\x88"},
    {"sad", "\xee\x8e\x84"},
    {"angry", "\xef\x95\x96"},
    {"crying", "\xef\x96\xb3"},
    {"loving", "\xef\x96\x84"},
    {"embarrassed", "\xef\x95\xb9"},
    {"surprised", "\xee\x8d\xab"},
    {"shocked", "\xee\x8d\xb5"},
    {"thinking", "\xee\x8e\x9b"},
    {"winking", "\xef\x93\x9a"},
    {"cool", "\xee\x8e\x98"},
    {"relaxed", "\xee\x8e\x92"},
    {"delicious", "\xee\x8d\xb2"},
    {"kissy", "\xef\x96\x98"},
    {"confident", "\xee\x90\x89"},
    {"sleepy", "\xee\x8e\x8d"},
    {"silly", "\xee\x8e\xa4"},
    {"confused", "\xee\x8d\xad"},
};

static const size_t font_awesome_symbol_count = sizeof(font_awesome_symbols) / sizeof(font_awesome_symbols[0]);
/**********************
 *      MACROS
 **********************/


/**********************
 *   GLOBAL FUNCTIONS
 **********************/
void lv_100ask_xz_ai_main(void)
{
    g_soft_stanby = 0;
    
    pthread_mutex_init(&lvgl_mutex, NULL);
     
    /* init */
    g_pt_lv_100ask_xz_ai = (T_lv_100ask_xz_ai *)lv_malloc(sizeof(T_lv_100ask_xz_ai));

    load_text_font();
    init_style();

    /* state bar */
    lv_obj_t * cont_state_bar = lv_obj_create(lv_screen_active());
    lv_obj_remove_style_all(cont_state_bar);
    lv_obj_set_size(cont_state_bar, LV_PCT(100), LV_SIZE_CONTENT);
    lv_obj_set_align(cont_state_bar, LV_ALIGN_TOP_MID);
    lv_obj_set_style_radius(cont_state_bar, 0, 0);
    lv_obj_set_style_bg_opa(cont_state_bar, LV_OPA_60, 0);
    lv_obj_set_style_pad_hor(cont_state_bar, 10, 0);
    lv_obj_set_layout(cont_state_bar, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(cont_state_bar, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(cont_state_bar, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

    // wifi
    // {"wifi",         "\xef\x87\xab"},
    // {"wifi_fair",    "\xef\x9a\xab"},
    // {"wifi_weak",    "\xef\x9a\xaa"},
    // {"wifi_slash",   "\xef\x9a\xac"},
    g_pt_lv_100ask_xz_ai->state_bar_img_wifi = lv_label_create(cont_state_bar); //lv_image_create(cont_state_bar);
    lv_obj_set_style_text_font(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, &font_awesome_20_4, 0);
    SetWifi(0);

    // state
    g_pt_lv_100ask_xz_ai->state_bar_label_state = lv_label_create(cont_state_bar);
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->state_bar_label_state, &g_style_state_font, 0);
    lv_obj_set_width(g_pt_lv_100ask_xz_ai->state_bar_label_state, LV_PCT(70));
    lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_label_state, "待命");

    // battery
    //{"battery_full", "\xef\x89\x80"},
    //{"battery_three_quarters", "\xef\x89\x81"},
    //{"battery_half", "\xef\x89\x82"},
    //{"battery_quarter", "\xef\x89\x83"},
    //{"battery_empty", "\xef\x89\x84"},
    //{"battery_slash", "\xef\x8d\xb7"},
    //{"battery_bolt", "\xef\x8d\xb6"},
    g_pt_lv_100ask_xz_ai->state_bar_img_battery = lv_label_create(cont_state_bar);
    lv_obj_set_style_text_font(g_pt_lv_100ask_xz_ai->state_bar_img_battery, &font_awesome_20_4, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_img_battery, "\xef\x89\x80");


    /* emoji */
    // https://www.iconfont.cn/search/index?searchType=icon&q=%E5%9C%86%E8%84%B8%E8%A1%A8%E6%83%85
    g_pt_lv_100ask_xz_ai->img_emoji = lv_label_create(lv_screen_active());
    lv_obj_set_style_text_font(g_pt_lv_100ask_xz_ai->img_emoji, &font_awesome_30_4, 0);
    lv_obj_align(g_pt_lv_100ask_xz_ai->img_emoji, LV_ALIGN_CENTER, 0, -40);


    /* chat */
    g_pt_lv_100ask_xz_ai->label_chat = lv_label_create(lv_screen_active());
    lv_obj_set_width(g_pt_lv_100ask_xz_ai->label_chat, LV_PCT(90));
    lv_obj_add_style(g_pt_lv_100ask_xz_ai->label_chat, &g_style_chat_font, 0);
    lv_label_set_text(g_pt_lv_100ask_xz_ai->label_chat, "尚未连接到服务器...");
    lv_obj_align_to(g_pt_lv_100ask_xz_ai->label_chat, g_pt_lv_100ask_xz_ai->img_emoji, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);


    // screen touch
    lv_obj_add_flag(g_pt_lv_100ask_xz_ai->img_emoji, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->img_emoji, screen_onclicked_event_cb, LV_EVENT_CLICKED, NULL);

    /* setting wifi */
    lv_obj_update_layout(cont_state_bar);
    g_pt_lv_100ask_xz_ai->btn_set_wifi = lv_button_create(lv_screen_active());
    lv_obj_set_size(g_pt_lv_100ask_xz_ai->btn_set_wifi, lv_obj_get_width(cont_state_bar), lv_obj_get_height(cont_state_bar));
    lv_obj_set_align(g_pt_lv_100ask_xz_ai->btn_set_wifi, LV_ALIGN_TOP_MID);
    lv_obj_set_style_opa(g_pt_lv_100ask_xz_ai->btn_set_wifi, LV_OPA_TRANSP, 0);

    lv_obj_t * cont_wifi_page = lv_100ask_wifi_page_init(lv_screen_active());
    lv_obj_add_event_cb(g_pt_lv_100ask_xz_ai->btn_set_wifi, btn_set_wifi_event_cb, LV_EVENT_CLICKED, cont_wifi_page);

    SetEmotion("neutral");

    /* 初始化UI交互系统 */
    ui_system_init();

}

void lvgl_lock(void)
{
    pthread_mutex_lock(&lvgl_mutex);
}

void lvgl_unlock(void)
{
    pthread_mutex_unlock(&lvgl_mutex);
}

void SetWifi(int enabled)
{
    // https://github.com/78/xiaozhi-fonts/blob/main/src/font_awesome.c
    lvgl_lock();   
    if (enabled)
    {        
        //lv_image_set_src(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, "\xef\x87\xab");  // wifi full
        lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, "\xef\x87\xab");  // wifi full
    }
    else
    {
        //lv_image_set_src(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, "\xef\x9a\xac");  // wifi_slash      
        lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_img_wifi, "\xef\x9a\xac");  // wifi_slash      
    }
    lvgl_unlock();
}

void SetStateString(char *str)
{
    lvgl_lock();
    lv_label_set_text(g_pt_lv_100ask_xz_ai->state_bar_label_state, str);
    lvgl_unlock();
}

void SetText(char *str)
{
    lvgl_lock();   
    lv_label_set_text(g_pt_lv_100ask_xz_ai->label_chat, str);
    lvgl_unlock();
}

void SetEmotion(char *name)
{
    lvgl_lock();
    lv_label_set_text(g_pt_lv_100ask_xz_ai->img_emoji, font_awesome_get_utf8(name));
    lvgl_unlock();
}


void OnClicked(void)
{
    static uint16_t index = 0;

    static char *str[][4] = {
        {"待命", "现在是待命状态哦。", "neutral", "standby"},
        {"聆听", "现在是聆听状态哦。", "loving", "listening"},
        //{"回答", "现在是回答状态哦。", "winking", "speaking"},
    };    

    SetStateString(str[index][0]);
    SetText(str[index][1]);
    SetEmotion(str[index][2]);
    SendState(str[index][3]);

    if (index == 0)
        g_soft_stanby = 1;
    else
        g_soft_stanby = 0;

    if(index >= 1) index = 0;
    else index++;

    LV_LOG_USER("Clicked, index: %d", index);
}


/**********************
 *   STATIC FUNCTIONS
 ***********************/
 
/**
 * @brief 从/tmp/wifi.txt文件中提取WiFi SSID
 * @param ap_names 用于存储SSID的二维字符数组
 * @param max_count ap_names数组的最大行数
 * @return 实际提取到的SSID数量
 * 
 * /tmp/wifi_scan_result.txt文件格式示例:
 * bssid / frequency / signal level / encode / ssid
 * 74:39:89:f8:f0:ae       5240    -59     0803    Programmers7
 * 76:39:89:fe:f0:ae       5240    -62     0803
 * 48:8a:d2:d1:62:a2       2412    -73     8000    MERCURY_62A2
 * f0:92:b4:a6:03:91       2422    -75     0803    ChinaNet-kRAH
 * 74:39:89:f8:f0:ad       2462    -85     0803    Programmers
 * 76:39:89:fe:f0:ad       2462    -85     0803
 * d2:ad:08:f8:eb:71       2462    -88     0803    DIRECT-71-HP Smart Tank 750
 * 7c:c8:4a:e9:39:f8       5180    -92     0803    yankemei168_5G
 * 7c:c8:4a:e9:39:fb       5180    -92     0805
 * c8:50:e9:bb:f5:0a       2422    -93     0803    ChinaNet-sqJr
 */
static int get_wifi_ssids(char ap_names[][32], int max_count)
{
    FILE *fp;
    char line[256];
    int count = 0;
    
    // 打开/tmp/wifi_scan_result.txt文件
    fp = fopen("/tmp/wifi_scan_result.txt", "r");
    if (fp == NULL) {
        // 文件不存在或无法打开，返回0
        return 0;
    }
    
    // 跳过第一行标题
    if (fgets(line, sizeof(line), fp) == NULL) {
        fclose(fp);
        return 0;
    }
    
    // 逐行读取文件内容
    while (fgets(line, sizeof(line), fp) != NULL && count < max_count) {
        char *token;
        int field_count = 0;
        char *saveptr = NULL;
        
        // 去除行尾的换行符
        char *newline = strchr(line, '\n');
        if (newline) {
            *newline = '\0';
        }
        
        // 使用空格分割字段
        token = strtok_r(line, " \t", &saveptr);
        while (token != NULL && field_count < 5) {
            // 第5个字段是SSID（索引为4）
            if (field_count == 4) {
                // 复制SSID到ap_names数组中
                strncpy(ap_names[count], token, 31);
                ap_names[count][31] = '\0'; // 确保字符串结束
                count++;
                break;
            }
            token = strtok_r(NULL, " \t", &saveptr);
            field_count++;
        }
    }
    
    fclose(fp);
    return count;
}

static lv_obj_t * lv_100ask_wifi_page_init(lv_obj_t *parent)
{
    lv_obj_t * cont_state_bar = lv_obj_get_child(parent, 0);

    lv_obj_t * cont = lv_obj_create(parent);
    lv_obj_remove_style_all(cont);
    lv_obj_set_size(cont, LV_PCT(100), LV_VER_RES - lv_obj_get_height(cont_state_bar));
    lv_obj_set_style_bg_opa(cont, LV_OPA_100, 0);
    lv_obj_set_style_bg_color(cont, lv_color_white(), 0);
    lv_obj_set_style_pad_all(cont, 0, 0);
    lv_obj_set_align(cont, LV_ALIGN_BOTTOM_MID);
    lv_obj_add_flag(cont, LV_OBJ_FLAG_HIDDEN);
    lv_obj_remove_flag(cont, LV_OBJ_FLAG_SCROLLABLE);

    return cont;
}

static void btn_set_wifi_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn_set_wifi = lv_event_get_target(e);
    lv_obj_t * wifi_page = lv_event_get_user_data(e);

    if(code == LV_EVENT_CLICKED) {
        if(lv_obj_has_flag(wifi_page, LV_OBJ_FLAG_HIDDEN))
        {
            lv_obj_remove_flag(wifi_page, LV_OBJ_FLAG_HIDDEN);
            
            lv_obj_t* list = lv_list_create(wifi_page);
            lv_obj_set_style_pad_all(list, 0, 0);
            lv_obj_set_style_text_font(list, get_text_font(), 0);
            lv_obj_set_style_radius(list, 0, 0);
            lv_obj_set_style_border_width(list, 0, 0);
            lv_obj_set_size(list, LV_PCT(100), LV_PCT(100));
            lv_obj_t * btn;

            // 执行命令获取WiFi列表
            LV_LOG_USER("to scan wifi ap!");
            SetStateString("扫描热点...");  
            lv_refr_now(NULL); // 立即刷新显示 
            system("sh /data/get_wifi.sh > /tmp/wifi_scan_result.txt");
            SetStateString("请选择热点...");   
              
            // wifi 列表
            char ap_names[100][32];
            int ap_count = get_wifi_ssids(ap_names, 100);
            
            for(int index = 0; index < ap_count; index++)
            {
                //btn = lv_list_add_button(list, LV_SYMBOL_WIFI, ap_names[index]);
                btn = lv_list_add_button(list, NULL, ap_names[index]);
                lv_obj_add_event_cb(btn, wifi_list_btn_event_cb, LV_EVENT_CLICKED, btn_set_wifi);
            }
            
        }
        else
        {
            LV_LOG_USER("exit wifi page!");
            SetStateString("待命"); 
            lv_obj_clean(wifi_page);
            lv_obj_add_flag(wifi_page, LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static char *g_wifi_ssid;
static void wifi_list_btn_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_target(e);

    if(code == LV_EVENT_CLICKED) {
        lv_obj_t *label = lv_obj_get_child(btn, 0);  // 索引 0 表示第一个子控件
        if (label)
            g_wifi_ssid = lv_label_get_text(label);
        SetStateString("请输入热点密码");   
        lv_obj_t * btn_set_wifi = lv_event_get_user_data(e);

        lv_obj_t * wifi_page = lv_obj_get_parent(btn);
        wifi_page = lv_obj_get_parent(wifi_page);

        lv_obj_t * cont = lv_obj_create(wifi_page);
        lv_obj_remove_style_all(cont);
        lv_obj_set_size(cont, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_opa(cont, LV_OPA_100, 0);
        lv_obj_set_style_bg_color(cont, lv_color_white(), 0);

        lv_obj_t * pwd_ta = lv_textarea_create(cont);
        lv_textarea_set_text(pwd_ta, "");
        lv_textarea_set_password_mode(pwd_ta, true);
        lv_textarea_set_one_line(pwd_ta, true);
        lv_obj_set_width(pwd_ta, lv_pct(100));
        lv_obj_set_pos(pwd_ta, 0, 20);

        /*Create a label and position it above the text box*/
        lv_obj_t * pwd_label = lv_label_create(cont);
        lv_label_set_text(pwd_label, "Password:");
        lv_obj_align_to(pwd_label, pwd_ta, LV_ALIGN_OUT_TOP_LEFT, 0, 0);

        /*Create a keyboard*/
        lv_obj_t * kb = lv_keyboard_create(cont);
        lv_obj_set_size(kb,  LV_HOR_RES, LV_VER_RES / 2);
        //lv_obj_set_size(kb,  240, 320 / 2);

        lv_keyboard_set_textarea(kb, pwd_ta); /*Focus it on one of the text areas to start*/

        lv_obj_set_user_data(pwd_ta, btn_set_wifi);
        lv_obj_add_event_cb(pwd_ta, ta_event_cb, LV_EVENT_ALL, kb);

        //lv_obj_t * btn_set_wifi = lv_event_get_user_data(e);
        //lv_obj_send_event(btn_set_wifi, LV_EVENT_CLICKED, NULL);
    }
}

static void ta_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * ta = lv_event_get_target(e);
    lv_obj_t * kb = lv_event_get_user_data(e);
    
    if(code == LV_EVENT_CLICKED || code == LV_EVENT_FOCUSED) {
        /*Focus on the clicked text area*/
        if(kb != NULL) lv_keyboard_set_textarea(kb, ta);
    }

    else if(code == LV_EVENT_READY) {
        lv_obj_t * btn_set_wifi = lv_obj_get_user_data(ta);

        // 密码内容，用于连接wifi
        const char *password = lv_textarea_get_text(ta);
        LV_LOG_USER("Ready, WIFI SSID: %s, password: %s", g_wifi_ssid, password);

        // 保存WiFi配置到文件
        save_wifi_config(g_wifi_ssid, password);

        // 可阻塞超时等待，如果连接成功，关闭wifi配置页面，失败则跳过继续输入密码
        SetStateString("连接热点中..."); 
        lv_obj_send_event(btn_set_wifi, LV_EVENT_CLICKED, NULL);
    }

    // 点击最左下角的按键，关闭wifi配置页面
    else if(code == LV_EVENT_CANCEL) {
        SetStateString("请选择热点...");   
        lv_obj_t * cont = lv_obj_get_parent(ta);
        lv_obj_delete(cont);
    }
    
}

static const char* font_awesome_get_utf8(const char* name) {
    if (!name) return NULL;
    
    for (size_t i = 0; i < font_awesome_symbol_count; i++) {
        if (strcmp(font_awesome_symbols[i].name, name) == 0) {
            return font_awesome_symbols[i].utf8_string;
        }
    }
    return NULL;
}

/**
 * @brief 将WiFi SSID和密码保存到配置文件
 * @param ssid WiFi名称
 * @param password WiFi密码
 * @return 0表示成功，其他值表示失败
 */
static int save_wifi_config(const char *ssid, const char *password)
{
    FILE *fp;
 
    // 打开配置文件
    fp = fopen("/data/wifi.cfg", "w");
    if (fp == NULL) {
        LV_LOG_ERROR("Failed to open /data/wifi.cfg for writing");
        return -1;
    }
    
    // 写入WiFi配置
    fprintf(fp, "SSID=\"%s\"\n", ssid ? ssid : "");
    fprintf(fp, "PASSWORD=\"%s\"\n", password ? password : "");
    
    // 关闭文件
    fclose(fp);
    
    LV_LOG_USER("WiFi config saved: SSID=%s", ssid ? ssid : "");
    return 0;
}

static void lv_100ask_xz_ai_main_deinit(void)
{
    lv_free(g_pt_lv_100ask_xz_ai);
    unload_text_font();

    lv_deinit();
}


static void init_style(void)
{
    /*Create style with the new font*/;
    lv_style_init(&g_style_chat_font);
    lv_style_set_text_font(&g_style_chat_font, get_text_font());
    lv_style_set_text_align(&g_style_chat_font, LV_TEXT_ALIGN_CENTER);

    lv_style_init(&g_style_state_font);
    lv_style_set_text_font(&g_style_state_font, get_text_font());
    lv_style_set_text_align(&g_style_state_font, LV_TEXT_ALIGN_CENTER);
}

static int load_text_font(void)
{
    char font_path[128];

    if (g_text_font) {
        return 0;
    }

    snprintf(font_path, sizeof(font_path), "%c:%s",
             (char)LV_FS_POSIX_LETTER, XIAOZHI_TEXT_FONT_FILE);
    g_text_font = lv_binfont_create(font_path);
    if (!g_text_font) {
        LV_LOG_ERROR("Failed to load font: %s", font_path);
        return -1;
    }

    LV_LOG_USER("Loaded UI font: %s", font_path);
    return 0;
}

static void unload_text_font(void)
{
    if (!g_text_font) {
        return;
    }

    lv_binfont_destroy(g_text_font);
    g_text_font = NULL;
}

static const lv_font_t *get_text_font(void)
{
    return g_text_font ? g_text_font : LV_FONT_DEFAULT;
}

static void screen_onclicked_event_cb(lv_event_t * e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if(code == LV_EVENT_CLICKED) {
        OnClicked();
    }
}
