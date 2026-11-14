#!/usr/bin/env python3
"""Host regression tests; no STM32 hardware or ARM compiler required."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory() as temp:
    temp = Path(temp)
    header = '''#include <stdint.h>
#include <stddef.h>
typedef struct { int unused; } GPIO_TypeDef;
typedef struct { int unused; } I2C_HandleTypeDef;
typedef struct { int unused; } ADC_HandleTypeDef;
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
typedef struct { uint32_t Channel, Rank, SamplingTime; } ADC_ChannelConfTypeDef;
extern GPIO_TypeDef mock_port;
#define HAL_OK 0
#define GPIO_MODE_INPUT 0
#define GPIO_MODE_OUTPUT_OD 1
#define GPIO_PULLDOWN 0
#define GPIO_PULLUP 1
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define ADC_SAMPLETIME_480CYCLES 480
#define ADC_CHANNEL_VREFINT 17
#define ADC_CHANNEL_8 8
int HAL_ADC_ConfigChannel(ADC_HandleTypeDef *, ADC_ChannelConfTypeDef *);
int HAL_ADC_Start(ADC_HandleTypeDef *);
int HAL_ADC_Stop(ADC_HandleTypeDef *);
int HAL_ADC_PollForConversion(ADC_HandleTypeDef *, int);
uint32_t HAL_ADC_GetValue(ADC_HandleTypeDef *);
void HAL_GPIO_Init(GPIO_TypeDef *, GPIO_InitTypeDef *);
void HAL_GPIO_WritePin(GPIO_TypeDef *, uint16_t, int);
int HAL_GPIO_ReadPin(GPIO_TypeDef *, uint16_t);
void HAL_Delay(uint32_t);
int HAL_I2C_Master_Transmit(I2C_HandleTypeDef *, uint16_t, uint8_t *, int, int);
int HAL_I2C_IsDeviceReady(I2C_HandleTypeDef *, uint16_t, int, int);
'''
    for pin in [1,2,3,4,5,6,8,9,10,11,12,13]:
        header += f'#define ZIF{pin}_GPIO_Port (&mock_port)\n#define ZIF{pin}_Pin {pin}\n'
    (temp / 'main.h').write_text(header)
    # Substitute only the memory-mapped factory calibration for the host build.
    source = (root / 'Core/Src/hc_tester.c').read_text().replace(
        '(*(const uint16_t *)0x1FFF7A2AU)', '(1500u)')
    (temp / 'hc_tester.c').write_text(source)
    subprocess.run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Wformat-truncation=2', '-I', str(temp),
                    str(root / 'tests/test_hc_tester.c'), '-o', str(temp / 'test')], check=True)
    subprocess.run([str(temp / 'test')], check=True)
