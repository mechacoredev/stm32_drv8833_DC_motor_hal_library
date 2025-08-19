/*
 * drv8833.c
 *
 * Created on: Aug 19, 2025
 * Author: Enes
 */

#include "drv8833.h"
#include <stdlib.h>
#include <math.h>


struct DRV8833_t {

    TIM_HandleTypeDef* tim_handle;
    uint32_t           in1_channel;
    uint32_t           in2_channel;
    TIM_HandleTypeDef* ramp_tim_handle;


    MotorDirection_t   current_direction;
    uint8_t            current_speed;
    bool               is_running;

    volatile bool      is_ramping;
    volatile uint8_t   target_speed;
    volatile int8_t    speed_step;
};

DRV8833_Handle_t DRV8833_Init(DRV8833_Config_t* config)
{
    DRV8833_Handle_t dev = (DRV8833_Handle_t)malloc(sizeof(struct DRV8833_t));
    if (dev == NULL) {
        return NULL;
    }


    dev->tim_handle   = config->tim_handle;
    dev->in1_channel  = config->in1_channel;
    dev->in2_channel  = config->in2_channel;
    dev->ramp_tim_handle = config->ramp_tim_handle;
    dev->is_ramping	=	false;
    dev->is_running     = false;
    dev->current_speed  = 0;


    HAL_TIM_PWM_Start(dev->tim_handle, dev->in1_channel);
    HAL_TIM_PWM_Start(dev->tim_handle, dev->in2_channel);


    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in1_channel, 0);
    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in2_channel, 0);

    return dev;
}

void DRV8833_StartRamped(DRV8833_Handle_t dev, MotorDirection_t direction, uint8_t target_speed, uint32_t duration_ms)
{
    if (dev == NULL || dev->is_ramping) return;
    if (target_speed > 100) target_speed = 100;

    int16_t speed_diff = target_speed - dev->current_speed;
    if (speed_diff == 0) return;


    dev->current_direction = direction;
    dev->is_running = true;
    dev->target_speed = target_speed;
    dev->is_ramping = true;
    dev->speed_step = (speed_diff > 0) ? 1 : -1;

    uint32_t step_count = abs(speed_diff);

    if (step_count == 0) {
        dev->is_ramping = false;
        return;
    }
    uint32_t interval_ms = duration_ms / step_count;
    if (interval_ms == 0) {
        interval_ms = 1;
    }

    uint32_t tim_clk = HAL_RCC_GetPCLK1Freq();
    if((RCC->CFGR & RCC_CFGR_PPRE1) >> 10 != 0b100) // APB1 Prescaler > 1
    {
        tim_clk *= 2;
    }


    HAL_TIM_Base_Stop_IT(dev->ramp_tim_handle);


    uint32_t new_arr = 999;
    uint32_t new_psc = (tim_clk / ((1000/interval_ms) * (new_arr + 1))) - 1;


    __HAL_TIM_SET_AUTORELOAD(dev->ramp_tim_handle, new_arr);
    __HAL_TIM_SET_PRESCALER(dev->ramp_tim_handle, new_psc);
    __HAL_TIM_SET_COUNTER(dev->ramp_tim_handle, 0);


    HAL_TIM_Base_Start_IT(dev->ramp_tim_handle);

}

void DRV8833_RampInterruptHandler(DRV8833_Handle_t dev)
{
    if (dev == NULL || !dev->is_ramping) return;


    dev->current_speed += dev->speed_step;


    DRV8833_SetSpeed(dev, dev->current_speed);


    if (dev->current_speed == dev->target_speed) {
        dev->is_ramping = false;
        HAL_TIM_Base_Stop_IT(dev->ramp_tim_handle); // Timer interrupt'ını durdur
    }

    if (dev->target_speed == 0) {
        dev->is_running = false;
    }

}

void DRV8833_Destroy(DRV8833_Handle_t dev)
{
	if (dev != NULL)
	{
		DRV8833_Stop(dev);
		HAL_TIM_Base_Stop_IT(dev->ramp_tim_handle);
		HAL_TIM_PWM_Stop(dev->tim_handle, dev->in1_channel);
		HAL_TIM_PWM_Stop(dev->tim_handle, dev->in2_channel);
		free(dev);
	}
}


void DRV8833_Start(DRV8833_Handle_t dev, MotorDirection_t direction, uint8_t speed)
{
	if (dev == NULL) return;

	dev->current_direction = direction;
	dev->is_running = true;


	DRV8833_SetSpeed(dev, speed);
}


void DRV8833_SetSpeed(DRV8833_Handle_t dev, uint8_t speed)
{
    if (dev == NULL || !dev->is_running) return;

    if (speed > 100) {
        speed = 100;
    }
    dev->current_speed = speed;


    uint32_t timer_period = __HAL_TIM_GET_AUTORELOAD(dev->tim_handle);


    uint32_t pwm_pulse = (timer_period * speed) / 100;


    if (dev->current_direction == FORWARD) {

        __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in1_channel, pwm_pulse);
        __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in2_channel, 0);
    } else {

        __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in2_channel, pwm_pulse);
        __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in1_channel, 0);
    }
}


void DRV8833_Stop(DRV8833_Handle_t dev)
{
    if (dev == NULL) return;

    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in1_channel, 0);
    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in2_channel, 0);

    dev->is_running = false;
    dev->current_speed = 0;
}

void DRV8833_Brake(DRV8833_Handle_t dev)
{
    if (dev == NULL) return;


    uint32_t timer_period = __HAL_TIM_GET_AUTORELOAD(dev->tim_handle);

    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in1_channel, timer_period);
    __HAL_TIM_SET_COMPARE(dev->tim_handle, dev->in2_channel, timer_period);

    dev->is_running = false;
    dev->current_speed = 0;
}
