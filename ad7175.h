#ifndef __AD7175_H
#define __AD7175_H

#include "./SYSTEM/sys/sys.h"
#include "stm32h7xx_hal.h"

/* 引脚定义 */
// SPI1_CS
#define AD7175_CS_GPIO_PORT      GPIOB
#define AD7175_CS_GPIO_PIN       GPIO_PIN_14

// SPI1_SCLK
#define AD7175_SCLK_GPIO_PORT    GPIOA
#define AD7175_SCLK_GPIO_PIN     GPIO_PIN_5

// SPI1_MISO(DOUT/RDY)
#define AD7175_MISO_GPIO_PORT    GPIOA
#define AD7175_MISO_GPIO_PIN     GPIO_PIN_6

// SPI1_MOSI(DIN)
#define AD7175_MOSI_GPIO_PORT    GPIOA
#define AD7175_MOSI_GPIO_PIN     GPIO_PIN_7

/* 寄存器地址 */
#define AD7175_REG_STATUS        0x00  // 状态寄存器 (8位，只读)
#define AD7175_REG_ADCMODE       0x01  // ADC模式寄存器 (16位)
#define AD7175_REG_IFMODE        0x02  // 接口模式寄存器 (16位)
#define AD7175_REG_DATA          0x04  // 数据寄存器 (24位或32位)
#define AD7175_REG_ID            0x07  // ID寄存器 (16位，只读，默认值应为 0x0CDx)
#define AD7175_REG_CH0           0x10  // 通道0映射寄存器 (16位)
#define AD7175_REG_SETUP0        0x20  // 设置配置寄存器0 (16位)
#define AD7175_REG_FILT0         0x28  // 滤波器配置 1，禁用滤波器，设置 27 次/秒的滤波器速率，设置 25K 的采样率


/* 函数声明 */
void ad7175_init(void);
void ad7175_reset(void);

uint8_t  ad7175_read_reg_8(uint8_t reg);
uint16_t ad7175_read_reg_16(uint8_t reg);
uint32_t ad7175_read_reg_24(uint8_t reg);
uint32_t ad7175_read_data_no_cs(void);

void ad7175_write_reg_16(uint8_t reg, uint16_t val);
void ad7175_environmental_auto_cal(void);

uint16_t ad7175_read_id(void);
uint8_t  ad7175_read_status(void);
uint32_t ad7175_read_data(void);

#endif

