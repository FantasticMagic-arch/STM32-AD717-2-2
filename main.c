/**
 ****************************************************************************************************
 * @file        main.c
 * @author      正点原子团队(ALIENTEK)
 * @version     V1.0
 * @date        2022-09-6
 * @brief       串口通信 实验
 * @license     Copyright (c) 2020-2032, 广州市星翼电子科技有限公司
 ****************************************************************************************************
 * @attention
 *
 * 实验平台:正点原子 阿波罗 H743开发板
 * 在线视频:www.yuanzige.com
 * 技术论坛:www.openedv.com
 * 公司网址:www.alientek.com
 * 购买地址:openedv.taobao.com
 *
 ****************************************************************************************************
 */

#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/usart/usart.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/LED/led.h"
#include "./BSP/AD7175/ad7175.h"

int main(void)
{
    uint8_t len;
    uint16_t times = 0;
    
    uint16_t id;
    uint8_t  status;
    uint16_t adcmode;
    uint16_t ifmode;
    uint16_t ch0;
    uint16_t setup0;
    uint16_t filt0;
    uint32_t data;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(160, 5, 2, 4);         /* 设置时钟, 400Mhz */
    delay_init(400);                            /* 延时初始化 */
    usart_init(115200);                         /* 初始化USART */
    led_init();                                 /* 初始化LED */
    
    
    printf("\r\n");
    printf("AD7175 test start!\r\n");

    /* 第一步：只初始化并配置 */
    ad7175_init();
    delay_ms(20);

    /* 第二步：先做 10 次 ID 稳定性验证 */
    printf("---- ID test ----\r\n");
    for (int i = 0; i < 10; i++)
    {
        ad7175_reset();
        delay_ms(2);

        id = ad7175_read_id();
        printf("ID[%d] = 0x%04X\r\n", i, id);
        delay_ms(20);
    }

    /* 第三步：读一次关键寄存器 */
    printf("\r\n---- Register dump ----\r\n");
    id      = ad7175_read_id();
    status  = ad7175_read_status();
    adcmode = ad7175_read_reg_16(AD7175_REG_ADCMODE);
    ifmode  = ad7175_read_reg_16(AD7175_REG_IFMODE);
    ch0     = ad7175_read_reg_16(AD7175_REG_CH0);
    setup0  = ad7175_read_reg_16(AD7175_REG_SETUP0);
    filt0   = ad7175_read_reg_16(AD7175_REG_FILT0);

    printf("ID      = 0x%04X\r\n", id);
    printf("STATUS  = 0x%02X\r\n", status);
    printf("ADCMODE = 0x%04X\r\n", adcmode);
    printf("IFMODE  = 0x%04X\r\n", ifmode);
    printf("CH0     = 0x%04X\r\n", ch0);
    printf("SETUP0  = 0x%04X\r\n", setup0);
    printf("FILT0   = 0x%04X\r\n", filt0);

    /* 第四步：连续读数据
     * 现在先不做电压换算，只看 DATA 是否稳定更新
     */
    printf("\r\n---- Data read ----\r\n");
    
    
    
    
    while (1)
    {
        status = ad7175_read_status();

        /* STATUS bit7 = RDY，0 表示有新数据 */
        if ((status & 0x80) == 0)
        {
            data = ad7175_read_data();
            printf("STATUS = 0x%02X, DATA = 0x%06lX\r\n", status, data);

            LED0_TOGGLE();
            delay_ms(100);
        }
        else
        {
            delay_ms(5);
        }
    }
        
}
