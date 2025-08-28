/*
 * tnet.c
 *
 *  Created on: 2025/07/05
 *      Author: yoshi
 */
#include "tnet.h"

// 関数プロトタイプ
LOCAL void init_task_tnet(void);

LOCAL ER send_net_res(INT result)
{
    ER er;
    user_msg_t *pum = NULL;

    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TNET_RES;
    pum->srctsk = TSKID_TNET;
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



// タスク初期化
LOCAL void init_task_tnet(void)
{
    return;
}


// タスクメイン
EXPORT void task_tnet(INT stacd, void *exinf) {

    APP_PRINT("[TNET started]\n");

    // タスク初期化
    init_task_tnet();

    // メインループ
    ER er;
    user_msg_t *pum = NULL;
    msg_net_req_t *mnr = NULL;

    while(1) {
        er = tk_rcv_mbx( MBXID_TNET, (T_MSG **)&pum, TMO_FEVR );
        if (er != E_OK) {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }
        mnr = (msg_net_req_t *)&pum->pyload;

        APP_PRINT( "rcv_mbx TNET:[%d]\n", pum->result );

        for (int i =0; i < 16; i++) {
            APP_PRINT("%d ", mnr->accz[i]);
        }
        APP_PRINT("\n");
        er = tk_rel_mpf(pum->mpfid, pum);
        if (er != E_OK) {
            APP_ERR_PRINT("error rel_mpf:%d\n", er);
            continue;
        }

        //send_net_res(9);

    }

    return;

}
