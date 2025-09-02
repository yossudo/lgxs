/**
 * TNETタスクヘッダ
 *
 * AI危険度判定結果をネットワークへ送信
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */
#ifndef TNET_H_
#define TNET_H_

#include "user_common.h"

// タスクメイン関数
IMPORT void task_tnet(INT stacd, void *exinf);

#endif /* TNET_H_ */
