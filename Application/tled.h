/*
 * LED制御タスク(EK-RA8D1)
 *
 * @file
 *
 * @note FSPのGPIOピン定義（USER_LED3_RED / USER_LED3_BLUE / USER_LED3_GREEN）を利用
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */

#ifndef _TLED_H_
#define _TLED_H_

#include "user_common.h"

#ifdef __cplusplus
extern "C" {
#endif

/* LED識別子 */
typedef enum {
    TLED_BLUE = 0,
    TLED_GREEN,
    TLED_RED,
    TLED_NUM
} tled_id_t;

/* 制御パターン */
typedef enum {
    TLED_PAT_OFF = 0,     /* 消灯 */
    TLED_PAT_ON,          /* 点灯 */
    TLED_PAT_BLINK_FAST,  /* 速点滅 (100ms ON / 100ms OFF) */
    TLED_PAT_BLINK_SLOW   /* 遅点滅 (500ms ON / 500ms OFF) */
} tled_pattern_t;

/* 点滅回数の特殊値: 無限回数 */
#define TLED_BLINK_INFINITE   (-1)

/* タスクエントリ */
IMPORT void task_tled(INT stacd, void *exinf);

#ifdef __cplusplus
}
#endif
#endif /* TLED_H_ */
