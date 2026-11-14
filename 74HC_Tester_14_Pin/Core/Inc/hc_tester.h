#ifndef HC_TESTER_H
#define HC_TESTER_H

#include <stdint.h>

void HC_Tester_Init(void);
void HC_Tester_Poll(void);
void HC_Tester_USBReset(void);
void HC_Tester_USBReceive(const uint8_t *data, uint32_t length);

#endif
