/**
 * TNETタスク
 *
 * AI危険度判定結果をネットワークへ送信
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "tnet.h"

// 関数プロトタイプ
LOCAL void init_task_tnet(void);
EXPORT void task_tnet(INT stacd, void *exinf);
LOCAL ER send_net_res(INT result);


/**
 * タスク初期化
 *
 * タスクの初期化
 * @param なし
 * @return なし
 */
LOCAL void init_task_tnet(void)
{
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
EXPORT void task_tnet(INT stacd, void *exinf)
{


    APP_PRINT("[TNET started]\n");

    // タスク初期化
    init_task_tnet();

    // メインループ
    ER er;
    user_msg_t *pum = NULL;
    msg_net_req_t *mnr = NULL;

    // グルグル．．．
    while(1) {

        // メッセージ受信待ち
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


/**
 * MSGID_TNET_RESを送信
 *
 * ネットワーク送信応答データをtappへ送信
 * @param[in] result 結果
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_net_res(INT result)
{
    ER er;
    user_msg_t *pum = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ送信構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TNET_RES;
    pum->srctsk = TSKID_TNET;
    pum->dsttsk = TSKID_TAPP;
    pum->result = (UH)result;
    pum->mpfid  = MPFID_SMALL;

    // メッセージ送信
    er = tk_snd_mbx( MBXID_TAPP, (T_MSG *)pum );
    if (er != E_OK) {
        APP_ERR_PRINT("error snd_mbx:%d\n", er);
        return er;
    }

    return E_OK;
}
