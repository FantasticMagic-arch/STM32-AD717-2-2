#include "./BSP/AD7175/ad7175.h"
#include "./SYSTEM/delay/delay.h"

SPI_HandleTypeDef g_spi1_handle;

/* SPI1 底层初始化
 * 
 * PA5 -> SPI1_SCK
 * PA6 -> SPI1_MISO (DOUT/RDY)
 * PA7 -> SPI1_MOSI (DIN)
 */
static void ad7175_spi1_init(void)
{
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_SPI1_CLK_ENABLE();

    GPIO_InitTypeDef gpio_init_struct;
    gpio_init_struct.Pin = GPIO_PIN_5 | GPIO_PIN_6 | GPIO_PIN_7;
    gpio_init_struct.Mode = GPIO_MODE_AF_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    gpio_init_struct.Alternate = GPIO_AF5_SPI1;
    HAL_GPIO_Init(GPIOA, &gpio_init_struct);
    
    gpio_init_struct.Pin = GPIO_PIN_14;
    gpio_init_struct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_init_struct.Pull = GPIO_PULLUP;
    gpio_init_struct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio_init_struct);
    
    
    g_spi1_handle.Instance = SPI1;
    g_spi1_handle.Init.Mode = SPI_MODE_MASTER;
    g_spi1_handle.Init.Direction = SPI_DIRECTION_2LINES;
    g_spi1_handle.Init.DataSize = SPI_DATASIZE_8BIT;
    
    /* AD7175 数据手册要求 SPI Mode 3 */
    g_spi1_handle.Init.CLKPolarity = SPI_POLARITY_HIGH;
    g_spi1_handle.Init.CLKPhase = SPI_PHASE_2EDGE;
    
    g_spi1_handle.Init.NSS = SPI_NSS_SOFT;
    
    /* 256分频 */
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
    
    /* 配置 PA4 为外部中断下降沿触发，用来接收 RDY 信号 */
    GPIO_InitTypeDef gpio_exti_struct;
    gpio_exti_struct.Pin = GPIO_PIN_4;
    gpio_exti_struct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发中断
    gpio_exti_struct.Pull = GPIO_PULLUP;          // 开启上拉，防止悬空误触发
    HAL_GPIO_Init(GPIOA, &gpio_exti_struct);
    
    /* 设置中断优先级并使能 EXTI4 中断 */
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);       // 优先级设为2 (可根据需要调整)
//    HAL_NVIC_EnableIRQ(EXTI4_IRQn);
    
}

/* 片选CS控制 */
static void ad7175_cs_low(void)
{
    HAL_GPIO_WritePin(AD7175_CS_GPIO_PORT, AD7175_CS_GPIO_PIN, GPIO_PIN_RESET);
}

static void ad7175_cs_high(void)
{
    HAL_GPIO_WritePin(AD7175_CS_GPIO_PORT, AD7175_CS_GPIO_PIN, GPIO_PIN_SET);
}


/* 软件复位：DIN=1，连续送 64 个 SCLK */
void ad7175_reset(void)
{
    uint8_t tx = 0xFF;
    uint8_t rx = 0xFF;

    ad7175_cs_low();
    for (int i = 0; i < 10; i++)
    {
        HAL_SPI_TransmitReceive(&g_spi1_handle, &tx, &rx, 1, 1000);
    }
    ad7175_cs_high();

    delay_ms(2);
}

/* 读 8 位寄存器 */
uint8_t ad7175_read_reg_8(uint8_t reg)
{
    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = 0x40 | (reg & 0x3F);
    txbuf[1] = 0xFF;

    ad7175_cs_low();
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 2, 1000);
    ad7175_cs_high();

    return rxbuf[1];
}

/* 读 16 位寄存器 */
uint16_t ad7175_read_reg_16(uint8_t reg)
{
    uint8_t txbuf[3];
    uint8_t rxbuf[3];

    txbuf[0] = 0x40 | (reg & 0x3F);
    txbuf[1] = 0xFF;
    txbuf[2] = 0xFF;

    ad7175_cs_low();
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 3, 1000);
    ad7175_cs_high();

    return ((uint16_t)rxbuf[1] << 8) | rxbuf[2];
}

/* 读 24 位寄存器 */
uint32_t ad7175_read_reg_24(uint8_t reg)
{
    uint8_t txbuf[4];
    uint8_t rxbuf[4];

    txbuf[0] = 0x40 | (reg & 0x3F); // 第1字节：写命令
    txbuf[1] = 0xFF;                // 后3字节：假数据，用来产生时钟接收ADC数据
    txbuf[2] = 0xFF;
    txbuf[3] = 0xFF;

    ad7175_cs_low();
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 4, 1000);
    ad7175_cs_high();

    // rxbuf[0] 是发命令时收到的无效位，rxbuf[1]~rxbuf[3] 才是真实的24位数据
    return ((uint32_t)rxbuf[1] << 16) | ((uint32_t)rxbuf[2] << 8) | rxbuf[3];
}

/* 写 16 位寄存器 */
void ad7175_write_reg_16(uint8_t reg, uint16_t val)
{
    uint8_t txbuf[3];
    
    txbuf[0] = (reg & 0x3F);   /* 写命令，RW=0 */
    txbuf[1] = (uint8_t)(val >> 8);
    txbuf[2] = (uint8_t)(val & 0xFF);
    
    ad7175_cs_low();
    HAL_SPI_Transmit(&g_spi1_handle, txbuf, 3, 1000);
    ad7175_cs_high();
    delay_ms(1);
}

uint16_t ad7175_read_id(void)
{
    uint16_t id = 0;
    uint8_t tx = 0x47;
    uint8_t rx = 0x00;

    ad7175_cs_low();

    HAL_SPI_TransmitReceive(&g_spi1_handle, &tx, &rx, 1, 1000);

    tx = 0xFF;
    HAL_SPI_TransmitReceive(&g_spi1_handle, &tx, &rx, 1, 1000);
    id = ((uint16_t)rx << 8);

    HAL_SPI_TransmitReceive(&g_spi1_handle, &tx, &rx, 1, 1000);
    id |= rx;

    ad7175_cs_high();

    return id;
}

uint8_t ad7175_read_status(void)
{
    return ad7175_read_reg_8(AD7175_REG_STATUS);
}

uint32_t ad7175_read_data(void)
{
    return ad7175_read_reg_24(AD7175_REG_DATA);
}

/* 初始化 */
void ad7175_init(void)
{
    ad7175_spi1_init();

    /* 模块上电后给一点裕量 */
    delay_ms(20);
    
    /* 先软件复位 */
    ad7175_reset();
    
    /* 工作配置：
     * ADCMODE = 0x8000
     * IFMODE  = 0x0000
     * CH0     = 0x8001
     * SETUP0  = 0x1320
     * FILT0   = 0x020A
     *
     */
    /* 1. ADC模式：使能内部2.5V电源，连续转换，使用内部时钟 */
    ad7175_write_reg_16(0x01, 0x8000);
    delay_ms(2);

    /* 2. 接口模式：24位数据，不带CRC，不开启连续读 */
    ad7175_write_reg_16(0x02, 0x0000);
    delay_ms(2);

    /* 3. 通道0：AIN0正，AIN1负，使用设置0 */
    ad7175_write_reg_16(0x10, 0x8001);
    delay_ms(2);

    /* 4. 配置寄存器0：0x1320 -> 双极性，内部2.5V基准，开启输入缓冲 */
    ad7175_write_reg_16(0x20, 0x1320);
    delay_ms(2);

    /* 5. 滤波器0：Sinc5+1滤波器 降速
    020A=01010 即为1000SPS 
    020B=01011 即为500SPS */
    ad7175_write_reg_16(0x28, 0x020B); 
    delay_ms(2);
    
    
}

// 没用这个函数
// 将 24位 Code 转换为实际电压值 (双极性，2.5V内部基准)
float ad7175_code_to_voltage(uint32_t code)
{
    int32_t signed_code = (int32_t)code - 0x800000;
    return ((float)signed_code * 2.5f) / 8388608.0f;
}

// 专为高速连续采集定制的无 CS 翻转读取函数
uint32_t ad7175_read_data_no_cs(void)
{
    // 直接发送读取数据寄存器(0x04)的指令，后跟3个字节的虚拟时钟
    uint8_t txbuf[4] = {0x44, 0xFF, 0xFF, 0xFF}; 
    uint8_t rxbuf[4] = {0};

    // 绝不拉高拉低 CS，杜绝一切电平跳变带来的电磁干扰！直接收发！
    HAL_SPI_TransmitReceive(&g_spi1_handle, txbuf, rxbuf, 4, 1000);

    return ((uint32_t)rxbuf[1] << 16) | ((uint32_t)rxbuf[2] << 8) | rxbuf[3];
}

