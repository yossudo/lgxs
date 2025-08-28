/*
 * tapp.c
 *
 *  Created on: 2025/07/05
 *      Author: yoshi
 */
#include "tapp.h"



// 関数プロトタイプ
LOCAL ER send_ai_req(INT result, msg_ai_req_t *data);
LOCAL ER send_net_req(INT result, msg_net_req_t *data);
LOCAL void init_task_tapp(void);
EXPORT void task_tapp(INT stacd, void *exinf);


LOCAL ER send_ai_req(INT result, msg_ai_req_t *data)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_ai_req_t *mar = NULL;

    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TAI_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TAI;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    mar = (msg_ai_req_t *)&pum->pyload;
    memcpy(mar, data, sizeof(msg_ai_req_t));

    er = tk_snd_mbx( MBXID_TAI, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


LOCAL ER send_net_req(INT result, msg_net_req_t *data)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_net_req_t *mnr = NULL;

    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TAI_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TNET;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    mnr = (msg_net_req_t *)&pum->pyload;
    memcpy(mnr, data, sizeof(msg_net_req_t));

    er = tk_snd_mbx( MBXID_TNET, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


// タスク初期化
LOCAL void init_task_tapp(void)
{
    return;
}


// タスクメイン
EXPORT void task_tapp(INT stacd, void *exinf) {

    APP_PRINT("[TAPP started]\n");

    // タスク初期化
    init_task_tapp();

    // メインループ
    ER er;
    user_msg_t *pum = NULL;
    msg_imu_req_t *mir = NULL;
    msg_ai_req_t mar;
    msg_net_req_t mnr;

    while(1) {
        er = tk_rcv_mbx( MBXID_TAPP, (T_MSG **)&pum, TMO_FEVR );
        if (er != E_OK && er != E_TMOUT) {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }

        if (pum->msgid == MSGID_TIMU_REQ) {
            mir = (msg_imu_req_t *)&pum->pyload;

            APP_PRINT( "rcv_mbx TAPP:[%d]\n", pum->result );

            APP_PRINT("%llu - ", SYSTIM_TO_UD(mir->tim));
            for (int i =0; i < 16; i++) {
                APP_PRINT("%d ", mir->accz[i]);
            }
            APP_PRINT("\n");
        }
        else if (pum->msgid == MSGID_TAI_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
        }
        else if (pum->msgid == MSGID_TNET_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
        }

        if (er == E_OK) {
            er = tk_rel_mpf(pum->mpfid, pum);
            if (er != E_OK) {
                APP_ERR_PRINT("error rel_mpf:%d\n", er);
                continue;
            }
        }

#if 0
        for (int i = 0; i < 16; i++) {
            mar.accz[i] = (UH)i * 3;
        }
        send_ai_req(2, &mar);

        for (int i = 0; i < 16; i++) {
            mnr.accz[i] = (UH)i * 4;
        }
        send_net_req(3, &mnr);
#endif

    }

}
