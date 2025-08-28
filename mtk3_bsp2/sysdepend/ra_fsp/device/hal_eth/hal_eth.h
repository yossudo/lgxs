#ifndef _DEV_HAL_ETH_H_
#define _DEV_HAL_ETH_H_

/*----------------------------------------------------------------------
 * Ether Device
 */
#define DEV_HAL_ETH    0

// 属性アクセス用ID（必要に応じて）
#define TDN_HAL_ETH_MACADDR   (-100)

IMPORT ER dev_init_hal_eth( UW unit, ether_instance_ctrl_t *heth, const ether_cfg_t *ceth);

#endif /* _DEV_HAL_ETH_H_ */
