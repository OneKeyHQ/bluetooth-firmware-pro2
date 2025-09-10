#include <memory.h>
#include <stdarg.h>

#include "power_manage.h"

#include "nrf_i2c.h"

#include "nrf_delay.h"
#include "nrf_log.h"
#include "nrf_log_ctrl.h"
#include "nrf_gpio.h"
#include "app_scheduler.h"

// workarounds to keep this file clean
static uint8_t stm_data_buff[8];
static void (*send_stm_data_p)(void *pdata, uint16_t lenth);
void set_send_stm_data_p(void (*send_stm_data_p_)(void *pdata, uint16_t lenth))
{
    send_stm_data_p = send_stm_data_p_;
}

static void pmu_data_inform_peer(void *pdata, uint16_t lenth)
{
    send_stm_data_p(pdata, lenth);
}

// ================================
// vars
static PMU_Interface_t pmu_if;
PMU_t                 *pmu_p = NULL;

// ================================
// functions private

static void pmu_if_irq(const uint64_t irq)
{
    uint16_t data_len = 0;

    if (irq == 0) {
        return;
    }

    NRF_LOG_INFO("---------> %d", irq);

    // pmu_p->PullStatus(); // refresh before enter, not here

    if (0 != (irq & (1 << PWR_IRQ_PWR_CONNECTED))) {
        NRF_LOG_INFO("irq PWR_IRQ_PWR_CONNECTED");
        stm_data_buff[0] = BLE_CMD_POWER_STA;
        stm_data_buff[1] = BLE_INSERT_POWER;
        stm_data_buff[2] = (pmu_p->PowerStatus->wiredCharge ? CHARGE_TYPE_USB : CHARGE_TYPE_WIRELESS);
        data_len = 3;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_PWR_DISCONNECTED))) {
        NRF_LOG_INFO("irq PWR_IRQ_PWR_DISCONNECTED");
        stm_data_buff[0] = BLE_CMD_POWER_STA;
        stm_data_buff[1] = BLE_REMOVE_POWER;
        stm_data_buff[2] = 0;
        data_len = 3;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_CHARGING))) {
        NRF_LOG_INFO("irq PWR_IRQ_CHARGING");
        stm_data_buff[0] = BLE_CMD_POWER_STA;
        stm_data_buff[1] = BLE_CHARGING_PWR;
        stm_data_buff[2] = (pmu_p->PowerStatus->wiredCharge ? CHARGE_TYPE_USB : CHARGE_TYPE_WIRELESS);
        data_len = 3;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    // if ( 0 != (irq & (1 << PWR_IRQ_CHARGED)) )
    // {
    //     NRF_LOG_INFO("irq PWR_IRQ_CHARGED");
    //     stm_data_buff[0] = BLE_CMD_POWER_STA;
    //     stm_data_buff[1] = BLE_CHARGE_OVER;
    //     stm_data_buff[2] = (pmu_p->PowerStatus->wiredCharge ? CHARGE_TYPE_USB : CHARGE_TYPE_WIRELESS);
    //     send_stm_data_p(stm_data_buff, 3);
    // }
    if (0 != (irq & (1 << PWR_IRQ_BATT_LOW))) {
        NRF_LOG_INFO("irq PWR_IRQ_BATT_LOW");
    }
    if (0 != (irq & (1 << PWR_IRQ_BATT_CRITICAL))) {
        NRF_LOG_INFO("irq PWR_IRQ_BATT_CRITICAL");
    }
    if (0 != (irq & (1 << PWR_IRQ_PB_PRESS))) {
        NRF_LOG_INFO("irq PWR_IRQ_PB_PRESS");
        stm_data_buff[0] = BLE_CMD_KEY_STA;
        stm_data_buff[1] = BLE_KEY_PRESS;
        data_len = 2;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_PB_RELEASE))) {
        NRF_LOG_INFO("irq PWR_IRQ_PB_RELEASE");
        stm_data_buff[0] = BLE_CMD_KEY_STA;
        stm_data_buff[1] = BLE_KEY_RELEASE;
        data_len = 2;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_PB_SHORT))) {
        NRF_LOG_INFO("irq PWR_IRQ_PB_SHORT");
        stm_data_buff[0] = BLE_CMD_KEY_STA;
        stm_data_buff[1] = BLE_KEY_SHORT_PRESS;
        data_len = 2;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_PB_LONG))) {
        NRF_LOG_INFO("irq PWR_IRQ_PB_LONG");
        stm_data_buff[0] = BLE_CMD_KEY_STA;
        stm_data_buff[1] = BLE_KEY_LONG_PRESS;
        data_len = 2;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }
    if (0 != (irq & (1 << PWR_IRQ_PB_FORCEOFF))) {
    }

    // power error handling
    if (0 != (irq & ((1 << PWR_IRQ_PMU_OVER_TEMP) | (1 << PWR_IRQ_BATT_OVER_TEMP) | (1 << PWR_IRQ_BATT_UNDER_TEMP) |
                     (1 << PWR_IRQ_BATT_OVER_VOLTAGE) | (1 << PWR_IRQ_CHARGE_TIMEOUT)))) {
        if (0 != (irq & (1 << PWR_IRQ_PMU_OVER_TEMP))) {
            NRF_LOG_INFO("irq PWR_IRQ_PMU_OVER_TEMP");
            stm_data_buff[0] = BLE_CMD_POWER_ERR;
            stm_data_buff[1] = BLE_CMD_POWER_ERR__PMU_OVER_TEMP;
            data_len = 2;
            app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
        }
        if (0 != (irq & (1 << PWR_IRQ_BATT_OVER_TEMP))) {
            NRF_LOG_INFO("irq PWR_IRQ_BATT_OVER_TEMP");
            stm_data_buff[0] = BLE_CMD_POWER_ERR;
            stm_data_buff[1] = BLE_CMD_POWER_ERR__BATT_OVER_TEMP;
            data_len = 2;
            app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
        }
        if (0 != (irq & (1 << PWR_IRQ_BATT_UNDER_TEMP))) {
            NRF_LOG_INFO("irq PWR_IRQ_BATT_UNDER_TEMP");
            stm_data_buff[0] = BLE_CMD_POWER_ERR;
            stm_data_buff[1] = BLE_CMD_POWER_ERR__BATT_UNDER_TEMP;
            data_len = 2;
            app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
        }
        if (0 != (irq & (1 << PWR_IRQ_BATT_OVER_VOLTAGE))) {
            NRF_LOG_INFO("irq PWR_IRQ_BATT_OVER_VOLTAGE");
            stm_data_buff[0] = BLE_CMD_POWER_ERR;
            stm_data_buff[1] = BLE_CMD_POWER_ERR__BATT_OVER_VOLTAGE;
            data_len = 2;
            app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
        }
        if (0 != (irq & (1 << PWR_IRQ_CHARGE_TIMEOUT))) {
            NRF_LOG_INFO("irq PWR_IRQ_CHARGE_TIMEOUT");
            stm_data_buff[0] = BLE_CMD_POWER_ERR;
            stm_data_buff[1] = BLE_CMD_POWER_ERR__CHARGE_TIMEOUT;
            data_len = 2;
            app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
        }
    } else {
        NRF_LOG_INFO("irq BLE_CMD_POWER_ERR__NONE");
        stm_data_buff[0] = BLE_CMD_POWER_ERR;
        stm_data_buff[1] = BLE_CMD_POWER_ERR__NONE;
        data_len = 2;
        app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    }

    // if (data_len > 0) {
    //     app_sched_event_put(stm_data_buff, data_len, pmu_data_inform_peer);
    // }
}

static bool pmu_if_gpio_config(uint32_t pin_num, const Power_GPIO_Config_t config)
{
    switch (config) {
        case PWR_GPIO_Config_DEFAULT:
            nrf_gpio_cfg_default(pin_num);
            break;

        case PWR_GPIO_Config_READ_NP:
            nrf_gpio_cfg_input(pin_num, NRF_GPIO_PIN_NOPULL);
            break;

        case PWR_GPIO_Config_READ_PH:
            nrf_gpio_cfg_input(pin_num, NRF_GPIO_PIN_PULLUP);
            break;

        case PWR_GPIO_Config_READ_PL:
            nrf_gpio_cfg_input(pin_num, NRF_GPIO_PIN_PULLDOWN);
            break;

        case PWR_GPIO_Config_WRITE_NP:
            nrf_gpio_cfg_output(pin_num);
            break;

        case PWR_GPIO_Config_WRITE_PH:
            nrf_gpio_cfg(pin_num, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLUP, NRF_GPIO_PIN_S0S1,
                         NRF_GPIO_PIN_NOSENSE);
            break;
        case PWR_GPIO_Config_WRITE_PL:
            nrf_gpio_cfg(pin_num, NRF_GPIO_PIN_DIR_OUTPUT, NRF_GPIO_PIN_INPUT_DISCONNECT, NRF_GPIO_PIN_PULLDOWN, NRF_GPIO_PIN_S0S1,
                         NRF_GPIO_PIN_NOSENSE);
            break;

        case PWR_GPIO_Config_UNUSED:
            nrf_gpio_input_disconnect(pin_num);
            break;

        default:
            return false;
    }

    return true;
}

static bool pmu_if_gpio_write(uint32_t pin_num, const bool high_low)
{
    // must already configured as output
    if (nrf_gpio_pin_dir_get(pin_num) == NRF_GPIO_PIN_DIR_OUTPUT) {
        nrf_gpio_pin_write(pin_num, (high_low ? 1 : 0));
        return true;
    }
    return false;
}

static bool pmu_if_gpio_read(uint32_t pin_num, bool *const high_low)
{
    // must already configured as input
    if (nrf_gpio_pin_dir_get(pin_num) == NRF_GPIO_PIN_DIR_INPUT) {
        *high_low = (nrf_gpio_pin_read(pin_num) != 0);
        return true;
    }
    return false;
}

#ifdef PMU_LOG_NRF_LOG
static void pmu_if_log(const Power_LogLevel_t level, const char *fmt, ...)
{

    // assume SEGGER_RTT_Init() already called

    char    log_buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);
    va_end(args);

    switch (level) {
        case PWR_LOG_LEVEL_ERR:
            SEGGER_RTT_printf(0, "%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_WARN:
            SEGGER_RTT_printf(0, "%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_INFO:
            SEGGER_RTT_printf(0, "%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_DBG:
            SEGGER_RTT_printf(0, "%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_TRACE:
            // not supported
            break;

        case PWR_LOG_LEVEL_OFF:
            break;
        default:
            break;
    }
}
#else
static void pmu_if_log(Power_LogLevel_t level, const char *fmt, ...)
{
    char    log_buffer[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(log_buffer, sizeof(log_buffer), fmt, args);
    va_end(args);

    switch (level) {
        case PWR_LOG_LEVEL_ERR:
            NRF_LOG_ERROR("%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_WARN:
            NRF_LOG_WARNING("%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_INFO:
            NRF_LOG_INFO("%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_DBG:
            NRF_LOG_DEBUG("%s", log_buffer);
            break;
        case PWR_LOG_LEVEL_TRACE:
            // not supported
            break;

        case PWR_LOG_LEVEL_OFF:
            break;
        default:
            break;
    }

    NRF_LOG_FLUSH();
}
#endif

// ================================
// functions public
bool power_manage_init()
{
    PRINT_CURRENT_LOCATION();

    I2C_t *i2c_handle = nrf_i2c_get_instance();

    // interface
    pmu_if.isInitialized         = i2c_handle->isInitialized;
    pmu_if.Init                  = i2c_handle->Init;
    pmu_if.Deinit                = i2c_handle->Deinit;
    pmu_if.Reset                 = i2c_handle->Reset;
    pmu_if.HighDriveStrengthCtrl = i2c_handle->HighDriveStrengthCtrl;
    pmu_if.Send                  = i2c_handle->Send;
    pmu_if.Receive               = i2c_handle->Receive;
    pmu_if.Irq                   = pmu_if_irq;
    pmu_if.Reg.Write             = i2c_handle->Reg.Write;
    pmu_if.Reg.Read              = i2c_handle->Reg.Read;
    pmu_if.Reg.SetBits           = i2c_handle->Reg.SetBits;
    pmu_if.Reg.ClrBits           = i2c_handle->Reg.ClrBits;
    pmu_if.GPIO.Config           = pmu_if_gpio_config;
    pmu_if.GPIO.Write            = pmu_if_gpio_write;
    pmu_if.GPIO.Read             = pmu_if_gpio_read;
    pmu_if.Delay_ms              = nrf_delay_ms;
    pmu_if.Log                   = pmu_if_log;

    // pmu handle

    pmu_p = pmu_probe(&pmu_if);
    if (pmu_p == NULL) {
        return false;
    }

    // init
    if (pmu_p->Init() != PWR_ERROR_NONE) {
        return false;
    }

    // config
    if (pmu_p->Config() != PWR_ERROR_NONE) {
        return false;
    }

    return true;
}

bool power_manage_deinit()
{
    PRINT_CURRENT_LOCATION();

    // deinit
    if (pmu_p->Deinit() != PWR_ERROR_NONE) {
        return false;
    }

    // pmu handle
    pmu_p = NULL;

    // interface
    memset(&pmu_if, 0x00, sizeof(PMU_Interface_t));

    return true;
}

void axp_reg_dump(uint8_t pmu_addr)
{
    uint8_t val = 0x99;

    if (!pmu_if.isInitialized) {
        return;
    }

    pmu_if.Log(PWR_LOG_LEVEL_INFO, "**************** axp_reg_dump ****************");

    for (uint16_t reg = 0; reg <= 0xff; reg++) {
        pmu_if.Reg.Read(pmu_addr, reg, &val);
        pmu_if.Log(PWR_LOG_LEVEL_INFO, "reg 0x%02x = 0x%02x", reg, val);
    }
}

PMU_t *power_manage_ctx_get(void)
{
    return pmu_p;
}

// #define AXP2101_I2C_ADDR            (0x35)
// #define AXP2101_RESET_CFG           (0x17)
// #define AXP2101_BROM                (0xA1)
// #define AXP2101_CONFIG              (0xA2)
// void axp2101_brom_dump()
// {
//     if ( !pmu_if.isInitialized )
//         return;

//     pmu_if.Log(PWR_LOG_LEVEL_INFO, "**************** axp2101_brom_dump ****************");

//     char print_buffer[(sizeof("0x00, ") - 1) * 16 + 1] = {'\0'};
//     uint8_t reg_17 = 0xff;
//     uint8_t reg_a2 = 0xff;
//     uint8_t batt_cal_data[128] = {0xff};

//     pmu_if.Reg.Read(AXP2101_I2C_ADDR, AXP2101_RESET_CFG, &reg_17);
//     pmu_if.Log(
//         PWR_LOG_LEVEL_INFO, "AXP2101_RESET_CFG 0x%02X = " BYTE_TO_BINARY_PATTERN, AXP2101_RESET_CFG,
//         BYTE_TO_BINARY(reg_17)
//     );
//     pmu_if.Reg.Read(AXP2101_I2C_ADDR, AXP2101_CONFIG, &reg_a2);
//     pmu_if.Log(
//         PWR_LOG_LEVEL_INFO, "AXP2101_CONFIG 0x%02X = " BYTE_TO_BINARY_PATTERN, AXP2101_CONFIG, BYTE_TO_BINARY(reg_a2)
//     );

//     // enable BROM access
//     pmu_if.Reg.ClrBits(AXP2101_I2C_ADDR, AXP2101_CONFIG, (1 << 0));
//     pmu_if.Reg.SetBits(AXP2101_I2C_ADDR, AXP2101_CONFIG, (1 << 0));
//     pmu_if.Log(PWR_LOG_LEVEL_INFO, "AXP2101_BROM 0x%02X = ", AXP2101_BROM);

//     const uint8_t bytes_wide = 8;
//     for ( uint16_t idx = 0; idx < sizeof(batt_cal_data); idx++ )
//     {
//         pmu_if.Reg.Read(AXP2101_I2C_ADDR, AXP2101_BROM, &(batt_cal_data[idx]));
//         // pmu_if.Log(PWR_LOG_LEVEL_INFO, "0x%02X", batt_cal_data[idx]);
//         if ( (idx + 1) % bytes_wide == 0 )
//         {
//             for ( uint16_t idxx = 0; idxx < bytes_wide; idxx++ )
//             {
//                 sprintf(
//                     print_buffer + ((sizeof("0x00, ") - 1) * idxx), "0x%02X, ",
//                     batt_cal_data[idx - (bytes_wide - 1) + idxx]
//                 );
//             }
//             pmu_if.Log(PWR_LOG_LEVEL_INFO, "BROM Dump 0x%02X: %s", (idx - (bytes_wide - 1)), print_buffer);
//         }
//     }
//     // disable BROM access
//     pmu_if.Reg.ClrBits(AXP2101_I2C_ADDR, AXP2101_CONFIG, (1 << 0));
// }
