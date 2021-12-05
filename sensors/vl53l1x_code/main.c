#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>

#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "nrf_uarte.h"
#include "nrf_power.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_log_default_backends.h"

#include "app_uart.h"

#include "buckler.h"
#include "display.h"
#include "kobukiActuator.h"
#include "kobukiUtilities.h"
#include "kobukiSensorPoll.h"
#include "kobukiSensorTypes.h"
#include "mpu9250.h"

// Pin configurations
#define LED NRF_GPIO_PIN_MAP(0, 17)
#define UART_RX NRF_GPIO_PIN_MAP(0, 15)
#define UART_TX NRF_GPIO_PIN_MAP(0, 16)
#define UART_TX_BUF_SIZE 256
#define UART_RX_BUF_SIZE 256

float data;
uint8_t *data_array = (uint8_t *)&data;
uint32_t r_error = 0;
int i = 0;

// I2C manager
// NRF_TWI_MNGR_DEF(twi_mngr_instance, 5, 0);

// error handler for UART
void uart_error_handle(app_uart_evt_t *p_event)
{
    if (p_event->evt_type == APP_UART_DATA)
    {
        if (i == 4)
        {
            printf("Reading %f\n", data);
            if (data < 2)
            {
                kobukiDriveDirect(20, 20);
            }
            else
            {
                kobukiDriveDirect(0, 0);
            }
            char buf[32];
            snprintf(buf, 32, "%f", data);
            display_write(buf, DISPLAY_LINE_1);
            i = 0;
            data = 0;
        }
        uint8_t r_data = 0;
        r_error = app_uart_get(&r_data);
        if (r_error == NRF_SUCCESS)
        {
            data_array[i] = r_data;
        }
        else
        {
            printf("Reading error!");
        }
        i++;
    }
    // just call app error handler
    else if (p_event->evt_type == APP_UART_COMMUNICATION_ERROR)
    {
        //APP_ERROR_HANDLER(p_event->data.error_communication);
    }
    else if (p_event->evt_type == APP_UART_FIFO_ERROR)
    {
        APP_ERROR_HANDLER(p_event->data.error_code);
    }
}

// initialization of UART
void uart_init(void)
{
    uint32_t err_code;

    // configure RX and TX pins
    // no RTS or CTS pins with flow control disabled
    // no parity
    // baudrate 115200
    const app_uart_comm_params_t comm_params = {
        UART_RX,
        UART_TX,
        0,
        0,
        APP_UART_FLOW_CONTROL_DISABLED,
        false,
        NRF_UARTE_BAUDRATE_115200};

    // actually initialize UART
    APP_UART_FIFO_INIT(&comm_params, UART_RX_BUF_SIZE, UART_TX_BUF_SIZE,
                       uart_error_handle, APP_IRQ_PRIORITY_LOW, err_code);
    APP_ERROR_CHECK(err_code);

    printf("ERROR: %d\n", err_code);
}

int main(void)
{
    // initialize RTT library
    ret_code_t error_code = NRF_LOG_INIT(NULL);
    APP_ERROR_CHECK(error_code);
    NRF_LOG_DEFAULT_BACKENDS_INIT();
    printf("Log initialized!\n");

    // initialize i2c master (two wire interface)
    // nrf_drv_twi_config_t i2c_config = NRF_DRV_TWI_DEFAULT_CONFIG;
    // i2c_config.scl = BUCKLER_SENSORS_SCL;
    // i2c_config.sda = BUCKLER_SENSORS_SDA;
    // i2c_config.frequency = NRF_TWIM_FREQ_100K;
    // error_code = nrf_twi_mngr_init(&twi_mngr_instance, &i2c_config);
    // APP_ERROR_CHECK(error_code);
    // mpu9250_init(&twi_mngr_instance);
    // printf("IMU initialized!\n");

    // initialize display
    nrf_drv_spi_t spi_instance = NRF_DRV_SPI_INSTANCE(1);
    nrf_drv_spi_config_t spi_config = {
        .sck_pin = BUCKLER_LCD_SCLK,
        .mosi_pin = BUCKLER_LCD_MOSI,
        .miso_pin = BUCKLER_LCD_MISO,
        .ss_pin = BUCKLER_LCD_CS,
        .irq_priority = NRFX_SPI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .orc = 0,
        .frequency = NRF_DRV_SPI_FREQ_4M,
        .mode = NRF_DRV_SPI_MODE_2,
        .bit_order = NRF_DRV_SPI_BIT_ORDER_MSB_FIRST};
    error_code = nrf_drv_spi_init(&spi_instance, &spi_config, NULL, NULL);
    APP_ERROR_CHECK(error_code);
    display_init(&spi_instance);
    printf("Display initialized!\n");

    // initialize kobuki
    // kobukiInit();
    // KobukiSensors_t sensors = {0};
    // printf("Kobuki initialized!\n");

    // init uart
    uart_init();

    while (1)
    {
        //kobukiSensorPoll(&sensors);
        nrf_delay_ms(1);
    }
}