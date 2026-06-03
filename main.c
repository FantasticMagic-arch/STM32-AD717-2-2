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
#include "./BSP/EXTI/exti.h"

/* TF卡需要包含的文件 */
#include "./FATFS/source/ff.h"
#include "./FATFS/exfuns/exfuns.h"
#include "./MALLOC/malloc.h"
#include "./BSP/SDMMC/sdmmc_sdcard.h"

/* 只能放在main函数上面，因为中断回调函数在main函数之外定义的，所以需要全局定义变量 */
#define SAMPLE_COUNT 500  // 定义一个能装 500 个数据的缓存数组 用于暂时存储500个数据
static float voltage_buffer[SAMPLE_COUNT];

/* 双缓冲(Ping-Pong Buffer)数据结构 */
volatile float ping_buffer[SAMPLE_COUNT];
volatile float pong_buffer[SAMPLE_COUNT];

volatile uint16_t buffer_index = 0;      
volatile uint8_t  current_buffer = 0;    // 0: 正在向 ping 写入; 1: 正在向 pong 写入

/* 标志位：通知 main 函数哪个数组满了可以存入 SD 卡 */
volatile uint8_t ping_ready_to_write = 0; 
volatile uint8_t pong_ready_to_write = 0;


    /* ================= 数据导出函数 ================= */
//void dump_sd_data_to_serial(void)
//{
//    FIL file;
//    FRESULT res;
//    UINT br;
//    float read_buffer[50]; // 减小单次读取量，降低内存负担

//    printf("\r\n--- Start Dumping SIP_Data.bin ---\r\n");

//    res = f_open(&file, "0:SIP_Data.bin", FA_READ);
//    if (res == FR_OK)
//    {
//        // 【关键侦测步骤】：先看看 SD 卡里的文件物理大小到底是多少！
//        // 2500个浮点数 * 4个字节 = 10000 字节。
//        uint32_t file_size_bytes = f_size(&file);
//        uint32_t total_points = file_size_bytes / sizeof(float);
////        printf("DEBUG: File Size on SD Card: %lu Bytes\r\n", file_size_bytes);
////        printf("DEBUG: This means there are %lu points inside.\r\n", total_points);
////        printf("---------------------------------------\r\n");
////        delay_ms(2000); // 停顿2秒，让你看清大小

//        while (1)
//        {
//            res = f_read(&file, read_buffer, sizeof(read_buffer), &br);
//            if (res != FR_OK || br == 0) break; 

//            int count = br / sizeof(float);
//            for (int i = 0; i < count; i++)
//            {
//                printf("%f\r\n", read_buffer[i]);
//                
//                // 将延时加大到 10ms，彻底防止串口助手缓存爆炸导致乱码（-0）
//                delay_ms(10); 
//            }
//        }
//        f_close(&file);
//        printf("--- Dump Complete ---\r\n");
//    }
//    else
//    {
//        printf("Failed to open file. Error: %d\r\n", res);
//    }
//}

int main(void)
{
    
    FATFS fs;           /* FatFS 逻辑驱动器工作区 */
    FIL file;           /* 文件对象 */
    UINT bw;            /* 实际写入的字节数 */
    
    /* 用于控制采集时间 */
    uint32_t total_saved_points = 0;
    const uint32_t TARGET_POINTS = 1000; // 设定目标采集点数 (1秒500个点,eg：1000个点为2秒)
    
    
    sys_cache_enable();                         /* 打开L1-Cache */
//    SCB_DisableDCache();                        /* 强制关闭数据缓存，防止 SD卡 DMA 搬运出错卡死 */
    HAL_Init();                                 /* 初始化HAL库 */
    sys_stm32_clock_init(160, 5, 2, 4);         /* 设置时钟, 400Mhz */
    delay_init(400);                            /* 延时初始化 */
    usart_init(115200);                         /* 初始化USART */
    led_init();                                 /* 初始化LED */
    
    
    printf("\r\n---- Multi-Freq IP Instrument Booting ----\r\n");
    
    /* 挂载 SD 卡 */
    if (f_mount(&fs, "0:", 1) != FR_OK) 
    {
        printf("FatFS Mount Failed! Check SD Card.\r\n");
        while(1); 
    }
    
    
    /* 调用数据导出函数 */ //没有SD读卡器，暂且先用
//    dump_sd_data_to_serial();
    
    // 数据导出时，需要把存入时的代码全都注释 
    
    /* 1、初始化ADC系统 */
    ad7175_init();
    delay_ms(20);
    
    /* 2、ID 验证 */
    uint16_t id = ad7175_read_id();
    printf("AD7175 ID = 0x%04X\r\n", id);
    
    /* 3、执行环境零点自校准 */
    printf("Executing Internal Zero-Scale Calibration...\r\n");
    ad7175_environmental_auto_cal();
    printf("Calibration Done.\r\n");
    
    /* 4、初始化外部中断并开始采集 */
    exti4_init();


    
    /* 创建波形存储文件 (直接以二进制写入，最高效) */
    /* FA_CREATE_ALWAYS这个宏定义再FatFS文件系统中的逻辑是：如果卡里没有这个文件，就新建一个；
    如果卡里已经有了同名文件，就直接将其强制清空为 0 字节，然后从头开始写。 */
    if (f_open(&file, "0:SIP_Data.bin", FA_CREATE_ALWAYS | FA_WRITE) != FR_OK) 
    {
        printf("File Open Failed!\r\n"); 
        while(1); 
    }
    printf("File Opened. Start 500SPS continuous capture...\r\n");

    
    /* 拉低片选，准备接收数据 */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
    delay_ms(2);
    ad7175_read_data_no_cs(); // 清理残余数据
    
    
    __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4); // 清除配置期间 MISO 翻转产生的任何虚假中断标志
    HAL_NVIC_EnableIRQ(EXTI4_IRQn);       // 此时才真正开启 EXTI4 中断，迎接纯净的 RDY 脉冲
    printf("Start Capture...\r\n");
    
    // 结束注释 
    
    
    
    while (1)
    {
        /* 同样注释掉 */

        if (ping_ready_to_write) 
        {
            f_write(&file, (void*)ping_buffer, sizeof(ping_buffer), &bw);
            f_sync(&file); 
            ping_ready_to_write = 0; 
            
            total_saved_points += SAMPLE_COUNT; // 累加已保存的点数
//            printf("Ping Buffer OK: %d / %d points\r\n", total_saved_points, TARGET_POINTS);
            printf("Saved %d / %d points\r\n", total_saved_points, TARGET_POINTS);
        }
        
        if (pong_ready_to_write) 
        {
            f_write(&file, (void*)pong_buffer, sizeof(pong_buffer), &bw);
            f_sync(&file); 
            pong_ready_to_write = 0; 
                
            total_saved_points += SAMPLE_COUNT;
//            printf("Pong Buffer OK: %d / %d points\r\n", total_saved_points, TARGET_POINTS);
            printf("Saved %d / %d points\r\n", total_saved_points, TARGET_POINTS);
        }

        // 检查是否达到目标采集量
        if (total_saved_points >= TARGET_POINTS)
        {
            f_close(&file); //安全封口，关闭文件
            
            // 停止 AD7175 转换 (拉高片选，关闭中断)
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); 
            HAL_NVIC_DisableIRQ(EXTI4_IRQn);
            
            printf("--- Capture Complete. File safely closed. ---\r\n");
            
            
            while(1)
            {
                LED0_TOGGLE();
                delay_ms(200); // 快闪表示完成
            }
         }
        /* 结束注释 */
    }
        
}


/* 外部中断回调函数 */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == GPIO_PIN_4) 
    {
        // 1. 读取数据
        uint32_t data = ad7175_read_data_no_cs();
        
        // 清除刚才读取数据期间，DOUT 数据位跳变在 PA4 上留下的所有虚假中断标志
        __HAL_GPIO_EXTI_CLEAR_IT(GPIO_PIN_4); 

        // 2. 换算电压
        float adc_pin_vin = (((int32_t)data - AD7175_HALF_SCALE) * AD7175_VREF) / AD7175_HALF_SCALE;
        //0.0381是零点偏置；0.4944是增益斜率；1.0195是硬件校准系数。
//        float final_voltage = (adc_pin_vin - (-0.0381f)) / (-0.4944f)* 1.0195f;

        // 3. 填入当前正在使用的缓冲区
        if (current_buffer == 0) 
        {
            ping_buffer[buffer_index] = adc_pin_vin;
        }
        else
        {
            pong_buffer[buffer_index] = adc_pin_vin;
        }
        
        buffer_index++;

        // 4. 数组装满 500 个点，立刻切换阵地
        if (buffer_index >= SAMPLE_COUNT) 
        {
            buffer_index = 0;
            if (current_buffer == 0) 
            {
                ping_ready_to_write = 1; 
                current_buffer = 1;      
            }
            else
            {
                pong_ready_to_write = 1; 
                current_buffer = 0;      
            }
        }
    }
}






