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


/* 全局标志位 */
volatile uint8_t g_adc_data_ready = 0;

int main(void)
{
//    uint8_t len;
//    uint16_t times = 0;
    
    uint16_t id;
//    uint8_t  status;
//    uint16_t adcmode;
//    uint16_t ifmode;
//    uint16_t ch0;
//    uint16_t setup0;
//    uint16_t filt0;
    uint32_t data;
    
    sys_cache_enable();                         /* 打开L1-Cache */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(160, 5, 2, 4);         /* 设置时钟, 400Mhz */
    delay_init(400);                            /* 延时初始化 */
    usart_init(115200);                         /* 初始化USART */
    led_init();                                 /* 初始化LED */
    
    
    printf("\r\n");
    printf("AD7175 test start!\r\n");

    /* 初始化 */
    ad7175_init();
    delay_ms(20);

    /* 先做 10 次 ID 稳定性验证 */
    printf("---- ID test ----\r\n");
    for (int i = 0; i < 10; i++)
    {
        ad7175_reset();
        delay_ms(2);              

        uint16_t id = ad7175_read_id();
        printf("ID[%d] = 0x%04X\r\n", i, id);
        delay_ms(20);
    }
    
    
        
    printf("\r\n---- Start 1-Second Sine Wave Capture (500 SPS) ----\r\n");
    
    // 定义一个能装 100 个数据的缓存数组 (100个点在 500SPS 下相当于 0.2 秒的波形数据)
    #define SAMPLE_COUNT 500
    static float voltage_buffer[SAMPLE_COUNT];
    
    while (1)
    {
//        //  一次性拉低片选，开启连续高速采集通道
//        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
//        delay_ms(2); 

//        // 假读一次，清空内部过期垃圾
//        ad7175_read_data_no_cs();

//        // 极速纯净区块采集
//        for (int i = 0; i < SAMPLE_COUNT; i++)
//        {
//            // 直接死等 PA6 (MISO) 物理引脚被 ADC 拉低
//            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET) 
//            {
//                // 此时单片机专心等待
//            }
//            
//            // 抓到低电平，说明新数据准备就绪，立刻读取！
//            uint32_t data = ad7175_read_data_no_cs();
//            
//            // 新增 死等PA6恢复高电平，防止H7太快，在引脚还没来得及拉高时又冲进下一次循环造成双重触发
//            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_RESET) 
//            {
//                // 此时单片机专心等待
//            }
//                
//            // 换算并存入数组
//            float adc_pin_vin = (((int32_t)data - 8388608) * 2.5f) / 8388608.0f;
//            voltage_buffer[i] = (adc_pin_vin - (-0.0381f)) / (-0.4944f);
//        }

//        // 采集完毕，拉高片选，让总线休息
//        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); 

//        // 打印这 100 个点
//        for (int i = 0; i < SAMPLE_COUNT; i++)
//        {
//            printf("%.5f\r\n", voltage_buffer[i]);
//        }
//        
//        printf("--- Batch %d Points Complete ---\r\n", SAMPLE_COUNT);
//        
//        delay_ms(5000);
        // ==========================================
        // 1. 采集前大扫除
        // ==========================================
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
        delay_ms(2); 
        ad7175_read_data_no_cs(); // 扔掉过期数据
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); 

        // ==========================================
        // 2. 严丝合缝的高速波形抓取 (CS 包含在循环内)
        // ==========================================
        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            // A. 拉低片选，等待本次转换完成
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
            
            // B. 死等下降沿 (新数据就绪)
            while (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_6) == GPIO_PIN_SET) {} 
            
            // C. 读取数据
            uint32_t data = ad7175_read_data_no_cs();
            
            // D. 【关键安全动作】拉高片选！强行复位引脚状态，杜绝连续读取的错位
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); 

            // E. 换算
            float adc_pin_vin = (((int32_t)data - 8388608) * 2.5f) / 8388608.0f;
            voltage_buffer[i] = (adc_pin_vin - (-0.0381f)) / (-0.4944f);
        }

        // ==========================================
        // 3. 护航打印 (防止电脑串口助手爆缓存)
        // ==========================================
        for (int i = 0; i < SAMPLE_COUNT; i++)
        {
            printf("%.5f\r\n", voltage_buffer[i]);
            // 【新增】：每次打印完歇 2 毫秒，彻底消灭 Excel 里的空格和乱码！
            delay_ms(2); 
        }
        
        printf("--- End of 1 Second Data ---\r\n");
        
        delay_ms(5000);
    }
        
}

