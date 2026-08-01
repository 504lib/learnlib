#include "HSM_Bus.h"

void HSM_Bus_Add(HSM_Bus* bus, HSM* hsm)
{
    if (!bus || !hsm) return;
    if (bus->count >= HSM_BUS_MAX) return;
    bus->hsms[bus->count++] = hsm;
}

void HSM_Bus_Remove(HSM_Bus* bus, HSM* hsm)
{
    if (!bus || !hsm) return;
    for (int i = 0; i < bus->count; i++) {
        if (bus->hsms[i] == hsm) {
            bus->hsms[i] = bus->hsms[--bus->count];  // swap-last 移除
            return;
        }
    }
}

void HSM_Bus_SendEvent(HSM_Bus* bus, HSM_Event_Package ev)
{
    if (!bus) return;
    for (int i = 0; i < bus->count; i++) {
        HSM_SendEvent(bus->hsms[i], ev);
    }
}

void HSM_Bus_Process(HSM_Bus* bus)
{
    if (!bus) return;
    for (int i = 0; i < bus->count; i++) {
        HSM_Process(bus->hsms[i]);
    }
}
