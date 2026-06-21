#ifndef __KEY_CONTROL_H
#define __KEY_CONTROL_H

#include "./multikey/multikey.h"
#include <stdbool.h>
#include <stdint.h>

void KeyControl_Init(void);
void KeyControl_Scan(void);
uint8_t KeyControl_GetDisplayMode(void);
bool KeyControl_IsConfirmed(void);
void KeyControl_ClearConfirm(void);
bool KeyControl_IsReset(void);
void KeyControl_ClearReset(void);

#endif
