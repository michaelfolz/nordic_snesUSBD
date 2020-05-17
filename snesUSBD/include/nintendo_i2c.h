#ifndef _NINTENDO_I2C_INCLUDED
#define _NINTENDO_I2C_INCLUDED

#include <stdbool.h>
#include <stdint.h>
#include "nrf_delay.h"
#include "boards.h"
#include "app_util_platform.h"
#include "app_error.h"

/**
 *  Snes Controller structure
 *   both 8bitdo and Nintendo branded controllers output the same structure
 */
typedef union
{
    struct __attribute__((__packed__))
    { 
      // byte 0 
      uint8_t  bNull: 1;
      uint8_t  sR : 1;
      uint8_t  bSt : 1; // start  
      uint8_t  bNull2: 1;

      uint8_t  bSl : 1;  // select
      uint8_t  sL : 1;   // left sholder 
      uint8_t  dDown : 1;
      uint8_t  dRight : 1; // dpad
   
      // byte 1
      uint8_t  dUp : 1;
      uint8_t  dLeft : 1;
      uint8_t  bNull4 : 1; 
      uint8_t  bX : 1;
   
      uint8_t  bB : 1;
      uint8_t  bY : 1;
      uint8_t  bA : 1;
      uint8_t  bNull3: 1;
    };
    uint8_t bytes[2];     // allows for quick comparison 

} snes_controller;


#define SNES_OFFICIAL_CONTROLLER_DATA_LOCATION     (4)
#define SNES_8BITDO_CONTROLLER_DATA_LOCATION       (5)

#define NINTENDO_DEVICE_GAMEPAD_DATA_LENGTH 	     (7)
#define NINTENDO_DEVICE_ID_LENGTH 				         (4)
#define NINTENDO_NUMBER_OF_CONTROLLERS             (2)
#define NINTENDO_TWI_BUFFER			                   (10)

#define NINTENDO_TWI_DEV_ADDRESS                   (0x52)
#define NINTENDO_TWI_CNT_DATA_REG                  (0xfa)


void nintendo_init_controller(uint8_t controllerID);

void nintendo_read_controller_data(uint8_t controllerID);

uint8_t nintendo_compare_controller_data(uint8_t controllerID);

void nintendo_get_hid_controller_data(uint8_t controllerID,  uint8_t *data);

#endif /* _NINTENDO_I2C_INCLUDED*/

