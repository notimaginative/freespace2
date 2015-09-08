/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/

#ifndef OAL_EFX_H_
#define OAL_EFX_H_


typedef struct
{
    uint environment;          // 0 to EAX_ENVIRONMENT_COUNT-1
    float fVolume;                      // 0 to 1
    float fDecayTime_sec;               // seconds, 0.1 to 100
    float fDamping;                     // 0 to 1
} EAX_REVERBPROPERTIES;


int oal_efx_init();
void oal_efx_close();

int oal_efx_is_inited();

int oal_efx_get_all(EAX_REVERBPROPERTIES *er, int id);
int oal_efx_set_all(uint id, float vol, float damping, float decay);

#endif // OAL_EFX_H_
