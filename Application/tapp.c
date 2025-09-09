/**
 * TAPPタスク
 *
 * アプリケーションタスク
 *
 * @file
 *
 * @note 周辺タスクをコントロールし、アプリケーションを実現する
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "tapp.h"
#include "tled.h"


// 関数プロトタイプ
LOCAL void init_task_tapp(void);
EXPORT void task_tapp(INT stacd, void *exinf);
LOCAL ER send_ai_req(INT result, msg_ai_req_t *data);
LOCAL ER send_net_req(INT result, msg_net_req_t *data);
LOCAL ER send_led_req(INT result, UB led, UB pattern, W blink_count);


/**
 * タスク初期化
 *
 * タスクの初期化
 * @param なし
 * @return なし
 */
LOCAL void init_task_tapp(void)
{

    // 周辺タスクが上がるまでちょっと待つ
    tk_dly_tsk(100);

    send_led_req(TRUE, TLED_GREEN, TLED_PAT_ON, 0);

    return;
}


/**
 * タスクメイン
 *
 * タスクのメイン処理
 * @param[in] stacd タスク起動時の開始コード
 * @param[in] exinf タスク起動時の拡張情報
 * @return なし
 */
EXPORT void task_tapp(INT stacd, void *exinf) {

    APP_PRINT("[TAPP started]\n");

    // タスク初期化
    init_task_tapp();

    // グルグル．．．
    ER er;
    user_msg_t *pum = NULL;
    msg_imu_ind_t *mir = NULL;
    while( 1 ) {
        // メッセージ受信待ち
        er = tk_rcv_mbx( MBXID_TAPP, (T_MSG **)&pum, TMO_FEVR );
        if (er != E_OK && er != E_TMOUT) {
            APP_ERR_PRINT("error rcv_mbx:%d\n", er);
            continue;
        }

        if (pum->msgid == MSGID_TIMU_IND) {
            mir = (msg_imu_ind_t *)&pum->pyload;

            APP_PRINT( "rcv_mbx TAPP:[%d]\n", pum->result );

            APP_PRINT("%llu - ", SYSTIM_TO_UD(mir->tim));
            for (int i =0; i < 16; i++) {
                APP_PRINT("%d ", mir->accz[i]);
            }
            APP_PRINT("\n");

            send_ai_req(TRUE, (msg_ai_req_t *)mir);
            send_led_req(TRUE, TLED_BLUE, TLED_PAT_BLINK_FAST, 5);

        }
        else if (pum->msgid == MSGID_TAI_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
            send_net_req(pum->result, NULL);
        }
        else if (pum->msgid == MSGID_TNET_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
        }
        else if (pum->msgid == MSGID_TLED_RES) {
            APP_PRINT( "rcv_mbx TAPP:[%d][%d]\n", pum->msgid, pum->result );
        }

        if (er == E_OK) {
            er = tk_rel_mpf(pum->mpfid, pum);
            if (er != E_OK) {
                APP_ERR_PRINT("error rel_mpf:%d\n", er);
                continue;
            }
        }

    }

}

/**
 * MSGID_TAI_REQを送信
 *
 * AI要求をTAIへ送信
 * @param[in] result 結果
 * @param[in] data 送信データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
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


/**
 * MSGID_TNET_REQを送信
 *
 * ネットワーク送信要求をTNETへ送信
 * @param[in] result 結果
 * @param[in] data 送信データへのポインタ
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_net_req(INT result, msg_net_req_t *data)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_net_req_t *mnr = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_LARGE, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TNET_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TNET;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_LARGE;

    mnr = (msg_net_req_t *)&pum->pyload;
    memcpy(mnr, data, sizeof(msg_net_req_t));

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TNET, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}


/**
 * MSGID_TLED_REQを送信
 *
 * LED制御要求をTLEDへ送信
 * @param[in] led LED種
 * @param[in] pattern 点灯パターン
 * @param[in] blink_count 点滅回数
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_led_req(INT result, UB led, UB pattern, W blink_count)
{
    ER er;
    user_msg_t *pum = NULL;
    msg_led_req_t *mlr = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TLED_REQ;
    pum->srctsk = TSKID_TAPP;
    pum->dsttsk = TSKID_TLED;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_SMALL;

    mlr = (msg_led_req_t *)&pum->pyload;
    mlr->led = led;
    mlr->pattern = pattern;
    mlr->blink_count = blink_count;

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TLED, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}

