/**
 * Copyright (c) 2017 - 2019, Nordic Semiconductor ASA
 *
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form, except as embedded into a Nordic
 *    Semiconductor ASA integrated circuit in a product or a software update for
 *    such product, must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other
 *    materials provided with the distribution.
 *
 * 3. Neither the name of Nordic Semiconductor ASA nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * 4. This software, with or without modification, must only be used with a
 *    Nordic Semiconductor ASA integrated circuit.
 *
 * 5. Any software provided in binary form under this license must not be reverse
 *    engineered, decompiled, modified and/or disassembled.
 *
 * THIS SOFTWARE IS PROVIDED BY NORDIC SEMICONDUCTOR ASA "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY, NONINFRINGEMENT, AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL NORDIC SEMICONDUCTOR ASA OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
 * GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 */


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

#include "app_error.h"
#include "nrf.h"

#include "nintendo_i2c.h"
#include "i2c.h"


#include "hid_gamepad.h"
#include "sw_spi.h"

#define SNES_TIME_MS (5)

#define GREEN_LED_PIN    NRF_GPIO_PIN_MAP(0,9)  
#define RED_LED_PIN      NRF_GPIO_PIN_MAP(1,00)  
#define BLUE_LED_PIN     NRF_GPIO_PIN_MAP(0,10)  

#define CNT_2_INT_PIN    NRF_GPIO_PIN_MAP(0,2)  
#define CNT_1_INT_PIN    NRF_GPIO_PIN_MAP(0,17)  

#define CONTROLLER_STATE_PIN   NRF_GPIO_PIN_MAP(1,10)  

uint8_t controllerInterruptPins[2] = {CNT_1_INT_PIN, CNT_2_INT_PIN} ;


/**
 * @brief Timer to repeat mouse move
 */
APP_TIMER_DEF(m_mouse_move_timer);

static ret_code_t nrf_initalize(void)
{
    ret_code_t err_code;

    // setup the clock 
    err_code = nrf_drv_clock_init();
    APP_ERROR_CHECK(err_code);
    nrf_drv_clock_lfclk_request(NULL);
    while(!nrf_drv_clock_lfclk_is_running())
    {
        /* Just waiting */
    }

    // init app timer 
    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    // init dongle leds 
    bsp_board_init(BSP_INIT_LEDS);

    // gipote isnt working properly temp measure for now 
    nrf_gpio_cfg_input(CNT_1_INT_PIN,GPIO_PIN_CNF_PULL_Pulldown );
    nrf_gpio_cfg_input(CNT_2_INT_PIN,GPIO_PIN_CNF_PULL_Pulldown );

    // read keyboard pin state 
    nrf_gpio_cfg_input(CONTROLLER_STATE_PIN,GPIO_PIN_CNF_PULL_Pullup );

        //set output 
    nrf_gpio_cfg_output(GREEN_LED_PIN); 
    nrf_gpio_cfg_output(RED_LED_PIN);  
    nrf_gpio_cfg_output(BLUE_LED_PIN); 

    return err_code; 
}


static void snes_timer_handler(void * p_context)
{
    static uint8_t previousState[2] = {0,0} ;
    static uint8_t currentState[2] ={0,0} ; 
    uint8_t update = 0;
   
    uint8_t report[HID_REP_SIZE];
    sw_spi_send_char_msb('a');
    // approx 1.4ms -- needs more work 
 // process the incoming controller data
    for(int currController =0; currController < 2; currController++)
    {
        
        // read the controller detect pin
        currentState[currController] = nrf_gpio_pin_read(controllerInterruptPins[currController]);
        if((currentState[currController]) && (!previousState[currController]))
        {
           nintendo_init_controller(currController);
        }

        if(currentState[currController])
        {
            // pull controller data from i2s 
            nintendo_read_controller_data(currController);
            update = nintendo_compare_controller_data(currController);
            // if update exists send it out!
            if(update)
            {
                nintendo_get_hid_controller_data(currController, report);
                controller_sendpacket((currController), report, HID_REP_SIZE) ;
            }
                 
        }
        previousState[currController] = currentState[currController]; 
    }

    sw_spi_send_char_msb('b');
 
    return; 
}

/**
 * @brief Function for application main entry.
 */
int main(void)
{
    ret_code_t err_code;
    bool cnt_state = 0;

    /* Configure board. */
    err_code = nrf_initalize();
  
    sw_spi_send_initalize(20,22,24); 
   
    // read gpio set the pins 
    cnt_state = nrf_gpio_pin_read(CONTROLLER_STATE_PIN);
    nrf_gpio_pin_set(RED_LED_PIN);  
    if(cnt_state)
    {
        nrf_gpio_pin_set(GREEN_LED_PIN);
        nrf_gpio_pin_clear(BLUE_LED_PIN);
    }
    else 
    {
        nrf_gpio_pin_clear(GREEN_LED_PIN);
        nrf_gpio_pin_set(BLUE_LED_PIN);
    }


    init_controllers(cnt_state);
   

    /* Initializing TWI master interface for EEPROM */
    err_code =  twi_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_init();
    APP_ERROR_CHECK(err_code);

    err_code = app_timer_create(&m_mouse_move_timer, APP_TIMER_MODE_REPEATED, snes_timer_handler);
    APP_ERROR_CHECK(err_code);

    app_timer_start(m_mouse_move_timer, APP_TIMER_TICKS(SNES_TIME_MS), NULL);


    while (true)
    {
        // process usbd events
        while (app_usbd_event_queue_process())
        {
            /* Nothing to do */
        }

        /* Sleep CPU only if there was no interrupt since last loop processing */
        __WFE();
   }
}


