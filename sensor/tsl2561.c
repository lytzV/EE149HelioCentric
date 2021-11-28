#include <math.h>
#include "tsl2561.h"
#include "nrf_twi_mngr.h"
#include "buckler.h"
#include "app_error.h"

static uint8_t TSL2561_ADDRESS = TSL2561_ADDR_FLOAT;

static const nrf_twi_mngr_t* twi_mngr; 

uint16_t tsl2561_read_reg(uint8_t i2c_addr, uint8_t reg_addr) {
  uint16_t rx_buf = 0;
  nrf_twi_mngr_transfer_t const read_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr, &reg_addr, 1, NRF_TWI_MNGR_NO_STOP),
    NRF_TWI_MNGR_READ(i2c_addr, &rx_buf, 2, 0),
  };
  ret_code_t error_code = nrf_twi_mngr_perform(twi_mngr, NULL, read_transfer, 2, NULL);
  APP_ERROR_CHECK(error_code);

  uint8_t temp = rx_buf & 0xFF;
  rx_buf >>= 8;
  rx_buf |= (temp << 8);
  return rx_buf;
}

static void tsl2561_write_reg(uint8_t i2c_addr, uint8_t reg_addr, uint16_t data) {
  uint8_t * data8 = (uint8_t*) &data;
  uint8_t buf[3] = {reg_addr, data8[1], data8[0]};
  nrf_twi_mngr_transfer_t const write_transfer[] = {
    NRF_TWI_MNGR_WRITE(i2c_addr, buf, 3, 0),
  };
  ret_code_t error_code = nrf_twi_mngr_perform(twi_mngr, NULL, write_transfer, 1, NULL);
  APP_ERROR_CHECK(error_code);
}

void tsl2561_init(const nrf_twi_mngr_t* twi) {
    twi_mngr = twi;
}

ret_code_t tsl2561_config() {
    tsl2561_write_reg(TSL2561_ADDRESS, (TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL), TSL2561_CONTROL_POWERON);
    tsl2561_write_reg(TSL2561_ADDR_HIGH, (TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL), TSL2561_CONTROL_POWERON);
    return NRF_SUCCESS; 
}

void tsl2561_shutdown() {
    tsl2561_write_reg(TSL2561_ADDRESS, (TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL), TSL2561_CONTROL_POWEROFF);
    tsl2561_write_reg(TSL2561_ADDR_HIGH, (TSL2561_COMMAND_BIT | TSL2561_REGISTER_CONTROL), TSL2561_CONTROL_POWEROFF);
}

uint16_t print_debug() {
    return tsl2561_read_reg(TSL2561_ADDRESS, 0x0A);
}

float tsl2561_read_result(uint8_t addr) {
    // busy loop until result ready
    // return tsl2561_read_reg(TSL2561_ADDRESS, (TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_HIGH));
    while(!(tsl2561_read_reg(addr, (TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_LOW)))) {}
    // read result register
    uint16_t broadband = tsl2561_read_reg(addr, (TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_LOW));
    uint16_t ir = tsl2561_read_reg(addr, (TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN1_LOW));

    uint16_t channel0 = (broadband * 1) >> TSL2561_LUX_CHSCALE;
    uint16_t channel1 = (ir * 1) >> TSL2561_LUX_CHSCALE;

    uint16_t temp = 0;
    /* Do not allow negative lux value */
    if (channel0 > channel1)
        temp = channel0 - channel1;

    /* Round lsb (2^(LUX_SCALE-1)) */
    temp += (1 << (TSL2561_LUX_LUXSCALE - 1));

    /* Strip off fractional portion */
    uint32_t lux = temp >> TSL2561_LUX_LUXSCALE;

    /* Signal I2C had no errors */
    return lux;  
}

// uint32_t* tsl2561_get_data() {
    
//     // read 2 byte value from 2 channels 
//     uint16_t broadband = (((uint16_t)i2c_reg_read(TSL_ADDRESS, TSL2561_COMMAND_BIT | TSL2561_REGISTER_CHAN0_HIGH)) << 8) | i2c_reg_read(TSL_ADDRESS, TSL2561_COMMAND_BIT | TSL2561_REGISTER_CHAN0_LOW);
//     uint16_t broadband = read16(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN0_LOW);
//     uint16_t ir = read16(TSL2561_COMMAND_BIT | TSL2561_WORD_BIT | TSL2561_REGISTER_CHAN1_LOW);

//     // calculate the lux value
//     uint16_t chScale;
//     uint16_t channel1;
//     uint16_t channel0;

//     uint8_t value = (isLow ? 0 : 1);

//     /* Make sure the sensor isn't saturated! */
//     uint16_t clipThreshold;
//     switch (TSL2561_INTEGRATIONTIME_101MS) {
//     case TSL2561_INTEGRATIONTIME_13MS:
//         clipThreshold = TSL2561_CLIPPING_13MS;
//         break;
//     case TSL2561_INTEGRATIONTIME_101MS:
//         clipThreshold = TSL2561_CLIPPING_101MS;
//         break;
//     default:
//         clipThreshold = TSL2561_CLIPPING_402MS;
//         break;
//     }

//     /* Return 65536 lux if the sensor is saturated */
//     if ((broadband > clipThreshold) || (ir > clipThreshold)) {
//         return 65536;
//     }

//     /* Get the correct scale depending on the intergration time */
//     switch (TSL2561_INTEGRATIONTIME_101MS) {
//     case TSL2561_INTEGRATIONTIME_13MS:
//         chScale = TSL2561_LUX_CHSCALE_TINT0;
//         break;
//     case TSL2561_INTEGRATIONTIME_101MS:
//         chScale = TSL2561_LUX_CHSCALE_TINT1;
//         break;
//     default: /* No scaling ... integration time = 402ms */
//         chScale = (1 << TSL2561_LUX_CHSCALE);
//         break;
//     }

//     /* Scale for gain (1x or 16x) */
//     if (!TSL2561_GAIN_1X)
//         chScale = chScale << 4;

//     /* Scale the channel values */
//     channel0 = (broadband * chScale) >> TSL2561_LUX_CHSCALE;
//     channel1 = (ir * chScale) >> TSL2561_LUX_CHSCALE;

//     /* Find the ratio of the channel values (Channel1/Channel0) */
//     unsigned long ratio1 = 0;
//     if (channel0 != 0)
//         ratio1 = (channel1 << (TSL2561_LUX_RATIOSCALE + 1)) / channel0;

//     /* round the ratio value */
//     unsigned long ratio = (ratio1 + 1) >> 1;

//     unsigned int b, m;
//     if ((ratio >= 0) && (ratio <= TSL2561_LUX_K1T)) {
//         b = TSL2561_LUX_B1T;
//         m = TSL2561_LUX_M1T;
//     } else if (ratio <= TSL2561_LUX_K2T) {
//         b = TSL2561_LUX_B2T;
//         m = TSL2561_LUX_M2T;
//     } else if (ratio <= TSL2561_LUX_K3T) {
//         b = TSL2561_LUX_B3T;
//         m = TSL2561_LUX_M3T;
//     } else if (ratio <= TSL2561_LUX_K4T) {
//         b = TSL2561_LUX_B4T;
//         m = TSL2561_LUX_M4T;
//     } else if (ratio <= TSL2561_LUX_K5T) {
//         b = TSL2561_LUX_B5T;
//         m = TSL2561_LUX_M5T;
//     } else if (ratio <= TSL2561_LUX_K6T) {
//         b = TSL2561_LUX_B6T;
//         m = TSL2561_LUX_M6T;
//     } else if (ratio <= TSL2561_LUX_K7T) {
//         b = TSL2561_LUX_B7T;
//         m = TSL2561_LUX_M7T;
//     } else if (ratio > TSL2561_LUX_K8T) {
//         b = TSL2561_LUX_B8T;
//         m = TSL2561_LUX_M8T;
//     }

//     unsigned long temp;
//     channel0 = channel0 * b;
//     channel1 = channel1 * m;

//     temp = 0;
//     /* Do not allow negative lux value */
//     if (channel0 > channel1)
//         temp = channel0 - channel1;

//     /* Round lsb (2^(LUX_SCALE-1)) */
//     temp += (1 << (TSL2561_LUX_LUXSCALE - 1));

//     /* Strip off fractional portion */
//     uint32_t lux = temp >> TSL2561_LUX_LUXSCALE;

//     /* Signal I2C had no errors */
//     return lux;  
// }