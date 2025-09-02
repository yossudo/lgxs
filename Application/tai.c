/**
 * TAIタスク
 *
 * 加速度データから信号処理、AI危険度判定を行い、結果をTAPPへ送信する
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#include "tai.h"

#define FFT_SIZE 1024
#define SAMPLE_FREQ_HZ 100.0f
#define THRESH_FREQ_HZ 20.0f    // 例：20Hz以上を「異常」とみなす簡易ロジック

static float input_f32[FFT_SIZE];
static float output_mag[FFT_SIZE / 2];


// 関数プロトタイプ
LOCAL void init_task_tai(void);
static void run_fft_and_infer(const UH *accz_raw);
LOCAL ER send_ai_res(INT result);



/**
 * タスク初期化
 *
 * タスクの初期化
 * @param なし
 * @return なし
 */
LOCAL void init_task_tai(void)
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

        run_fft_and_infer(mar->accz);

        er = tk_rel_mpf(pum->mpfid, pum);
        if (er != E_OK) {
            APP_ERR_PRINT("error rel_mpf:%d\n", er);
            continue;
        }

        //send_ai_res(TRUE);

    }

}

static void run_fft_and_infer(const UH *accz_raw)
{
    arm_rfft_fast_instance_f32 fft_inst;
    arm_status status;

    // 1. CMSIS-DSPのRFFT初期化
    status = arm_rfft_fast_init_f32(&fft_inst, FFT_SIZE);
    if (status != ARM_MATH_SUCCESS) {
        APP_ERR_PRINT("FFT init failed\n");
        return;
    }

    // 2. uint16_t → float変換（DCシフト除去のためmeanを引く）
    float mean = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) {
        input_f32[i] = (float)accz_raw[i];
        mean += input_f32[i];
    }
    mean /= FFT_SIZE;
    for (int i = 0; i < FFT_SIZE; i++) {
        input_f32[i] -= mean;
    }

    // 3. FFT実行
    arm_rfft_fast_f32(&fft_inst, input_f32, input_f32, 0);

    // 4. 振幅スペクトル（複素数→パワー）
    arm_cmplx_mag_f32(input_f32, output_mag, FFT_SIZE / 2);

    // 5. 最大成分の周波数を探索
    float max_val = 0.0f;
    uint32_t max_index = 0;
    arm_max_f32(output_mag, FFT_SIZE / 2, &max_val, &max_index);

    float max_freq = (float)max_index * (SAMPLE_FREQ_HZ / FFT_SIZE);
    APP_PRINT("Dominant Freq = %.2f Hz\n", max_freq);

    // 6. 簡易異常判定ロジック（後でAI推論に置換）
    if (max_freq > THRESH_FREQ_HZ) {
        send_ai_res(TRUE);  // 異常あり
    } else {
        send_ai_res(FALSE); // 異常なし
    }
}


/**
 * MSGID_TAI_RESを送信
 *
 * AI判定応答データをtappへ送信
 * @param[in] result 結果
 * @return 処理結果
 * @retval E_OK 成功
　* @retval !E_OK エラー(APIのエラー値)
 */
LOCAL ER send_ai_res(INT result)
{
    ER er;
    user_msg_t *pum = NULL;

    // 固定長メモリを取得
    er = tk_get_mpf(MPFID_SMALL, (void **)&pum, TMO_FEVR);
    if (er != E_OK) {
        APP_ERR_PRINT("error get_mpf:%d", er);
       return er;
    }

    // メッセージ構造体を作成
    memset(pum, 0x00, sizeof(user_msg_t));
    pum->msgid  = MSGID_TAI_RES;
    pum->srctsk = TSKID_TAI;
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
