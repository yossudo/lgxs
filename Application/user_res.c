/**
 * ユーザリソースの生成
 *
 * ユーザ定義のT-Kernelリソースを生成
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include <tk/tkernel.h>
#include "user_common.h"
#include "user_res.h"
#include "tapp.h"
#include "tai.h"
#include "timu.h"
#include "tnet.h"
#include "tled.h"

// TCB
typedef struct {
    ID tskid;
    PRI priority;
    SZ stksz;
    FP entry;
} task_def_t;


/**
 * タスクの生成
 *
 * タスクを生成する
 * @return 処理結果
 * @retval E_OK 成功
 * @retval !E_OK エラー(APIのエラー値)
 */
EXPORT ER create_tasks(void) {
    ER ercd;
    T_CTSK ctsk;

    const task_def_t task_table[] = {
        { TSKID_TAPP, TPRI_TAPP, STKSZ_TAPP, task_tapp },
        { TSKID_TAI,  TPRI_TAI,  STKSZ_TAI,  task_tai  },
        { TSKID_TIMU, TPRI_TIMU, STKSZ_TIMU, task_timu },
        { TSKID_TNET, TPRI_TNET, STKSZ_TNET, task_tnet },
        { TSKID_TLED, TPRI_TLED, STKSZ_TLED, task_tled },
    };

    for (int i = 0; i < DIM(task_table); i++) {
        ctsk.tskatr = TA_HLNG | TA_RNG0;
        ctsk.exinf  = NULL;
        ctsk.task   = task_table[i].entry;
        ctsk.itskpri = task_table[i].priority;
        ctsk.stksz   = task_table[i].stksz;

        ercd = tk_cre_tsk(&ctsk);
        if (ercd < E_OK) {
            APP_ERR_PRINT("Task creation failed: %d (ID=%d)\n", ercd, task_table[i].tskid);
            return ercd;
        }
        tk_sta_tsk(ercd, 0);
        APP_PRINT("Task started (ID=%d)\n", task_table[i].tskid);

    }

    return E_OK;
}


/**
 * メールボックスの生成
 *
 * メールボックスを生成する
 * @return 処理結果
 * @retval E_OK 成功
 * @retval !E_OK エラー(APIのエラー値)
 */
EXPORT ER create_mailboxes(void) {
    ER ercd;

    const T_CMBX mbx_table[] = {
        { .mbxatr = TA_TFIFO }, // MBXID_TAPP
        { .mbxatr = TA_TFIFO }, // MBXID_TIMU
        { .mbxatr = TA_TFIFO }, // MBXID_TAI
        { .mbxatr = TA_TFIFO },  // MBXID_TNET
        { .mbxatr = TA_TFIFO },  // MBXID_TLED
    };

    for (int i = 0; i < DIM(mbx_table); i++) {
        ercd = tk_cre_mbx(&mbx_table[i]);
        if (ercd < E_OK) {
            return ercd;
        }
    }

    return E_OK;
}


/**
 * 固定長メモリプールの生成
 *
 * 固定長メモリプールを生成する
 * @return 処理結果
 * @retval E_OK 成功
 * @retval !E_OK エラー(APIのエラー値)
 */
EXPORT ER create_mem_pools(void) {
    ER ercd;

    const T_CMPF mpf_table[] = {
        { .mpfatr = TA_TFIFO, .mpfcnt = MPFNUM_LARGE, .blfsz = MPFSZ_LARGE, .bufptr = NULL }, // MPFID_LARGE (ID=1)
        { .mpfatr = TA_TFIFO, .mpfcnt = MPFNUM_SMALL, .blfsz = MPFSZ_SMALL, .bufptr = NULL }, // MPFID_SMALL (ID=2)
    };

    for (int i = 0; i < DIM(mpf_table); i++) {
        ercd = tk_cre_mpf(&mpf_table[i]); // MPFID_LARGE (ID=1)
        if (ercd < E_OK) {
            return ercd;
        }
    }

    return E_OK;
}
