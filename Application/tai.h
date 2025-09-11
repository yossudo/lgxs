/**
 * TAIタスクヘッダ
 *
 * 加速度データから信号処理、AI危険度判定を行う
 *
 * @file
 *
 * @date 2025/7/5
 * @author: Things Base y.sudo
 */

#ifndef TAI_H_
#define TAI_H_
#include <math.h>

#include "arm_math.h"
//#include "arm_const_structs.h"
#include "arm_nnfunctions.h" // CMSIS-NN（将来モデル用）
#include "user_common.h"

// タスクメイン関数
IMPORT void task_tai(INT stacd, void *exinf);

#endif /* TAI_H_ */
