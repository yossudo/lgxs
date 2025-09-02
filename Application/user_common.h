/**
 * ユーザ共通ヘッダ
 *
 * アプリケーション全体で使用できるマクロ
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#ifndef _USER_COMMON_H_
#define _USER_COMMON_H_

#include <tk/tkernel.h>
#include "common_utils.h"
#include "user_res.h"

#define DIM(x)  (sizeof(x) / sizeof(x[0]))
#define SYSTIM_TO_UD(systim)  ( ((UD)((systim).hi) << 32) | (UD)((systim).lo) )

#define millis32() ({ \
    SYSTIM __now; \
    tk_get_tim(&__now); \
    (UW)__now.lo; \
})

#endif
