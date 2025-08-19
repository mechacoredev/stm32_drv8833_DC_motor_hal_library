/*
 * drv8833.h
 *
 *  Created on: Aug 19, 2025
 *      Author: Enes
 */

#ifndef INC_DRV8833_H_
#define INC_DRV8833_H_

#include "stm32f4xx_hal.h"
#include <stdbool.h>

struct DRV8833_t;

typedef struct DRV8833_t *DRV8833_Handle_t;

typedef enum {
	FORWARD, REVERSE
} MotorDirection_t;

typedef struct {

    TIM_HandleTypeDef* tim_handle;

    uint32_t           in1_channel;

    uint32_t           in2_channel;

    // GPIO_TypeDef* nsleep_port;
    // uint16_t           nsleep_pin;

    TIM_HandleTypeDef* ramp_tim_handle;

} DRV8833_Config_t;

void DRV8833_StartRamped(DRV8833_Handle_t dev, MotorDirection_t direction, uint8_t target_speed, uint32_t duration_ms);

void DRV8833_RampInterruptHandler(DRV8833_Handle_t dev);

DRV8833_Handle_t DRV8833_Init(DRV8833_Config_t* config);

void DRV8833_Destroy(DRV8833_Handle_t dev);

void DRV8833_Start(DRV8833_Handle_t dev, MotorDirection_t direction, uint8_t speed);

void DRV8833_SetSpeed(DRV8833_Handle_t dev, uint8_t speed);

void DRV8833_Stop(DRV8833_Handle_t dev);

void DRV8833_Brake(DRV8833_Handle_t dev);

#endif /* INC_DRV8833_H_ */
