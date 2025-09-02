/**
 * TIMUタスクヘッダ
 *
 * AI危険度判定結果をネットワークへ送信
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#ifndef TIMU_H_
#define TIMU_H_

#include "user_common.h"

// タスクメイン関数
IMPORT void task_timu(INT stacd, void *exinf);

#endif /* TIMU_H_ */
