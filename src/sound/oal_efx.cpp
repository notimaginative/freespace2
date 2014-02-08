/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "oal.h"
#include "sound.h"
#include "oal_efx.h"

#include "alext.h"
#include "efx-presets.h"


// effects
LPALGENEFFECTS alGenEffects;
LPALDELETEEFFECTS alDeleteEffects;
LPALEFFECTI alEffecti;
LPALEFFECTF alEffectf;
LPALEFFECTFV alEffectfv;
LPALGETEFFECTF alGetEffectf;

// aux effect slots
LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots;
LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots;
LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot;
LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti;
LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv;
LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf;
LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv;

static uint EFX_active_environment = SND_ENV_GENERIC;
static EFXEAXREVERBPROPERTIES EFX_properties = EFX_REVERB_PRESET_GENERIC;

static ALuint AL_EFX_aux_id = 0;
static ALuint AL_EFX_effect_id = 0;

static int OAL_efx_inited = 0;


static void oal_efx_set_effect_properties()
{
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DENSITY, EFX_properties.flDensity);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DIFFUSION, EFX_properties.flDiffusion);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAIN, EFX_properties.flGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAINHF, EFX_properties.flGainHF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAINLF, EFX_properties.flGainLF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_TIME, EFX_properties.flDecayTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFRATIO, EFX_properties.flDecayHFRatio);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_LFRATIO, EFX_properties.flDecayLFRatio);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_GAIN, EFX_properties.flReflectionsGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_DELAY, EFX_properties.flReflectionsDelay);
    alEffectfv(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_PAN, EFX_properties.flReflectionsPan);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_GAIN, EFX_properties.flLateReverbGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_DELAY, EFX_properties.flLateReverbDelay);
    alEffectfv(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_PAN, EFX_properties.flLateReverbPan);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ECHO_TIME, EFX_properties.flEchoTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ECHO_DEPTH, EFX_properties.flEchoDepth);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_MODULATION_TIME, EFX_properties.flModulationTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_MODULATION_DEPTH, EFX_properties.flModulationDepth);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, EFX_properties.flAirAbsorptionGainHF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_HFREFERENCE, EFX_properties.flHFReference);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LFREFERENCE, EFX_properties.flLFReference);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, EFX_properties.flRoomRolloffFactor);
    alEffecti(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFLIMIT, EFX_properties.iDecayHFLimit);
}

static void oal_efx_update_effect_properties()
{
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAIN, EFX_properties.flGain);
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_TIME, EFX_properties.flDecayTime);
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFRATIO, EFX_properties.flDecayHFRatio);
}

static void oal_efx_set_environment(uint id)
{
	uint n_id = id;

	if (n_id == SND_ENV_GENERIC) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_GENERIC;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_PADDEDCELL) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_PADDEDCELL;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_ROOM) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_ROOM;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_BATHROOM) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_BATHROOM;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_LIVINGROOM) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_LIVINGROOM;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_STONEROOM) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_STONEROOM;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_AUDITORIUM) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_AUDITORIUM;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_CONCERTHALL) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_CONCERTHALL;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_CAVE) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_CAVE;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_ARENA) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_ARENA;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_HANGAR) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_HANGAR;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_CARPETEDHALLWAY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_CARPETEDHALLWAY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_HALLWAY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_HALLWAY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_STONECORRIDOR) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_STONECORRIDOR;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_ALLEY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_ALLEY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_FOREST) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_FOREST;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_CITY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_CITY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_MOUNTAINS) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_MOUNTAINS;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_QUARRY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_QUARRY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_PLAIN) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_PLAIN;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_PARKINGLOT) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_PARKINGLOT;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_SEWERPIPE) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_SEWERPIPE;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_UNDERWATER) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_UNDERWATER;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_DRUGGED) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_DRUGGED;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_DIZZY) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_DIZZY;
		EFX_properties = ptmp;
	} else if (n_id == SND_ENV_PSYCHOTIC) {
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_PSYCHOTIC;
		EFX_properties = ptmp;
	} else {
		n_id = SND_ENV_GENERIC;
		EFXEAXREVERBPROPERTIES ptmp = EFX_REVERB_PRESET_GENERIC;
		EFX_properties = ptmp;
	}

	oal_efx_set_effect_properties();

	EFX_active_environment = n_id;
}

int oal_efx_init()
{
	if (OAL_efx_inited) {
		return 0;
	}

	// make sure we support EFX and the EAXREVERB effect
	if ( !alcIsExtensionPresent(alcGetContextsDevice(alcGetCurrentContext()), "ALC_EXT_EFX") ) {
		return -1;
	}

	if ( !alGetEnumValue("AL_EFFECT_EAXREVERB") ) {
		return -1;
	}

	// effects
	alGenEffects = (LPALGENEFFECTS) alGetProcAddress("alGenEffects");
	alDeleteEffects = (LPALDELETEEFFECTS) alGetProcAddress("alDeleteEffects");
	alEffecti = (LPALEFFECTI) alGetProcAddress("alEffecti");
	alEffectf = (LPALEFFECTF) alGetProcAddress("alEffectf");
	alEffectfv = (LPALEFFECTFV) alGetProcAddress("alEffectfv");
	alGetEffectf = (LPALGETEFFECTF) alGetProcAddress("alGetEffectf");

	SDL_assert_release( alGenEffects != NULL );
	SDL_assert_release( alDeleteEffects != NULL );
	SDL_assert_release( alEffecti != NULL );
	SDL_assert_release( alEffectf != NULL );
	SDL_assert_release( alEffectfv != NULL );
	SDL_assert_release( alGetEffectf != NULL );

	// aux effect slots
	alGenAuxiliaryEffectSlots = (LPALGENAUXILIARYEFFECTSLOTS) alGetProcAddress("alGenAuxiliaryEffectSlots");
	alDeleteAuxiliaryEffectSlots = (LPALDELETEAUXILIARYEFFECTSLOTS) alGetProcAddress("alDeleteAuxiliaryEffectSlots");
	alIsAuxiliaryEffectSlot = (LPALISAUXILIARYEFFECTSLOT) alGetProcAddress("alIsAuxiliaryEffectSlot");
	alAuxiliaryEffectSloti = (LPALAUXILIARYEFFECTSLOTI) alGetProcAddress("alAuxiliaryEffectSloti");
	alAuxiliaryEffectSlotiv = (LPALAUXILIARYEFFECTSLOTIV) alGetProcAddress("alAuxiliaryEffectSlotiv");
	alAuxiliaryEffectSlotf = (LPALAUXILIARYEFFECTSLOTF) alGetProcAddress("alAuxiliaryEffectSlotf");
	alAuxiliaryEffectSlotfv = (LPALAUXILIARYEFFECTSLOTFV) alGetProcAddress("alAuxiliaryEffectSlotfv");

	SDL_assert_release( alGenAuxiliaryEffectSlots != NULL );
	SDL_assert_release( alDeleteAuxiliaryEffectSlots != NULL );
	SDL_assert_release( alIsAuxiliaryEffectSlot != NULL );
	SDL_assert_release( alAuxiliaryEffectSloti != NULL );
	SDL_assert_release( alAuxiliaryEffectSlotiv != NULL );
	SDL_assert_release( alAuxiliaryEffectSlotf != NULL );
	SDL_assert_release( alAuxiliaryEffectSlotfv != NULL );


	EFX_active_environment = SND_ENV_GENERIC;


	alGenAuxiliaryEffectSlots(1, &AL_EFX_aux_id);

	if (alGetError() != AL_NO_ERROR) {
		mprintf(("\n  EFX:  Unable to create Aux effect!\n"));
		return -1;
	}

	alGenEffects(1, &AL_EFX_effect_id);

	if (alGetError() != AL_NO_ERROR) {
		mprintf(("\n  EFX:  Unable to create effect!\n"));
		return -1;
	}

	alEffecti(AL_EFX_effect_id, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

	if (alGetError() != AL_NO_ERROR) {
		mprintf(("\n  EFX:  EAXReverb not supported!\n"));
		return -1;
	}

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFX_effect_id);

	if (alGetError() != AL_NO_ERROR) {
		mprintf(("\n  EFX:  Couldn't load effect!\n"));
		return -1;
	}

	oal_efx_set_effect_properties();

	OAL_efx_inited = 1;

	return 0;
}

int oal_efx_is_inited()
{
	return OAL_efx_inited;
}

void oal_efx_close()
{
 	if ( !OAL_efx_inited ) {
 		return;
 	}

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);

	alDeleteEffects(1, &AL_EFX_effect_id);
	AL_EFX_effect_id = 0;

	alDeleteAuxiliaryEffectSlots(1, &AL_EFX_aux_id);
	AL_EFX_aux_id = 0;

	OAL_efx_inited = 0;
}

// Get up the parameters for the current environment
//
// er: (output) hold environment parameters
// id: if set will get specified preset env, otherwise current env
//
// returns: 0 if successful, otherwise return -1
//
int oal_efx_get_all(EAX_REVERBPROPERTIES *er, int id)
{
	if (er == NULL) {
		return -1;
	}

	if ( (id < 0) || (id == (int)EFX_active_environment) ) {
		er->environment = EFX_active_environment;
		er->fVolume = EFX_properties.flGain;
		er->fDecayTime_sec = EFX_properties.flDecayTime;
		er->fDamping = EFX_properties.flDecayHFRatio;
	} else {
		// ignoring alternate environments for now
		return -1;
	}

	return 0;
}

// Set up all the parameters for an environment
//
// id: value from teh EAX_ENVIRONMENT_* enumeration
// volume: volume for the environment (0 to 1.0)
// damping: damp value for the environment (0 to 2.0)
// decay: decay time in seconds (0.1 to 20.0)
//
// returns: 0 if successful, otherwise return -1
//
int oal_efx_set_all(uint id, float vol, float damping, float decay)
{
	if ( !OAL_efx_inited ) {
		return -1;
	}

	oal_check_for_errors("oal_efx_set_all() begin");

	// special disabled case
	if ( (id == SND_ENV_GENERIC) && (vol == 0.0f) && (damping == 0.0f) && (decay == 0.0f) ) {
		ALint props[3] = { AL_EFFECT_NULL, 0, AL_FILTER_NULL };

		oal_set_source_properties_all(AL_AUXILIARY_SEND_FILTER, props);

		return 0;
	}

	if (id != EFX_active_environment) {
		oal_efx_set_environment(id);
	}

	CAP(vol, AL_EAXREVERB_MIN_GAIN, AL_EAXREVERB_MAX_GAIN);
	CAP(decay, AL_EAXREVERB_MIN_DECAY_TIME, AL_EAXREVERB_MAX_DECAY_TIME);
	CAP(damping, AL_EAXREVERB_MIN_DECAY_HFRATIO, AL_EAXREVERB_MAX_DECAY_HFRATIO);

	EFX_properties.flGain = vol;
	EFX_properties.flDecayTime = decay;
	EFX_properties.flDecayHFRatio = damping;

	oal_efx_update_effect_properties();

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFX_effect_id);

	if ( oal_check_for_errors("oal_efx_set_all() end") ) {
		return -1;
	}

	return 0;
}
