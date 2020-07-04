#include "hid_gamepad.h"



// Simple USB HID Class
//  All buttons including the D-PAD are mapped between button 1 & 16
//  The D pad will also manifest as a change in the X/Y axis - some emulators prefer the axis vs button
//                                                           - the button method is more reliable 
#define APP_USBD_HID_U2F_REPORT_DSC() {   \
  0x05, 0x01,                   /*USAGE_PAGE (Generic Desktop)*/\
  0x09, 0x05,                   /*USAGE (Game Pad)*/\
  0xa1, 0x01,                   /*COLLECTION (APPLICATION)*/\
  0xa1, 0x00,                    /*   COLLECTION (Physical)*/\
  0x05, 0x09,                   /*USAGE_PAGE (Button)*/\
  0x19, 0x01,                   /*USAGE_MINIMUM (Button1)*/\
  0x29, 0x10,                   /*USAGE_MAXIMUM (Button 16)*/\
  0x15, 0x00,                   /*LOGICAL_MINIMUM (0)*/\
  0x25, 0x01,                   /*LOGICAL_MAXIMUM(1)*/\
  0x95, 0x10,                   /*REPORT_COUNT (16)*/\
  0x75, 0x01,                   /*REPORT_SIZE (1)*/\
  0x81, 0x02,                   /*INPUT(Data, Var, Abs)*/\
  0x05, 0x01,                   /*USAGE_PAGE (Generic Desktop)*/\
  0x09, 0x30,                   /*USAGE (X)*/\
  0x09, 0x31,                   /*USAGE (Y)*/\
  0x15, 0x81,                   /*LOGICAL_MINIMUM(-127)*/\
  0x25, 0x7f,                   /*LOGICAL_MAXIMUM(127)*/\
  0x75, 0x08,                   /*REPORT_SIZE(8)*/\
  0x95, 0x02,                   /*REPORT_COUNT(4)*/\
  0x81, 0x02,                   /*INPUT(Data,Var,Abs)*/\
  0xc0,                     /*END_Collection*/\
  0xc0                      /*END_Collection*/\
}


static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event);


static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                    app_usbd_hid_user_event_t event);

APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(controller_one,APP_USBD_HID_U2F_REPORT_DSC());
APP_USBD_HID_GENERIC_SUBCLASS_REPORT_DESC(controller_two,APP_USBD_HID_U2F_REPORT_DSC());


static const app_usbd_hid_subclass_desc_t * reps[] = {&controller_one};
static const app_usbd_hid_subclass_desc_t * reps2[] = {&controller_two};

/**
 * @brief USB composite interfaces
 */
#define APP_USBD_INTERFACE_KBD   3



/**
 * @brief Global HID generic instance
 */
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic,
                                HID_GENERIC_INTERFACE,
                                hid_user_ev_handler,
                                ENDPOINT_LIST(),
                                reps,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_NONE,  // non booting sub class
                                APP_USBD_HID_PROTO_GENERIC); // generic hid protocol 



/**
 * @brief Global HID generic instance
 */
APP_USBD_HID_GENERIC_GLOBAL_DEF(m_app_hid_generic2,
                                HID_GENERIC_INTERFACE2,
                                hid_user_ev_handler,
                                ENDPOINT_LIST2(),
                                reps2,
                                REPORT_IN_QUEUE_SIZE,
                                REPORT_OUT_MAXSIZE,
                                APP_USBD_HID_SUBCLASS_NONE,  // non booting sub class
                                APP_USBD_HID_PROTO_GENERIC); // generic hid protocol 


/**
 * @brief Global HID keyboard instance
 */
APP_USBD_HID_KBD_GLOBAL_DEF(m_app_hid_kbd,
                            APP_USBD_INTERFACE_KBD,
                            NRF_DRV_USBD_EPIN4,
                            hid_kbd_user_ev_handler,
                            APP_USBD_HID_SUBCLASS_BOOT
);



/**
 * @brief Mark the ongoing transmission
 *
 * Marks that the report buffer is busy and cannot be used until transmission finishes
 * or invalidates (by USB reset or suspend event).
 */
static bool m_report_pending;




/**
 * @brief USBD library specific event handler.
 *
 * @param event     USBD library event.
 * */
static void usbd_user_ev_handler(app_usbd_event_type_t event)
{
    switch (event)
    {
        case APP_USBD_EVT_DRV_SOF:
            break;
        case APP_USBD_EVT_DRV_RESET:
            m_report_pending = false;
            break;
        case APP_USBD_EVT_DRV_SUSPEND:
            m_report_pending = false;
            app_usbd_suspend_req(); // Allow the library to put the peripheral into sleep mode
            break;
        case APP_USBD_EVT_DRV_RESUME:
            m_report_pending = false;
            break;
        case APP_USBD_EVT_STARTED:
            m_report_pending = false;
            break;
        case APP_USBD_EVT_STOPPED:
            app_usbd_disable();
            break;
        case APP_USBD_EVT_POWER_DETECTED:
            NRF_LOG_INFO("USB power detected");
            if (!nrf_drv_usbd_is_enabled())
            {
                app_usbd_enable();
            }
            break;
        case APP_USBD_EVT_POWER_REMOVED:
            NRF_LOG_INFO("USB power removed");
            app_usbd_stop();
            break;
        case APP_USBD_EVT_POWER_READY:
            NRF_LOG_INFO("USB ready");
            app_usbd_start();
            break;
        default:
            break;
    }
}


/**
 * @brief Class specific event handler.
 *
 * @param p_inst    Class instance.
 * @param event     Class specific event.
 * */
static void hid_user_ev_handler(app_usbd_class_inst_t const * p_inst,
                                app_usbd_hid_user_event_t event)
{
    switch (event)
    {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
        {
            /* No output report defined for this example.*/
            ASSERT(0);
            break;
        }
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
        {
            m_report_pending = false;
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
        {
         //   UNUSED_RETURN_VALUE(hid_generic_clear_buffer(p_inst));
            NRF_LOG_INFO("SET_BOOT_PROTO");
            break;
        }
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
        {
         //   UNUSED_RETURN_VALUE(hid_generic_clear_buffer(p_inst));
            NRF_LOG_INFO("SET_REPORT_PROTO");
            break;
        }
        default:
            break;
    }
}

/**
 * @brief Class specific event handler.
 *
 * @param p_inst    Class instance.
 * @param event     Class specific event.
 * */
static void hid_kbd_user_ev_handler(app_usbd_class_inst_t const * p_inst, app_usbd_hid_user_event_t event)
{
    UNUSED_PARAMETER(p_inst);
    switch (event) {
        case APP_USBD_HID_USER_EVT_OUT_REPORT_READY:
            /* Only one output report IS defined for HID keyboard class. Update LEDs state. */
            break;
        case APP_USBD_HID_USER_EVT_IN_REPORT_DONE:
            break;
        case APP_USBD_HID_USER_EVT_SET_BOOT_PROTO:
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));
            break;
        case APP_USBD_HID_USER_EVT_SET_REPORT_PROTO:
            UNUSED_RETURN_VALUE(hid_kbd_clear_buffer(p_inst));

            break;
        default:
            break;
    }
}




/**
 * idle handle for controller 1 used by the nordic libraries
 * @param  p_inst    current app usbd instance
 * @param  report_id report type
 * @return           error state 
 */
static ret_code_t idle_handle(app_usbd_class_inst_t const * p_inst, uint8_t report_id)
{
    switch (report_id)
    {
        case 0:
        {
            uint8_t report[] = {0xBE, 0xEF};
            return app_usbd_hid_generic_idle_report_set(
              &m_app_hid_generic,
              report,
              sizeof(report));
        }
        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }
    
}

/**
 * idle handle for controller 2 used by the nordic libraries
 * @param  p_inst    current app usbd instance
 * @param  report_id report type
 * @return           error state 
 */
static ret_code_t idle_handle2(app_usbd_class_inst_t const * p_inst, uint8_t report_id)
{
    switch (report_id)
    {
        case 0:
        {
            uint8_t report[] = {0xBE, 0xEF};
            return app_usbd_hid_generic_idle_report_set(
              &m_app_hid_generic2,
              report,
              sizeof(report));
        }
        default:
            return NRF_ERROR_NOT_SUPPORTED;
    }
    
}


static uint8_t useKeyboard; 

void init_controllers(uint8_t enablekeyboard)
{
	uint32_t ret; 
    app_usbd_class_inst_t const * class_controller;
    app_usbd_class_inst_t const * class_controller_2;

    static const app_usbd_config_t usbd_config = {
        .ev_state_proc = usbd_user_ev_handler
    };
    
    ret = app_usbd_init(&usbd_config);
    APP_ERROR_CHECK(ret);

    useKeyboard = enablekeyboard;

    if(enablekeyboard)
    {
        class_controller = app_usbd_hid_kbd_class_inst_get(&m_app_hid_kbd);
        ret = app_usbd_class_append(class_controller);
        APP_ERROR_CHECK(ret);  
    }
    else 
    {
        // INIT USBD HID Controller class for controller 1 
        class_controller = app_usbd_hid_generic_class_inst_get(&m_app_hid_generic);
        ret = hid_generic_idle_handler_set(class_controller, idle_handle);
        APP_ERROR_CHECK(ret);
        ret = app_usbd_class_append(class_controller);


        // INIT USBD HID Controller class for controller 2 
        class_controller_2 = app_usbd_hid_generic_class_inst_get(&m_app_hid_generic2);
        ret = hid_generic_idle_handler_set(class_controller_2, idle_handle2);
        APP_ERROR_CHECK(ret); 
        ret = app_usbd_class_append(class_controller_2);
    }

    // Init USBD power events and begin service
    if (USBD_POWER_DETECTION)
    {
        ret = app_usbd_power_events_enable();
        APP_ERROR_CHECK(ret);
    }
    else
    {
        NRF_LOG_INFO("No USB power detection enabled\r\nStarting USB now");

        app_usbd_enable();
        app_usbd_start();
    }
	return;

}

#include "sw_spi.h"

app_usbd_hid_kbd_codes_t kbController1[16] = {
    APP_USBD_HID_KBD_A               ,  /**<KBD_A               code*/
    APP_USBD_HID_KBD_B               ,  /**<KBD_B               code*/
    APP_USBD_HID_KBD_C               ,  /**<KBD_C               code*/
    APP_USBD_HID_KBD_D               ,  /**<KBD_D               code*/
    APP_USBD_HID_KBD_E               ,  /**<KBD_E               code*/
    APP_USBD_HID_KBD_F               ,  /**<KBD_F               code*/
    APP_USBD_HID_KBD_G               , /**<KBD_G               code*/
    APP_USBD_HID_KBD_H               , /**<KBD_H               code*/
    APP_USBD_HID_KBD_I               , /**<KBD_I               code*/
    APP_USBD_HID_KBD_J               , /**<KBD_J               code*/
    APP_USBD_HID_KBD_K               , /**<KBD_K               code*/
    APP_USBD_HID_KBD_L               , /**<KBD_L               code*/
    APP_USBD_HID_KBD_M               , /**<KBD_M               code*/
    APP_USBD_HID_KBD_N               , /**<KBD_N               code*/
    APP_USBD_HID_KBD_O               , /**<KBD_M               code*/
    APP_USBD_HID_KBD_P               , /**<KBD_N               code*/
};

void controller_sendpacket(uint8_t controllerID, uint8_t* data, uint8_t dataLength)
{
    static uint8_t report[HID_REP_SIZE];

    if(dataLength > HID_REP_SIZE)
    	return;

    if (m_report_pending)
        return;

    memcpy(report, data, HID_REP_SIZE);

    if(useKeyboard)
    {
        uint8_t state = true; 
        // cycle through the bytes 
        uint8_t index =0;
        uint8_t data1; 
        for(uint8_t j =0; j < dataLength; j++)
        {
            data1 = data[j];
            index = j * 8; 
            for (uint8_t i = 0; i < 8; i++)
            {
                (data1 & 0x80) ? (state = true) : (state = false);
                data1 <<= 1;
                app_usbd_hid_kbd_key_control(&m_app_hid_kbd, kbController1[(index++)], state);
            }
        }
        
    }
    else 
    {
        // transefer the packet via HID joypad
        if(controllerID == 0)
            app_usbd_hid_generic_in_report_set(&m_app_hid_generic, report, sizeof(report));
        else if(controllerID == 1 )
            app_usbd_hid_generic_in_report_set(&m_app_hid_generic2, report, sizeof(report));
    }

    return;
}
