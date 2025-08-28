/*
 * tai.c
 *
 *  Created on: 2025/07/05
 *      Author: yoshi
 */
#include "tai.h"

LOCAL void init_task_tai(void);

LOCAL ER send_ai_res(INT result)
{
    ER er;
    user_msg_t *pum = NULL;

    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TAI_RES;
    pum->srctsk = TSKID_TAI;
    pum->dsttsk = TSKID_TAPP;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_SMALL;

    er = tk_snd_mbx( MBXID_TAPP, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


LOCAL void init_task_tai(void)
{
    return;
}




// TAIタスクメイン
EXPORT void task_tai(INT stacd, void *exinf) {

    APP_PRINT("[TAI started]\n");

    // タスクの初期化
    init_task_tai();

    ER er;
    user_msg_t *pum = NULL;
    msg_ai_req_t *mar = NULL;

    while(1) {
        er = tk_rcv_mbx( MBXID_TAI, (T_MSG **)&pum, TMO_FEVR );
        if (er != E_OK) {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }
        mar = (msg_ai_req_t *)&pum->pyload;

        APP_PRINT( "rcv_mbx TAI:[%d]\n", pum->result );

        for (int i =0; i < 16; i++) {
            APP_PRINT("%d ", mar->accz[i]);
        }
        APP_PRINT("\n");
        er = tk_rel_mpf(pum->mpfid, pum);
        if (er != E_OK) {
            APP_ERR_PRINT("error rel_mpf:%d\n", er);
            continue;
        }

        send_ai_res(TRUE);

    }

}
