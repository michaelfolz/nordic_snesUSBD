#ifndef _I2C_SPI_H_
#define _I2C_SPI_H_

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"
#include "nrf_drv_twi.h"

#include "bsp.h"
#include "app_util_platform.h"
#include "app_timer.h"
#include "nrf_drv_clock.h"
// gpio drivers
#include "nrf_drv_gpiote.h"
#include "nrf_gpiote.h"
#include "nrf_gpio.h"


ret_code_t twi_init(void);

ret_code_t twi_tx(uint8_t controller, uint8_t addr, uint8_t *p_data, uint8_t length);
ret_code_t twi_rx(uint8_t controller, uint8_t addr, uint8_t *p_data, uint8_t length);

#endif /* _I2C_SPI_H_*/
