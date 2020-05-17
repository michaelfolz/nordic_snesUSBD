#include "nintendo_i2c.h"

static uint8_t NINTENDO_TWI_INIT_DATA_REG0[2] = { 0x00, 0x00 };
static uint8_t NINTENDO_TWI_INIT_DATA_REG[2] = { 0xf0, 0x55 };
static uint8_t NINTENDO_TWI_INIT_DATA_REG2[2] = { 0xfb, 0x00 };
static uint8_t snesClassicID[NINTENDO_DEVICE_ID_LENGTH] = {0xA4, 0x20, 0x01, 0x01};

// TWI - data stored 
static uint8_t nintendo_twi_raw_sample[NINTENDO_NUMBER_OF_CONTROLLERS][NINTENDO_TWI_BUFFER];
static uint8_t nintendo_device_type[NINTENDO_NUMBER_OF_CONTROLLERS];

snes_controller currentSNESController[NINTENDO_NUMBER_OF_CONTROLLERS]; 
snes_controller previousSNESController[NINTENDO_NUMBER_OF_CONTROLLERS]; 


/**
 * nintendo_get_device_id - pulls the current device type from the controller id
 * @param  controllerID [description]
 * @return              [description]
 */
uint8_t nintendo_get_device_id(uint8_t controllerID)
{
    uint8_t values[10];
    char diff =0;
    uint8_t reg = 0x00; 

    twi_rx(controllerID, NINTENDO_TWI_DEV_ADDRESS, reg, 1);

    /* Read 1 byte from the specified address - skip 3 bits dedicated for fractional part of temperature. */
    ret_code_t err_code = twi_rx(controllerID, NINTENDO_TWI_DEV_ADDRESS, &nintendo_twi_raw_sample[controllerID], NINTENDO_DEVICE_ID_LENGTH+1);

    // The first byte for the OFFICIAL SNES controller isn't valid 
    diff = memcmp(&nintendo_twi_raw_sample[controllerID][1], snesClassicID, NINTENDO_DEVICE_ID_LENGTH);
    if(!diff)
    {
    	nintendo_device_type[controllerID] = SNES_OFFICIAL_CONTROLLER_DATA_LOCATION;
 		return 1;
    }

    // check for clone "8-bit do"
    diff = memcmp(nintendo_twi_raw_sample[controllerID], snesClassicID, NINTENDO_DEVICE_ID_LENGTH); 
    if(!diff)
    {
		nintendo_device_type[controllerID] = SNES_8BITDO_CONTROLLER_DATA_LOCATION;
     	return 1;
    }

    return 0; 
}



/**
 * sends out request to start reading input and obtain controller type 
 * @param controllerID current controller id
 */
void nintendo_init_controller(uint8_t controllerID)
{
    ret_code_t err_code;

    // send out init request 
    err_code = twi_tx(controllerID, NINTENDO_TWI_DEV_ADDRESS, NINTENDO_TWI_INIT_DATA_REG, 2);
    nrf_delay_us(250);
    
    err_code = twi_tx(controllerID, NINTENDO_TWI_DEV_ADDRESS, NINTENDO_TWI_INIT_DATA_REG2, 2);
    nrf_delay_us(250);

    // pull controller id
    err_code = nintendo_get_device_id(controllerID);
    nrf_delay_us(250);

    return; 
 }

/**
 * reads current controller button and io data
 * @param controllerID 
 */
void nintendo_read_controller_data(uint8_t controllerID)
{
    uint8_t reg = 0x00; 
    uint8_t location = nintendo_device_type[controllerID]; 

    ret_code_t err_code = twi_rx(controllerID, NINTENDO_TWI_DEV_ADDRESS, &nintendo_twi_raw_sample[controllerID], NINTENDO_DEVICE_GAMEPAD_DATA_LENGTH);
    if(nintendo_device_type[controllerID] != SNES_OFFICIAL_CONTROLLER_DATA_LOCATION)
    	nrf_delay_us(5); 
    else 
    	nrf_delay_us(50);

    // write 0x00, 0x00 this "resets the controller"
    err_code = twi_tx(controllerID, NINTENDO_TWI_DEV_ADDRESS, NINTENDO_TWI_INIT_DATA_REG0, 2, false);
    if(nintendo_device_type[controllerID] != SNES_OFFICIAL_CONTROLLER_DATA_LOCATION)
    	nrf_delay_us(5); 
    else 
    	nrf_delay_us(50);

   	currentSNESController[controllerID].bytes[0] = nintendo_twi_raw_sample[controllerID][location]; 
   	currentSNESController[controllerID].bytes[1] = nintendo_twi_raw_sample[controllerID][location+1];

    return;
}

/**
 *  Assemble the controller data packet
 * @param controllerID  current controller
 * @param data          output data packet
 */
void nintendo_get_hid_controller_data(uint8_t controllerID, uint8_t *data)
{
    data[0] = ~(currentSNESController[controllerID].bytes[0]); 
    data[1] = ~(currentSNESController[controllerID].bytes[1]);
    data[2] = 0x00;
    data[3] = 0x00;
    return;
}

/**
 * notify system of any change in the controller state
 * @param  controllerID  current controller 
 * @return               update state
 */
uint8_t nintendo_compare_controller_data(uint8_t controllerID)
{
    uint8_t update = false; 

    if((previousSNESController[controllerID].bytes[0]  != currentSNESController[controllerID].bytes[0]) || 
    	(previousSNESController[controllerID].bytes[1]  != currentSNESController[controllerID].bytes[1]))
    update = true;

    previousSNESController[controllerID].bytes[0]  = currentSNESController[controllerID].bytes[0]; 
    previousSNESController[controllerID].bytes[1]  = currentSNESController[controllerID].bytes[1]; 

    return update;
}
