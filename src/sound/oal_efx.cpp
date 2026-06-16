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

#ifndef __EMSCRIPTEN__

#include "alext.h"
#include "efx-presets.h"


// effects
static LPALGENEFFECTS alGenEffects = nullptr;
static LPALDELETEEFFECTS alDeleteEffects = nullptr;
static LPALEFFECTI alEffecti = nullptr;
static LPALEFFECTF alEffectf = nullptr;
static LPALEFFECTFV alEffectfv = nullptr;
static LPALGETEFFECTF alGetEffectf = nullptr;

// aux effect slots
static LPALGENAUXILIARYEFFECTSLOTS alGenAuxiliaryEffectSlots = nullptr;
static LPALDELETEAUXILIARYEFFECTSLOTS alDeleteAuxiliaryEffectSlots = nullptr;
static LPALISAUXILIARYEFFECTSLOT alIsAuxiliaryEffectSlot = nullptr;
static LPALAUXILIARYEFFECTSLOTI alAuxiliaryEffectSloti = nullptr;
static LPALAUXILIARYEFFECTSLOTIV alAuxiliaryEffectSlotiv = nullptr;
static LPALAUXILIARYEFFECTSLOTF alAuxiliaryEffectSlotf = nullptr;
static LPALAUXILIARYEFFECTSLOTFV alAuxiliaryEffectSlotfv = nullptr;

static uint EFX_active_environment = SND_ENV_GENERIC;
static EFXEAXREVERBPROPERTIES EFX_env_properties = EFX_REVERB_PRESET_GENERIC;
static int EFX_enabled = 0;

static ALuint AL_EFX_aux_id = 0;
static ALuint AL_EFX_effect_id = 0;

static int OAL_efx_inited = 0;

static const EFXEAXREVERBPROPERTIES EFX_ENV_Generic = EFX_REVERB_PRESET_GENERIC;
static const EFXEAXREVERBPROPERTIES EFX_ENV_PaddedCell = EFX_REVERB_PRESET_PADDEDCELL;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Room = EFX_REVERB_PRESET_ROOM;
static const EFXEAXREVERBPROPERTIES EFX_ENV_BathRoom = EFX_REVERB_PRESET_BATHROOM;
static const EFXEAXREVERBPROPERTIES EFX_ENV_LivingRoom = EFX_REVERB_PRESET_LIVINGROOM;
static const EFXEAXREVERBPROPERTIES EFX_ENV_StoneRoom = EFX_REVERB_PRESET_STONEROOM;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Auditorium = EFX_REVERB_PRESET_AUDITORIUM;
static const EFXEAXREVERBPROPERTIES EFX_ENV_ConcertHall = EFX_REVERB_PRESET_CONCERTHALL;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Cave = EFX_REVERB_PRESET_CAVE;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Arena = EFX_REVERB_PRESET_ARENA;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Hangar = EFX_REVERB_PRESET_HANGAR;
static const EFXEAXREVERBPROPERTIES EFX_ENV_CarpetedHallway = EFX_REVERB_PRESET_CARPETEDHALLWAY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Hallway = EFX_REVERB_PRESET_HALLWAY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_StoneCorridor = EFX_REVERB_PRESET_STONECORRIDOR;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Alley = EFX_REVERB_PRESET_ALLEY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Forest = EFX_REVERB_PRESET_FOREST;
static const EFXEAXREVERBPROPERTIES EFX_ENV_City = EFX_REVERB_PRESET_CITY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Mountains = EFX_REVERB_PRESET_MOUNTAINS;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Quarry = EFX_REVERB_PRESET_QUARRY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Plain = EFX_REVERB_PRESET_PLAIN;
static const EFXEAXREVERBPROPERTIES EFX_ENV_ParkingLot = EFX_REVERB_PRESET_PARKINGLOT;
static const EFXEAXREVERBPROPERTIES EFX_ENV_SewerPipe = EFX_REVERB_PRESET_SEWERPIPE;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Underwater = EFX_REVERB_PRESET_UNDERWATER;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Drugged = EFX_REVERB_PRESET_DRUGGED;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Dizzy = EFX_REVERB_PRESET_DIZZY;
static const EFXEAXREVERBPROPERTIES EFX_ENV_Psychotic = EFX_REVERB_PRESET_PSYCHOTIC;


static void oal_efx_set_env_properties()
{
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DENSITY, EFX_env_properties.flDensity);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DIFFUSION, EFX_env_properties.flDiffusion);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAIN, EFX_env_properties.flGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAINHF, EFX_env_properties.flGainHF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAINLF, EFX_env_properties.flGainLF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_TIME, EFX_env_properties.flDecayTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFRATIO, EFX_env_properties.flDecayHFRatio);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_LFRATIO, EFX_env_properties.flDecayLFRatio);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_GAIN, EFX_env_properties.flReflectionsGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_DELAY, EFX_env_properties.flReflectionsDelay);
    alEffectfv(AL_EFX_effect_id, AL_EAXREVERB_REFLECTIONS_PAN, EFX_env_properties.flReflectionsPan);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_GAIN, EFX_env_properties.flLateReverbGain);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_DELAY, EFX_env_properties.flLateReverbDelay);
    alEffectfv(AL_EFX_effect_id, AL_EAXREVERB_LATE_REVERB_PAN, EFX_env_properties.flLateReverbPan);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ECHO_TIME, EFX_env_properties.flEchoTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ECHO_DEPTH, EFX_env_properties.flEchoDepth);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_MODULATION_TIME, EFX_env_properties.flModulationTime);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_MODULATION_DEPTH, EFX_env_properties.flModulationDepth);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_AIR_ABSORPTION_GAINHF, EFX_env_properties.flAirAbsorptionGainHF);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_HFREFERENCE, EFX_env_properties.flHFReference);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_LFREFERENCE, EFX_env_properties.flLFReference);
    alEffectf(AL_EFX_effect_id, AL_EAXREVERB_ROOM_ROLLOFF_FACTOR, EFX_env_properties.flRoomRolloffFactor);
    alEffecti(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFLIMIT, EFX_env_properties.iDecayHFLimit);
}

static void oal_efx_update_env_properties()
{
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_GAIN, EFX_env_properties.flGain);
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_TIME, EFX_env_properties.flDecayTime);
	alEffectf(AL_EFX_effect_id, AL_EAXREVERB_DECAY_HFRATIO, EFX_env_properties.flDecayHFRatio);
}

static void oal_efx_set_environment(uint id)
{
	uint n_id = id;

	switch (n_id) {
		case SND_ENV_GENERIC:
			EFX_env_properties = EFX_ENV_Generic;
			break;

		case SND_ENV_PADDEDCELL:
			EFX_env_properties = EFX_ENV_PaddedCell;
			break;

		case SND_ENV_ROOM:
			EFX_env_properties = EFX_ENV_Room;
			break;

		case SND_ENV_BATHROOM:
			EFX_env_properties = EFX_ENV_BathRoom;
			break;

		case SND_ENV_LIVINGROOM:
			EFX_env_properties = EFX_ENV_LivingRoom;
			break;

		case SND_ENV_STONEROOM:
			EFX_env_properties = EFX_ENV_StoneRoom;
			break;

		case SND_ENV_AUDITORIUM:
			EFX_env_properties = EFX_ENV_Auditorium;
			break;

		case SND_ENV_CONCERTHALL:
			EFX_env_properties = EFX_ENV_ConcertHall;
			break;

		case SND_ENV_CAVE:
			EFX_env_properties = EFX_ENV_Cave;
			break;

		case SND_ENV_ARENA:
			EFX_env_properties = EFX_ENV_Arena;
			break;

		case SND_ENV_HANGAR:
			EFX_env_properties = EFX_ENV_Hangar;
			break;

		case SND_ENV_CARPETEDHALLWAY:
			EFX_env_properties = EFX_ENV_CarpetedHallway;
			break;

		case SND_ENV_HALLWAY:
			EFX_env_properties = EFX_ENV_Hallway;
			break;

		case SND_ENV_STONECORRIDOR:
			EFX_env_properties = EFX_ENV_StoneCorridor;
			break;

		case SND_ENV_ALLEY:
			EFX_env_properties = EFX_ENV_Alley;
			break;

		case SND_ENV_FOREST:
			EFX_env_properties = EFX_ENV_Forest;
			break;

		case SND_ENV_CITY:
			EFX_env_properties = EFX_ENV_City;
			break;

		case SND_ENV_MOUNTAINS:
			EFX_env_properties = EFX_ENV_Mountains;
			break;

		case SND_ENV_QUARRY:
			EFX_env_properties = EFX_ENV_Quarry;
			break;

		case SND_ENV_PLAIN:
			EFX_env_properties = EFX_ENV_Plain;
			break;

		case SND_ENV_PARKINGLOT:
			EFX_env_properties = EFX_ENV_ParkingLot;
			break;

		case SND_ENV_SEWERPIPE:
			EFX_env_properties = EFX_ENV_SewerPipe;
			break;

		case SND_ENV_UNDERWATER:
			EFX_env_properties = EFX_ENV_Underwater;
			break;

		case SND_ENV_DRUGGED:
			EFX_env_properties = EFX_ENV_Drugged;
			break;

		case SND_ENV_DIZZY:
			EFX_env_properties = EFX_ENV_Dizzy;
			break;

		case SND_ENV_PSYCHOTIC:
			EFX_env_properties = EFX_ENV_Psychotic;
			break;

		default:
			n_id = SND_ENV_GENERIC;
			EFX_env_properties = EFX_ENV_Generic;
			break;
	}

	EFX_active_environment = n_id;
}

static bool oal_efx_init_prototypes()
{
	#define GET_PROC(type, func)	\
		do {	\
			(func) = reinterpret_cast<type>(alGetProcAddress(#func));	\
			if ( !(func) ) {	\
				SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "  Couldn't load OpenAL function %s!", #func);	\
				return false;	\
			}	\
		} while(false);

	GET_PROC(LPALGENEFFECTS, alGenEffects);
	GET_PROC(LPALDELETEEFFECTS, alDeleteEffects);
	GET_PROC(LPALEFFECTI, alEffecti);
	GET_PROC(LPALEFFECTF, alEffectf);
	GET_PROC(LPALEFFECTFV, alEffectfv);
	GET_PROC(LPALGETEFFECTF, alGetEffectf);

	GET_PROC(LPALGENAUXILIARYEFFECTSLOTS, alGenAuxiliaryEffectSlots);
	GET_PROC(LPALDELETEAUXILIARYEFFECTSLOTS, alDeleteAuxiliaryEffectSlots);
	GET_PROC(LPALISAUXILIARYEFFECTSLOT, alIsAuxiliaryEffectSlot);
	GET_PROC(LPALAUXILIARYEFFECTSLOTI, alAuxiliaryEffectSloti);
	GET_PROC(LPALAUXILIARYEFFECTSLOTIV, alAuxiliaryEffectSlotiv);
	GET_PROC(LPALAUXILIARYEFFECTSLOTF, alAuxiliaryEffectSlotf);
	GET_PROC(LPALAUXILIARYEFFECTSLOTFV, alAuxiliaryEffectSlotfv);

	return true;
}
#endif	// !__EMSCRIPTEN__


int oal_efx_init()
{
#ifndef __EMSCRIPTEN__
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

	if ( !oal_efx_init_prototypes() ) {
		return -1;
	}

	EFX_active_environment = SND_ENV_GENERIC;
	EFX_env_properties = EFX_ENV_Generic;


	alGenAuxiliaryEffectSlots(1, &AL_EFX_aux_id);

	if (alGetError() != AL_NO_ERROR) {
		nprintf(("Sound", "SOUND ==>  EFX:  Unable to create Aux effect!\n"));
		return -1;
	}

	alGenEffects(1, &AL_EFX_effect_id);

	if (alGetError() != AL_NO_ERROR) {
		nprintf(("Sound", "SOUND ==>  EFX:  Unable to create effect!\n"));
		return -1;
	}

	alEffecti(AL_EFX_effect_id, AL_EFFECT_TYPE, AL_EFFECT_EAXREVERB);

	if (alGetError() != AL_NO_ERROR) {
		nprintf(("Sound", "SOUND ==>  EFX:  EAXReverb not supported!\n"));
		return -1;
	}

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFX_effect_id);

	if (alGetError() != AL_NO_ERROR) {
		nprintf(("Sound", "SOUND ==>  EFX:  Couldn't load effect!\n"));
		return -1;
	}

	OAL_efx_inited = 1;

	oal_efx_set_env_properties();

	return 0;
#else
	return -1;
#endif
}

int oal_efx_is_inited()
{
#ifndef __EMSCRIPTEN__
	return OAL_efx_inited;
#else
	return 0;
#endif
}

void oal_efx_close()
{
#ifndef __EMSCRIPTEN__
 	if ( !OAL_efx_inited ) {
 		return;
 	}

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFFECT_NULL);

	alDeleteEffects(1, &AL_EFX_effect_id);
	AL_EFX_effect_id = 0;

	alDeleteAuxiliaryEffectSlots(1, &AL_EFX_aux_id);
	AL_EFX_aux_id = 0;

	EFX_enabled = 0;

	OAL_efx_inited = 0;
#endif
}

void oal_efx_attach(ALuint source_id)
{
#ifndef __EMSCRIPTEN__
	if ( !OAL_efx_inited ) {
		return;
	}

	// by default it's disabled
	ALint plist[3] = { 0, 0, AL_FILTER_NULL };

	if (EFX_enabled) {
		plist[0] = AL_EFX_aux_id;
	}

	oal_check_for_errors("oal_efx_attach() begin");

	alSourceiv(source_id, AL_AUXILIARY_SEND_FILTER, plist);

	oal_check_for_errors("oal_efx_attach() end");
#endif
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
#ifndef __EMSCRIPTEN__
	if (er == NULL) {
		return -1;
	}

	uint active_env_save = EFX_active_environment;
	EFXEAXREVERBPROPERTIES env_save = EFX_env_properties;
	bool saved = false;

	if ( (id >= 0) && (id != (int)EFX_active_environment) ) {
		saved = true;

		oal_efx_set_environment(id);
	}

	er->environment = EFX_active_environment;
	er->fVolume = EFX_env_properties.flGain;
	er->fDecayTime_sec = EFX_env_properties.flDecayTime;
	er->fDamping = EFX_env_properties.flDecayHFRatio;

	if (saved) {
		EFX_active_environment = active_env_save;
		EFX_env_properties = env_save;
	}

	return 0;
#else
	return -1;
#endif
}

// Set up all the parameters for an environment
//
// id: value from the SND_ENV_* enumeration
// volume: volume for the environment
// damping: damp value for the environment
// decay: decay time in seconds
//
// returns: 0 if successful, otherwise return -1
//
int oal_efx_set_all(uint id, float vol, float damping, float decay)
{
#ifndef __EMSCRIPTEN__
	if ( !OAL_efx_inited ) {
		return -1;
	}

	oal_check_for_errors("oal_efx_set_all() begin");

	// special disabled case (NOTE: does not take immediate affect!)
	if ( (id == SND_ENV_GENERIC) && (vol == 0.0f) && (damping == 0.0f) && (decay == 0.0f) ) {
		EFX_enabled = 0;
		return 0;
	}

	if (id != EFX_active_environment) {
		oal_efx_set_environment(id);
		oal_efx_set_env_properties();
	}

	CAP(vol, AL_EAXREVERB_MIN_GAIN, AL_EAXREVERB_MAX_GAIN);
	CAP(decay, AL_EAXREVERB_MIN_DECAY_TIME, AL_EAXREVERB_MAX_DECAY_TIME);
	CAP(damping, AL_EAXREVERB_MIN_DECAY_HFRATIO, AL_EAXREVERB_MAX_DECAY_HFRATIO);

	EFX_env_properties.flGain = vol;
	EFX_env_properties.flDecayTime = decay;
	EFX_env_properties.flDecayHFRatio = damping;

	oal_efx_update_env_properties();

	alAuxiliaryEffectSloti(AL_EFX_aux_id, AL_EFFECTSLOT_EFFECT, AL_EFX_effect_id);

	EFX_enabled = 1;

	if ( oal_check_for_errors("oal_efx_set_all() end") ) {
		return -1;
	}

	return 0;
#else
	return -1;
#endif
}
