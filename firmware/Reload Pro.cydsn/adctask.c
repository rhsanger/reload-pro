/* ========================================
 *
 * Copyright YOUR COMPANY, THE YEAR
 * All Rights Reserved
 * UNPUBLISHED, LICENSED SOFTWARE.
 *
 * CONFIDENTIAL AND PROPRIETARY INFORMATION
 * WHICH IS THE PROPERTY OF your company.
 *
 * ========================================
*/
#include "project.h"
#include <FreeRTOS.h>
#include <task.h>
#include "tasks.h"
#include "config.h"

static int total_voltage = 0;
static int total_current = 0;

CY_ISR(ADC_ISR_func) {
	uint32 isr_flags = ADC_SAR_INTR_MASKED_REG;
	uint32 range_flags = ADC_SAR_RANGE_INTR_MASKED_REG;
	if(range_flags & (1 << 2)) { // Channel 2
		set_output_mode(OUTPUT_MODE_OFF);

		xQueueSendToBackFromISR(ui_queue, &((ui_event){
			.type=UI_EVENT_OVERTEMP,
			.int_arg=0,
			.when=xTaskGetTickCountFromISR()
		}), NULL);
		xQueueOverwriteFromISR(comms_queue, &((comms_event){
			.type=COMMS_EVENT_OVERTEMP,
		}), NULL);
			}

	ADC_SAR_RANGE_INTR_REG = range_flags;
	ADC_SAR_INTR_REG = isr_flags;
}

void vTaskADC(void *pvParameters) {
	portTickType lastWakeTime = xTaskGetTickCount();

	ADC_Start();
	ADC_SAR_INTR_MASK_REG = 0; // Don't listen to any regular interrupts, just the range check ones
	ADC_IRQ_StartEx(ADC_ISR_func);
	ADC_StartConvert();

	while(1) {
		total_current = total_current - (total_current >> ADC_MIX_RATIO) + ADC_GetResult16(0);
		total_voltage = total_voltage - (total_voltage >> ADC_MIX_RATIO) + ADC_GetResult16(1);
		
		vTaskDelayUntil(&lastWakeTime, configTICK_RATE_HZ / 10);
	}
}

int16 get_raw_current_usage() {
	return total_current >> ADC_MIX_RATIO;
}

int get_current_usage() {
	int ret = ((total_current >> ADC_MIX_RATIO) - settings->adc_current_offset) * settings->adc_current_gain;
	return (ret < 0)?0:ret;
}

int16 get_raw_voltage() {
	return total_voltage >> ADC_MIX_RATIO;
}

int get_voltage() {
	int ret = ((total_voltage >> ADC_MIX_RATIO) - settings->adc_voltage_offset) * settings->adc_voltage_gain;
	return (ret < 0)?0:ret;
}

/* [] END OF FILE */
