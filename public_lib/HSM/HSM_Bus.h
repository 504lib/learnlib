#ifndef __HSM_BUS_H__
#define __HSM_BUS_H__

#include "HSM_Core.h"

#ifndef HSM_BUS_MAX
#define HSM_BUS_MAX 8
#endif

typedef struct HSM_Bus {
    HSM* hsms[HSM_BUS_MAX];
    int  count;
} HSM_Bus;

#ifdef __cplusplus
extern "C" {
#endif

void HSM_Bus_Add       (HSM_Bus* bus, HSM* hsm);
void HSM_Bus_Remove    (HSM_Bus* bus, HSM* hsm);
void HSM_Bus_SendEvent (HSM_Bus* bus, HSM_Event_Package ev);
void HSM_Bus_Process   (HSM_Bus* bus);

#ifdef __cplusplus
}
#endif

#endif
