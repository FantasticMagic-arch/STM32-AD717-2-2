#include "./BSP/AD7175/ad7175.h"
#include "./SYSTEM/delay/delay.h"

SPI_HandleTypeDef g_spi1_handle;

/* SPI1 底层初始化
 * 你现在已经改为：
 * PA5 -> SPI1_SCK
 * PA6 -> SPI1_MISO (DOUT/RDY)
 * PA7 -> SPI1_MOSI (DIN)
 */
static void ad7175_spi1_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_NOPULL;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio_init_struct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);

    g_spi1_handle.Instance = SPI1;
    g_spi1_handle.Init.Mode = SPI_MODE_MASTER;
    g_spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    g_spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;

    /* AD7175 数据手册要求 SPI Mode 3 */
    g_spi1_handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    g_spi1_handle.Init.CLKPhase = SPI_PHASE_2EDGE;

    g_spi1_handle.Init.NSS = SPI_NSS_SOFT;

    /* 先降速，优先保证稳定 */
    g_spi1_handle.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;

    g_spi1_handle.Init.FirstBit = SPI_FIRSTBIT_MSB;
    g_spi1_handle.Init.TIMode = SPI_TIMODE_DISABLE;
    g_spi1_handle.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
    g_spi1_handle.Init.CRCPolynomial = 7;
    g_spi1_handle.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
    g_spi1_handle.Init.NSSPolarity = SPI_NSS_POLARITY_LOW;
    g_spi1_handle.Init.FifoThreshold = SPI_FIFO_THRESHOLD_01DATA;
    g_spi1_handle.Init.TxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    g_spi1_handle.Init.RxCRCInitializationPattern = SPI_CRC_INITIALIZATION_ALL_ZERO_PATTERN;
    g_spi1_handle.Init.MasterSSIdleness = SPI_MASTER_SS_IDLENESS_00CYCLE;
    g_spi1_handle.Init.MasterInterDataIdleness = SPI_MASTER_INTERDATA_IDLENESS_00CYCLE;
    g_spi1_handle.Init.MasterReceiverAutoSusp = SPI_MASTER_RX_AUTOSUSP_DISABLE;
    g_spi1_handle.Init.MasterKeepIOState = SPI_MASTER_KEEP_IO_STATE_DISABLE;
    g_spi1_handle.Init.IOSwap = SPI_IO_SWAP_DISABLE;

    HAL_SPI_Init(&g_spi1_handle);
}

/* 软件复位：DIN=1，连续送 64 个 SCLK */
void ad7175_reset(void)
{
    uint8_t txbuf[8];
    for (uint8_t i = 0; i < 8; i++)
    {
        txbuf[i] = 0xFF;
    }

    HAL_SPI_Transmit(&g_spi1_handle, txbuf, 8, 1000);

    /* 手册要求复位后至少等待 500us，这里放宽到 2ms */
    delay_ms(2);
}

/* 读 8 位寄存器
 * 注意：不再做伪 CS 翻转
 * 先发通信寄存器命令，再收数据
 */
uint8_t ad7175_read_reg_8(uint8_t reg)
{
    uint8_t cmd = 0x40 | (reg & 0x3F);
    uint8_t rx  = 0x00;
    uint8_t dummy = 0xFF;

    HAL_SPI_Transmit(&g_spi1_handle, &cmd, 1, 1000);
    HAL_SPI_TransmitReceive(&g_spi1_handle, &dummy, &rx, 1, 1000);

    return rx;
}

/* 读 16 位寄存器 */
uint16_t ad7175_read_reg_16(uint8_t reg)
{
    uint8_t cmd = 0x40 | (reg & 0x3F);
    uint8_t txbuf[2] = {0xFF, 0xFF};
    uint8_t rxbuf[2] = {0x00, 0x00};

    HAL_SPI_Transmit(&g_spi1_handle, &cmd, 1, 1000);
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 2, 1000);

    return ((uint16_t)rxbuf[0] << 8) | rxbuf[1];
}

/* 读 24 位寄存器 */
uint32_t ad7175_read_reg_24(uint8_t reg)
{
    uint8_t cmd = 0x40 | (reg & 0x3F);
    uint8_t txbuf[3] = {0xFF, 0xFF, 0xFF};
    uint8_t rxbuf[3] = {0x00, 0x00, 0x00};

    HAL_SPI_Transmit(&g_spi1_handle, &cmd, 1, 1000);
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 3, 1000);

    return ((uint32_t)rxbuf[0] << 16) |
           ((uint32_t)rxbuf[1] << 8)  |
            (uint32_t)rxbuf[2];
}

/* 写 16 位寄存器 */
void ad7175_write_reg_16(uint8_t reg, uint16_t val)
{
    uint8_t txbuf[3];

    txbuf[0] = (reg & 0x3F);   /* 写命令，RW=0 */
    txbuf[1] = (uint8_t)(val >> 8);
    txbuf[2] = (uint8_t)(val & 0xFF);

    HAL_SPI_Transmit(&g_spi1_handle, txbuf, 3, 1000);
    delay_ms(1);
}

uint16_t ad7175_read_id(void)
{
    return ad7175_read_reg_16(AD7175_REG_ID);
}

uint8_t ad7175_read_status(void)
{
    return ad7175_read_reg_8(AD7175_REG_STATUS);
}

uint32_t ad7175_read_data(void)
{
    return ad7175_read_reg_24(AD7175_REG_DATA);
}

/* 初始化：
 * 现在分两步：
 * 1) 先把总线打通
 * 2) 再写最小配置
 */
void ad7175_init(void)
{
    ad7175_spi1_init();

    /* 模块上电后给一点裕量 */
    delay_ms(20);

    /* 先软件复位 */
    ad7175_reset();

    /* 先验证 ID 是否正常 */
    /* 这里不在 init 里打印，由 main 决定 */

    /* 最小工作配置，采用你同事成功代码里验证过的思路：
     * ADCMODE = 0x8008
     * IFMODE  = 0x0001
     * CH0     = 0x8001
     * SETUP0  = 0x1F04
     * FILT0   = 0x0205
     *
     * 但注意：只有在 main 里先确认 ID 正确后，才真正依赖这些配置读数据。
     */
    ad7175_write_reg_16(AD7175_REG_ADCMODE, 0x8008);
    ad7175_write_reg_16(AD7175_REG_IFMODE,  0x0001);
    ad7175_write_reg_16(AD7175_REG_CH0,     0x8001);
    ad7175_write_reg_16(AD7175_REG_SETUP0,  0x1F04);
    ad7175_write_reg_16(AD7175_REG_FILT0,   0x0205);

    delay_ms(5);
}


