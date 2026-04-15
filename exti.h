#ifndef __EXTI_H
#define __EXTI_H

#include "./SYSTEM/sys/sys.h"

/* 外部中断引脚定义 */
#define AD7175_EXIT_GPIO_PORT              GPIOA
#define AD7175_EXIT_GPIO_PIN               GPIO_PIN_4

/* 外部中断初始化函数 */
void exti4_init(void);

#endif

