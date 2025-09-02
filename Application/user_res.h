/**
 * ユーザリソースヘッダ
 *
 * ユーザ定義のT-Kernelリソース定義（タスクID・メールボックスID・メモリプールID・メッセージ構造体 等）
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#ifndef USER_RES_H_
#define USER_RES_H_

/*------------------------------------------
 * タスクID（静的割当）
 *------------------------------------------*/
typedef enum {
    TSKID_USRMAIN = 1,      // 初期タスク
    TSKID_TAPP,             // アプリケーションメインタスク
    TSKID_TAI,              // AI推論タスク
    TSKID_TIMU,             // 慣性センサ(MPU9250)中間タスク
    TSKID_TNET,             // ネットワーク通信中間タスク
    // 追加はここに
    TSKID_NUM               // タスク数（末尾）
} task_id_t;


/*------------------------------------------
 * タスク優先度（低いほど高優先度）
 *------------------------------------------*/
typedef enum {
    TPRI_TIMU  = 5,         // IMUデータ取得（正確に100Hzサンプリングさせたいため最高優先度で動作）
    TPRI_TNET  = 8,         // ネットワーク通信（UDP通知を即時処理）
    TPRI_TAI   = 10,        // AI推論（FFT・AI推論を含む重処理）
    TPRI_TAPP  = 12         // アプリケーション制御（全体統括）
} task_pri_t;


/*------------------------------------------
 * タスクスタックサイズ定義（単位：Byte）
 *------------------------------------------*/
#define STKSZ_TAPP      2048
#define STKSZ_TAI       4096
#define STKSZ_TIMU      1024
#define STKSZ_TNET      1024

IMPORT ER create_tasks(void);

/*------------------------------------------
 * メールボックスID（静的割当）
 *------------------------------------------*/
typedef enum {
    MBXID_TAPP = 1,         // TAPPの受信用メールボックス
    MBXID_TIMU,             // TIMUの受信用メールボックス
    MBXID_TAI,              // TAIの受信用メールボックス
    MBXID_TNET,             // TNETの受信用メールボックス
    // 追加はここに
    MBXID_NUM
} mbx_id_t;

IMPORT ER create_mailboxes(void);


/*------------------------------------------
 * 固定長メモリプールID（静的割当）
 *------------------------------------------*/
typedef enum {
    MPFID_LARGE = 1,     // 2080B用（加速度1024点など）
    MPFID_SMALL          // 32B用（結果や通知など）
    // 追加はここに
} mpf_id_t;

#define MPFNUM_LARGE  4     // 同時に4通の大きなデータを送れる想定
#define MPFSZ_LARGE  ((1024 * sizeof(UH)) + 64)
#define MPFNUM_SMALL  16    // 小メッセージは多数送信可能
#define MPFSZ_SMALL 32

IMPORT ER create_mem_pools(void);

/*------------------------------------------
 * メッセージ種別
 *------------------------------------------*/
typedef enum {
    MSGID_NONE = 0,
    MSGID_TIMU_IND,     // TIMU→TAPP：MPU9250データ塊送信通知
    MSGID_TAI_REQ,      // TAPP→TAI：推論要求
    MSGID_TAI_RES,      // TAI→TAPP：推論応答
    MSGID_TNET_REQ,     // TAPP→TNET：ネットワーク送信要求
    MSGID_TNET_RES,     // TNET→TAPP：ネットワーク送信応答
    // 追加はここに
} msg_id_t;


/*------------------------------------------
 * 共通メッセージ構造体
 *------------------------------------------*/
typedef struct {
    T_MSG hdr;       // T-Kernel ヘッダ（拡張用：現在は未使用）
    ID msgid;        // メッセージID（msg_id_t）
    ID srctsk;       // 送信元タスクID
    ID dsttsk;       // 送信先タスクID（未使用ならTNULL）
    INT result;      // 結果コード（オプション用途）
    ID mpfid;        // 使用メモリプールID
    UB pyload;       // ペイロードの先頭データ
} user_msg_t;

#define IMU_REC_MAX 1024

typedef struct {
    SYSTIM tim;
    UH accz[IMU_REC_MAX];
} msg_imu_ind_t;

typedef struct {
    SYSTIM tim;
    UH accz[IMU_REC_MAX];
} msg_ai_req_t;

typedef struct {
    SYSTIM tim;
    UH accz[IMU_REC_MAX];
} msg_net_req_t;


#define GPT_INTNO   GPT0_COUNTER_OVERFLOW_IRQn


#endif /* USER_RES_H_ */
