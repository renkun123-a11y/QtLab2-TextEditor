#ifndef APPINFO_H
#define APPINFO_H

// 程序的基本信息集中放在这里，改名称、版本或开发者信息时只改这一个文件。
#define APP_NAME            "小牛文本编辑器"
#define APP_VERSION         "V1.0"
#define APP_BUILD           "build 202611"
#define APP_VERSION_FULL    APP_VERSION " " APP_BUILD
#define APP_DEVELOPER       "张梓鸿"
#define APP_STUDENT_ID      "2024414300227"
#define APP_EMAIL           "3481179396@qq.com"

// 打开文件时的安全上限：超过这个大小的文件不当作文本载入。
#define APP_MAX_FILE_SIZE   (32 * 1024 * 1024)

#endif // APPINFO_H
