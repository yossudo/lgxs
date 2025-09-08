/**
 * アプリケーションレベルスタートアップ
 *
 * アプリケーションレベルのスタートアップ
 *
 * @file
 *
 * @note T-Kernelから呼ばれるアプリケーションレベルのスタートアップコード
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "user_common.h"

/**
 * アプリケーションレベルスタートアップ
 *
 * アプリケーションレベルのスタートアップ
 * @param なし
 * @return なし(0固定)
 */
EXPORT INT usermain(void)
{
    APP_PRINT("[app_main] usermain started\n");

    // T-Kernelオブジェクト生成
    create_tasks();         // タスク
    create_mailboxes();     // メールボックス
    create_mem_pools();     // メモリプール

    // 眠れ、永遠に、、、
    tk_slp_tsk(TMO_FEVR);

    return 0;
}
