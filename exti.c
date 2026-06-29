#include "./SYSTEM/sys/sys.h"
#include "./SYSTEM/delay/delay.h"
#include "./BSP/EXTI/exti.h"

/* PA4 外部中断初始化 (专门接收 AD7175 的 RDY 脉冲) */
void exti4_init(void)
{
    /* 配置 PA4 为外部中断下降沿触发，用来接收 RDY 信号 */
    GPIO_InitTypeDef gpio_exti_struct;
    
    __HAL_RCC_GPIOA_CLK_ENABLE();
    
    
    gpio_exti_struct.Pin = AD7175_EXIT_GPIO_PIN;
    gpio_exti_struct.Mode = GPIO_MODE_IT_FALLING; // 下降沿触发中断
    gpio_exti_struct.Pull = GPIO_PULLUP;          // 开启上拉，防止悬空误触发
    HAL_GPIO_Init(AD7175_EXIT_GPIO_PORT, &gpio_exti_struct);
    
    /* 设置中断优先级并使能 EXTI4 中断 */
    HAL_NVIC_SetPriority(EXTI4_IRQn, 2, 0);       // 优先级设为2 (可根据需要调整)
//    HAL_NVIC_EnableIRQ(EXTI4_IRQn); //绝不在这里使能中断，留给主循环控制，在这里使能中断会使后续ADC都中断了
    
}

/* 硬件中断入口 */
// 当 PA4 引脚发生下降沿跳变，STM32 硬件会第一时间跳进这个函数
void EXTI4_IRQHandler(void)
{
    // 这行代码的作用：清除单片机内部的中断挂起标志位，并自动去调用 HAL_GPIO_EXTI_Callback
    HAL_GPIO_EXTI_IRQHandler(AD7175_EXIT_GPIO_PIN); 
}


/* PD3 外部中断初始化 (专门接收发射机的同步脉冲) */
void sync_exti_init(void)
{
    GPIO_InitTypeDef gpio_exti_struct;
    __HAL_RCC_GPIOD_CLK_ENABLE(); // 开启GPIOD时钟

    gpio_exti_struct.Pin = GPIO_PIN_3;            // 使用引脚 3
    gpio_exti_struct.Mode = GPIO_MODE_IT_RISING;  // 上升沿触发
    gpio_exti_struct.Pull = GPIO_PULLDOWN;        // 下拉
    HAL_GPIO_Init(GPIOD, &gpio_exti_struct);      // 

    HAL_NVIC_SetPriority(EXTI3_IRQn, 1, 0);       // 改成 EXTI3_IRQn
    HAL_NVIC_EnableIRQ(EXTI3_IRQn);               // 改成 EXTI3_IRQn
}

/* PD3 的硬件中断入口 */
void EXTI3_IRQHandler(void)                   // 改成 EXTI3_IRQHandler
{
    HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);     // 改成 GPIO_PIN_3
}

