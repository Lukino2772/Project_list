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
typedef struct { uint32_t Pin, Mode, Pull, Speed; } GPIO_InitTypeDef;
extern GPIO_TypeDef mock_port;
#define HAL_OK 0
#define GPIO_MODE_INPUT 0
#define GPIO_MODE_OUTPUT_OD 1
#define GPIO_PULLDOWN 0
#define GPIO_PULLUP 1
#define GPIO_SPEED_FREQ_LOW 0
#define GPIO_PIN_SET 1
#define GPIO_PIN_RESET 0
#define DUT_EN_GPIO_Port (&mock_port)
#define DUT_EN_Pin 14
#define StartKey_GPIO_Port (&mock_port)
#define StartKey_Pin 7
static inline uint32_t __get_PRIMASK(void) { return 0; }
static inline void __disable_irq(void) {}
static inline void __set_PRIMASK(uint32_t m) { (void)m; }
uint32_t HAL_GetTick(void);
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
    (temp / 'usbd_cdc_if.h').write_text(
        '#include <stdint.h>\nuint8_t CDC_Transmit_FS(uint8_t *, uint16_t);\n')
    (temp / 'hc_tester.c').write_bytes((root / 'Core/Src/hc_tester.c').read_bytes())
    subprocess.run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Werror',
                    '-Wformat-truncation=2', '-I', str(temp), '-I', str(root / 'Core/Inc'),
                    str(root / 'tests/test_hc_tester.c'), '-o', str(temp / 'test')], check=True)
    subprocess.run([str(temp / 'test')], check=True)
