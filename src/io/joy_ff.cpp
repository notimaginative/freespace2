/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell 
 * or otherwise commercially exploit the source or things you created based on the 
 * source.
 *
*/ 


#include "pstypes.h"
#include "vecmat.h"
#include "osregistry.h"
#include "joy_ff.h"
#include "osapi.h"


static int Joy_ff_enabled = 0;
static int Joy_ff_acquired = 0;
static SDL_Haptic *haptic = NULL;
static int joy_ff_handling_scaler = 0;
static int Joy_ff_directional_hit_effect_enabled = 1;
static int Joy_rumble = 0;
static int Joy_ff_afterburning = 0;

typedef struct {
	SDL_HapticEffect eff;
	int id;
	int loaded;
} haptic_effect_t;

static haptic_effect_t pHitEffect1;
static haptic_effect_t pHitEffect2;
static haptic_effect_t pAfterburn1;
static haptic_effect_t pAfterburn2;
static haptic_effect_t pShootEffect;
static haptic_effect_t pSecShootEffect;
static haptic_effect_t pSpring;
static haptic_effect_t pDock;
//static haptic_effect_t pDeathroll1;
//static haptic_effect_t pDeathroll2;
//static haptic_effect_t pExplode;

static void joy_ff_create_effects();
static int joy_ff_effect_playing(haptic_effect_t *eff);
static void joy_ff_start_effect(haptic_effect_t *eff, const char *name);

extern SDL_Joystick *sdljoy;


int joy_ff_init()
{
	int ff_enabled = 0;

	ff_enabled = os_config_read_uint("Controls", "EnableJoystickFF", 0);

	if ( !ff_enabled || !SDL_JoystickIsHaptic(sdljoy) ) {
		return 0;
	}

	if (SDL_InitSubSystem(SDL_INIT_HAPTIC) < 0) {
		mprintf(("  ERROR: Unable to initialize haptic subsystem\n"));
		return -1;
	}

	haptic = SDL_HapticOpenFromJoystick(sdljoy);

	if (haptic == NULL) {
		mprintf(("  ERROR: Unable to open haptic joystick\n"));
		SDL_QuitSubSystem(SDL_INIT_HAPTIC);
		return -1;
	}

	if ( SDL_HapticRumbleSupported(haptic) ) {
		SDL_HapticRumbleInit(haptic);
		Joy_rumble = 1;
	}

	mprintf(("  Rumble  : %s\n", Joy_rumble ? "Yes" : "No"));
	mprintf(("  Axes    : %d\n", SDL_HapticNumAxes(haptic)));
	mprintf(("  Max effects     : %d\n", SDL_HapticNumEffects(haptic)));
	mprintf(("  Running effects : %d\n", SDL_HapticNumEffectsPlaying(haptic)));

	joy_ff_create_effects();

	Joy_ff_enabled = 1;
	Joy_ff_acquired = 1;

	Joy_ff_directional_hit_effect_enabled = os_config_read_uint("Controls", "EnableHitEffect", 1);

	return 0;
}

void joy_ff_shutdown()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	Joy_rumble = 0;

	SDL_HapticClose(haptic);
	haptic = NULL;

	SDL_QuitSubSystem(SDL_INIT_HAPTIC);

	Joy_ff_acquired = 0;
	Joy_ff_enabled = 0;
}

static void joy_ff_create_effects()
{
	// clear all SDL errors
	SDL_ClearError();

	unsigned int supported = 0;

	supported = SDL_HapticQuery(haptic);

	if ( !(supported & SDL_HAPTIC_CONSTANT) ) {
		mprintf(("  Constant Force  : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SINE) ) {
		mprintf(("  Sine Wave       : not supported\n"));
		Warning(LOCATION, "Sine Wave: not supported");
	}

	if ( !(supported & SDL_HAPTIC_SAWTOOTHDOWN) ) {
		mprintf(("  Sawtooth Down   : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SPRING) ) {
		mprintf(("  Spring          : not supported\n"));
	//	Error(LOCATION, "Spring: not supported");
	}
/*
	if ( !(supported & SDL_HAPTIC_SQUARE) ) {
		mprintf(("  Square          : not supported\n"));
	}
*/
	if ( !(supported & SDL_HAPTIC_TRIANGLE) ) {
		mprintf(("  Triangle        : not supported\n"));
	}


	if (supported & SDL_HAPTIC_CONSTANT) {
		// pHitEffect1
		memset(&pHitEffect1, 0, sizeof(haptic_effect_t));

		pHitEffect1.eff.type = SDL_HAPTIC_CONSTANT;
		pHitEffect1.eff.constant.direction.type = SDL_HAPTIC_POLAR;
		pHitEffect1.eff.constant.direction.dir[0] = 0;
		pHitEffect1.eff.constant.length = 300;
		pHitEffect1.eff.constant.level = 0x7FFF;
		pHitEffect1.eff.constant.attack_length = 0;
		pHitEffect1.eff.constant.attack_level = 0x7FFF;
		pHitEffect1.eff.constant.fade_length = 120;
		pHitEffect1.eff.constant.fade_level = 1;

		pHitEffect1.id = SDL_HapticNewEffect(haptic, &pHitEffect1.eff);

		if (pHitEffect1.id < 0) {
			mprintf(("    Hit effect 1 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pHitEffect1.loaded = 1;
		}
	}

	if (supported & SDL_HAPTIC_SINE) {
		// pHitEffect2
		memset(&pHitEffect2, 0, sizeof(haptic_effect_t));

		pHitEffect2.eff.type = SDL_HAPTIC_SINE;
		pHitEffect2.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pHitEffect2.eff.periodic.direction.dir[0] = 9000;
		pHitEffect2.eff.periodic.length = 300;
		pHitEffect2.eff.periodic.period = 100;
		pHitEffect2.eff.periodic.magnitude = 0x7FFF;
		pHitEffect2.eff.periodic.attack_length = 100;
		pHitEffect2.eff.periodic.fade_length = 100;

		pHitEffect2.id = SDL_HapticNewEffect(haptic, &pHitEffect2.eff);

		if (pHitEffect2.id < 0) {
			mprintf(("    Hit effect 2 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pHitEffect2.loaded = 1;
		}
	}

	if (supported & SDL_HAPTIC_SAWTOOTHDOWN) {
		// pShootEffect
		memset(&pShootEffect, 0, sizeof(haptic_effect_t));

		pShootEffect.eff.type = SDL_HAPTIC_SAWTOOTHDOWN;
		pShootEffect.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pShootEffect.eff.periodic.direction.dir[0] = 0;
		pShootEffect.eff.periodic.length = 160;
		pShootEffect.eff.periodic.period = 20;
		pShootEffect.eff.periodic.magnitude = 0x7FFF;
		pShootEffect.eff.periodic.fade_length = 120;

		pShootEffect.id = SDL_HapticNewEffect(haptic, &pShootEffect.eff);

		if (pShootEffect.id < 0) {
			mprintf(("    Fire primary effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pShootEffect.loaded = 1;
		}
	}

	if (supported & SDL_HAPTIC_CONSTANT) {
		// pSecShootEffect
		memset(&pSecShootEffect, 0, sizeof(haptic_effect_t));

		pSecShootEffect.eff.type = SDL_HAPTIC_CONSTANT;
		pSecShootEffect.eff.constant.direction.type = SDL_HAPTIC_POLAR;
		pSecShootEffect.eff.constant.direction.dir[0] = 0;
		pSecShootEffect.eff.constant.length = 200;
		pSecShootEffect.eff.constant.level = 0x7FFF;
		pSecShootEffect.eff.constant.attack_length = 50;
		pSecShootEffect.eff.constant.attack_level = 0x7FFF;
		pSecShootEffect.eff.constant.fade_length = 100;
		pSecShootEffect.eff.constant.fade_level = 1;

		pSecShootEffect.id = SDL_HapticNewEffect(haptic, &pSecShootEffect.eff);

		if (pSecShootEffect.id < 0) {
			mprintf(("    Fire secondary effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pSecShootEffect.loaded = 1;
		}
	}

	if (supported & SDL_HAPTIC_SPRING) {
		// pSpring
		memset(&pSpring, 0, sizeof(haptic_effect_t));

		pSpring.eff.type = SDL_HAPTIC_SPRING;
		pSpring.eff.condition.length = SDL_HAPTIC_INFINITY;

		for (int i = 0; i < SDL_HapticNumAxes(haptic); i++) {
			pSpring.eff.condition.right_sat[i] = 0x7FFF;
			pSpring.eff.condition.left_sat[i] = 0x7FFF;
			pSpring.eff.condition.right_coeff[i] = 0x147;
			pSpring.eff.condition.left_coeff[i] = 0x147;
		}

		pSpring.id = SDL_HapticNewEffect(haptic, &pSpring.eff);

		if (pSpring.id < 0) {
			mprintf(("    Spring effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pSpring.loaded = 1;
			joy_ff_start_effect(&pSpring, "Spring");
		}
	}

	if (supported & SDL_HAPTIC_SINE) {
		// pAfterburn1
		memset(&pAfterburn1, 0, sizeof(haptic_effect_t));

		pAfterburn1.eff.type = SDL_HAPTIC_SINE;
		pAfterburn1.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pAfterburn1.eff.periodic.direction.dir[0] = 0;
		pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn1.eff.periodic.period = 20;
		pAfterburn1.eff.periodic.magnitude = 0x3332;

		pAfterburn1.id = SDL_HapticNewEffect(haptic, &pAfterburn1.eff);

		if (pAfterburn1.id < 0) {
			mprintf(("    Afterburn effect 1 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pAfterburn1.loaded = 1;
		}

		// pAfterburn2
		memset(&pAfterburn2, 0, sizeof(haptic_effect_t));

		pAfterburn2.eff.type = SDL_HAPTIC_SINE;
		pAfterburn2.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pAfterburn2.eff.periodic.direction.dir[0] = 9000;
		pAfterburn2.eff.periodic.length = 125;
		pAfterburn2.eff.periodic.period = 100;
		pAfterburn2.eff.periodic.magnitude = 0x1999;

		pAfterburn2.id = SDL_HapticNewEffect(haptic, &pAfterburn2.eff);

		if (pAfterburn2.id < 0) {
			mprintf(("    Afterburn effect 2 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pAfterburn2.loaded = 1;
		}
	}


//	if (supported & SDL_HAPTIC_SQUARE) {
	if (supported & SDL_HAPTIC_TRIANGLE) {
		// pDock
		memset(&pDock, 0, sizeof(haptic_effect_t));

	//	pDock.eff.type = SDL_HAPTIC_SQUARE;
		pDock.eff.type = SDL_HAPTIC_TRIANGLE;
		pDock.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pDock.eff.periodic.direction.dir[0] = 9000;
		pDock.eff.periodic.length = 125;
		pDock.eff.periodic.period = 100;
		pDock.eff.periodic.magnitude = 0x3332;

		pDock.id = SDL_HapticNewEffect(haptic, &pDock.eff);

		if (pDock.id < 0) {
			mprintf(("    Dock effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pDock.loaded = 1;
		}
	}

/*
	if (supported & SDL_HAPTIC_SAWTOOTHDOWN) {
		// pExplode
		memset(&pExplode, 0, sizeof(haptic_effect_t));

		pExplode.eff.type = SDL_HAPTIC_SAWTOOTHDOWN;
		pExplode.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pExplode.eff.periodic.direction.dir[0] = 9000;
		pExplode.eff.periodic.length = 500;
		pExplode.eff.periodic.period = 20;
		pExplode.eff.periodic.magnitude = 0x7FFF;
		pExplode.eff.periodic.attack_length = 0;
		pExplode.eff.periodic.fade_length = 500;

		pExplode.id = SDL_HapticNewEffect(haptic, &pExplode.eff);

		if (pExplode.id < 0) {
			mprintf(("    Explosion effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pExplode.loaded = 1;
		}
	}

	if (supported & SDL_HAPTIC_SINE) {
		// pDeathroll1
		memset(&pDeathroll1, 0, sizeof(haptic_effect_t));

		pDeathroll1.eff.type = SDL_HAPTIC_SINE;
		pDeathroll1.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pDeathroll1.eff.periodic.direction.dir[0] = 0;
		pDeathroll1.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pDeathroll1.eff.periodic.period = 200;
		pDeathroll1.eff.periodic.magnitude = 0x7FFF;
		pDeathroll1.eff.periodic.attack_length = 200;

		pDeathroll1.id = SDL_HapticNewEffect(haptic, &pDeathroll1.eff);

		if (pDeathroll1.id < 0) {
			mprintf(("    Deathroll effect 1 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pDeathroll1.loaded = 1;
		}

		// pDeathroll2
		memset(&pDeathroll2, 0, sizeof(haptic_effect_t));

		pDeathroll2.eff.type = SDL_HAPTIC_SINE;
		pDeathroll2.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pDeathroll2.eff.periodic.direction.dir[0] = 9000;
		pDeathroll2.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pDeathroll2.eff.periodic.period = 200;
		pDeathroll2.eff.periodic.magnitude = 0x7FFF;
		pDeathroll2.eff.periodic.attack_length = 200;

		pDeathroll2.id = SDL_HapticNewEffect(haptic, &pDeathroll2.eff);

		if (pDeathroll2.id < 0) {
			mprintf(("    Deathroll effect 2 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pDeathroll2.loaded = 1;
		}
	}
*/
}

static void joy_ff_start_effect(haptic_effect_t *eff, const char *name)
{
	if ( !eff->loaded ) {
		return;
	}

//	nprintf(("Joystick", "FF: Starting effect %s\n", name));

	if ( SDL_HapticRunEffect(haptic, eff->id, 1) ) {
		mprintf(("HapticERROR:  Unable to run %s:\n  %s\n", name, SDL_GetError()));
	}
}

void joy_ff_stop_effects()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	SDL_HapticStopAll(haptic);
}

void joy_ff_mission_init(vector v)
{
	v.xyz.z = 0.0f;

	joy_ff_handling_scaler = (int) ((vm_vec_mag(&v) + 1.3f) * 5.0f);

	Joy_ff_afterburning = 0;
}

void joy_reacquire_ff()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if (Joy_ff_acquired) {
		return;
	}

	joy_ff_start_effect(&pSpring, "Spring");

	Joy_ff_acquired = 1;
}

void joy_unacquire_ff()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	joy_ff_stop_effects();

	Joy_ff_acquired = 0;
}

void joy_ff_play_vector_effect(vector *v, float scaler)
{
	vector vf;
	float x, y;

//	nprintf(("Joystick", "FF: vec = { %f, %f, %f } s = %f\n", v->xyz.x, v->xyz.y, v->xyz.z, scaler));
	vm_vec_copy_scale(&vf, v, scaler);
	x = vf.xyz.x;
	vf.xyz.x = 0.0f;

	if (vf.xyz.y + vf.xyz.z < 0.0f) {
		y = -vm_vec_mag(&vf);
	} else {
		y = vm_vec_mag(&vf);
	}

	joy_ff_play_dir_effect(-x, -y);
}

void joy_ff_play_dir_effect(float x, float y)
{
	int idegs, imag;
	float degs;

	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	// allow for at least one of the effects to work
	if ( !pHitEffect1.loaded && !pHitEffect2.loaded ) {
		return;
	}

	if (joy_ff_effect_playing(&pHitEffect1) || joy_ff_effect_playing(&pHitEffect2)) {
		nprintf(("Joystick", "FF: HitEffect already playing.  Skipping\n"));
		return;
	}

	if (Joy_ff_directional_hit_effect_enabled) {
		if (x > 8000.0f) {
			x = 8000.0f;
		} else if (x < -8000.0f) {
			x = -8000.0f;
		}

		if (y > 8000.0f) {
			y = 8000.0f;
		} else if (y < -8000.0f) {
			y = -8000.0f;
		}

		imag = (int) fl_sqrt(x * x + y * y);
		if (imag > 10000) {
			imag = 10000;
		}

		degs = (float)atan2(x, y);
		idegs = (int) (degs * 18000.0f / PI) + 90;
		while (idegs < 0) {
			idegs += 36000;
		}

		while (idegs >= 36000) {
			idegs -= 36000;
		}

		if (pHitEffect1.loaded) {
			pHitEffect1.eff.constant.direction.dir[0] = idegs;
			pHitEffect1.eff.constant.level = (Sint16)(32767.0f * (imag / 10000.0f));

			if ( SDL_HapticUpdateEffect(haptic, pHitEffect1.id, &pHitEffect1.eff) < 0 ) {
				mprintf(("HapticERROR:  Unable to update pHitEffect1:\n  %s\n", SDL_GetError()));
			}
		}

		idegs += 9000;
		if (idegs >= 36000)
			idegs -= 36000;

		if (pHitEffect2.loaded) {
			pHitEffect2.eff.periodic.direction.dir[0] = idegs;
			pHitEffect2.eff.periodic.magnitude = (Sint16)(32767.0f * (imag / 10000.0f));

			if ( SDL_HapticUpdateEffect(haptic, pHitEffect2.id, &pHitEffect2.eff) < 0 ) {
				mprintf(("HapticERROR:  Unable to update pHitEffect2:\n  %s\n", SDL_GetError()));
			}
		}
	}

	joy_ff_start_effect(&pHitEffect1, "HitEffect1");
	joy_ff_start_effect(&pHitEffect2, "HitEffect2");
}

void joy_ff_play_primary_shoot(int gain)
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !pShootEffect.loaded && !Joy_rumble ) {
		return;
	}

	if (gain < 1) {
		gain = 1;
	} else if (gain > 10000) {
		gain = 10000;
	}

	if (pShootEffect.loaded) {
		SDL_HapticStopEffect(haptic, pShootEffect.id);

		static int primary_ff_level = 10000;

		if (gain != primary_ff_level) {
			pShootEffect.eff.periodic.direction.dir[0] = 0;
			pShootEffect.eff.periodic.length = 160;
			pShootEffect.eff.periodic.magnitude = (Sint16)(32767.0f * (gain / 10000.0f));
			pShootEffect.eff.periodic.fade_length = 120;

			if ( SDL_HapticUpdateEffect(haptic, pShootEffect.id, &pShootEffect.eff) < 0 ) {
				mprintf(("HapticERROR:  Unable to update pShootEffect:\n  %s\n", SDL_GetError()));
			}

			primary_ff_level = gain;
		}

		joy_ff_start_effect(&pShootEffect, "ShootEffect");
	} else if (Joy_rumble) {
		SDL_HapticRumblePlay(haptic, (gain / 10000.0f) * 0.5f, 100);
	}
}

void joy_ff_play_secondary_shoot(int gain)
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !pSecShootEffect.loaded && !Joy_rumble ) {
		return;
	}

	gain = gain * 100 + 2500;

	if (gain < 1) {
		gain = 1;
	} else if (gain > 10000) {
		gain = 10000;
	}

	if (pSecShootEffect.loaded) {
		SDL_HapticStopEffect(haptic, pSecShootEffect.id);

		static int secondary_ff_level = 10000;

		if (gain != secondary_ff_level) {
			pSecShootEffect.eff.constant.level = (Sint16)(32767.0f * (gain / 10000.0f));
			pSecShootEffect.eff.constant.length = (150000 + gain * 25) / 1000;

			if ( SDL_HapticUpdateEffect(haptic, pSecShootEffect.id, &pSecShootEffect.eff) < 0 ) {
				mprintf(("HapticERROR:  Unable to update pSecShootEffect:\n  %s\n", SDL_GetError()));
			}

			secondary_ff_level = gain;
			nprintf(("Joystick", "FF: Secondary force = 0x%04x\n", pSecShootEffect.eff.constant.level));
		}

		joy_ff_start_effect(&pSecShootEffect, "SecShootEffect");
	} else if (Joy_rumble) {
		SDL_HapticRumblePlay(haptic, (gain / 10000.0f) * 0.5f, (150000 + gain * 25) / 1000);
	}
}

void joy_ff_adjust_handling(int speed)
{
	int v;
	short coeff = 0;

	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !pSpring.loaded ) {
		return;
	}

	static int last_speed = -1000;

	if (speed == last_speed) {
		return;
	}

	last_speed = speed;

	v = speed * joy_ff_handling_scaler * 2 / 3;
//	v += joy_ff_handling_scaler * joy_ff_handling_scaler * 6 / 7 + 250;
	v += joy_ff_handling_scaler * 45 - 500;

	if (v < 0) {
		v = 0;
	} else if (v > 10000) {
		v = 10000;
	}

	coeff = (short)(32767.0f * (v / 10000.0f));

	for (int i = 0; i < SDL_HapticNumAxes(haptic); i++) {
		pSpring.eff.condition.right_coeff[i] = coeff;
		pSpring.eff.condition.left_coeff[i] = coeff;
	}

//	nprintf(("Joystick", "FF: New handling force = 0x%04x\n", coeff));

	SDL_HapticUpdateEffect(haptic, pSpring.id, &pSpring.eff);
}

static int joy_ff_effect_playing(haptic_effect_t *eff)
{
	if ( !eff->loaded ) {
		return 0;
	} else {
		return (SDL_HapticGetEffectStatus(haptic, eff->id) > 0);
	}
}

void joy_ff_docked()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !pDock.loaded ) {
		return;
	}

	SDL_HapticStopEffect(haptic, pDock.id);

	pDock.eff.periodic.magnitude = 0x3332;

	if ( SDL_HapticUpdateEffect(haptic, pDock.id, &pDock.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pDock:\n  %s\n", SDL_GetError()));
	}

	joy_ff_start_effect(&pDock, "Dock");
}

void joy_ff_play_reload_effect()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !pDock.loaded ) {
		return;
	}

	SDL_HapticStopEffect(haptic, pDock.id);

	pDock.eff.periodic.magnitude = 0x1999;

	if ( SDL_HapticUpdateEffect(haptic, pDock.id, &pDock.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pDock:\n  %s\n", SDL_GetError()));
	}

	joy_ff_start_effect(&pDock, "Dock (Reload)");
}

void joy_ff_afterburn_on()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if (Joy_ff_afterburning) {
		return;
	}

	if ( !(pAfterburn1.loaded && pAfterburn2.loaded) ) {
		return;
	}

	SDL_HapticStopEffect(haptic, pAfterburn1.id);

	pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn1.eff.periodic.period = 20;
	pAfterburn1.eff.periodic.magnitude = 0x3332;
	pAfterburn1.eff.periodic.attack_length = 0;

	if ( SDL_HapticUpdateEffect(haptic, pAfterburn1.id, &pAfterburn1.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pAfterburn1:\n  %s\n", SDL_GetError()));
	}

	SDL_HapticStopEffect(haptic, pAfterburn2.id);

	pAfterburn2.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn2.eff.periodic.period = 100;
	pAfterburn2.eff.periodic.magnitude = 0x1999;
	pAfterburn2.eff.periodic.attack_length = 0;

	if ( SDL_HapticUpdateEffect(haptic, pAfterburn2.id, &pAfterburn2.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pAfterburn2:\n  %s\n", SDL_GetError()));
	}

	joy_ff_start_effect(&pAfterburn1, "Afterburn1");
	joy_ff_start_effect(&pAfterburn2, "Afterburn2");

//	nprintf(("Joystick", "FF: Afterburn started\n"));

	Joy_ff_afterburning = 1;
}

void joy_ff_afterburn_off()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if ( !Joy_ff_afterburning ) {
		return;
	}

	if (pAfterburn1.loaded) {
		SDL_HapticStopEffect(haptic, pAfterburn1.id);
	}

	if (pAfterburn2.loaded) {
		SDL_HapticStopEffect(haptic, pAfterburn2.id);
	}

	Joy_ff_afterburning = 0;

//	nprintf(("Joystick", "FF: Afterburn stopped\n"));
}

void joy_ff_explode()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}
/*
	if (pDeathroll1.loaded) {
		SDL_HapticStopEffect(haptic, pDeathroll1.id);
	}

	if (pDeathroll2.loaded) {
		SDL_HapticStopEffect(haptic, pDeathroll2.id);
	}

	if (pExplode.loaded) {
		SDL_HapticStopEffect(haptic, pExplode.id);
	}

	joy_ff_start_effect(&pExplode, "Explode");
*/

	if (pAfterburn1.loaded) {
		SDL_HapticStopEffect(haptic, pAfterburn1.id);
	}

	if (pAfterburn2.loaded) {
		SDL_HapticStopEffect(haptic, pAfterburn2.id);
	}

	if (pShootEffect.loaded) {
		SDL_HapticStopEffect(haptic, pShootEffect.id);

		pShootEffect.eff.periodic.direction.dir[0] = 9000;
		pShootEffect.eff.periodic.length = 500;
		pShootEffect.eff.periodic.magnitude = 0x7FFF;
		pShootEffect.eff.periodic.fade_length = 500;

		if ( SDL_HapticUpdateEffect(haptic, pShootEffect.id, &pShootEffect.eff) < 0 ) {
			mprintf(("HapticERROR:  Unable to update pShootEffect:\n  %s\n", SDL_GetError()));
		}

		joy_ff_start_effect(&pShootEffect, "ShootEffect (Explode)");
	} else if (Joy_rumble) {
		SDL_HapticRumblePlay(haptic, 0.75f, 500);
	}

	Joy_ff_afterburning = 0;
}

void joy_ff_fly_by(int mag)
{
	int gain;

	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}

	if (Joy_ff_afterburning) {
		return;
	}

	if ( !(pAfterburn1.loaded && pAfterburn2.loaded) ) {
		return;
	}

	gain = mag * 120 + 4000;

	if (gain < 1) {
		gain = 1;
	} else if (gain > 10000) {
		gain = 10000;
	}

	SDL_HapticStopEffect(haptic, pAfterburn1.id);

	pAfterburn1.eff.periodic.length = (6000 * mag + 400000) / 1000;
	pAfterburn1.eff.periodic.period = 20;
	pAfterburn1.eff.periodic.magnitude = (Sint16)(32767.0f * (gain / 10000.0f));
	pAfterburn1.eff.periodic.attack_length = 0;

	if ( SDL_HapticUpdateEffect(haptic, pAfterburn1.id, &pAfterburn1.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pAfterburn1:\n  %s\n", SDL_GetError()));
	}


	SDL_HapticStopEffect(haptic, pAfterburn2.id);

	pAfterburn2.eff.periodic.length = (6000 * mag + 400000) / 1000;
	pAfterburn2.eff.periodic.period = 100;
	pAfterburn2.eff.periodic.magnitude = (Sint16)(32767.0f * (gain / 10000.0f));
	pAfterburn2.eff.periodic.attack_length = 0;

	if ( SDL_HapticUpdateEffect(haptic, pAfterburn2.id, &pAfterburn2.eff) < 0 ) {
		mprintf(("HapticERROR:  Unable to update pAfterburn2:\n  %s\n", SDL_GetError()));
	}

	joy_ff_start_effect(&pAfterburn1, "Afterburn1 (Fly by)");
	joy_ff_start_effect(&pAfterburn2, "Afterburn2 (Fly by)");
}

void joy_ff_deathroll()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if ( !Joy_ff_acquired ) {
		return;
	}
/*
	if (pDeathroll1.loaded) {
		SDL_HapticStopEffect(haptic, pDeathroll1.id);
	}

	if (pDeathroll2.loaded) {
		SDL_HapticStopEffect(haptic, pDeathroll2.id);
	}

	joy_ff_start_effect(&pDeathroll1, "Deathroll1");
	joy_ff_start_effect(&pDeathroll2, "Deathroll2");
*/

	if (pAfterburn1.loaded && pAfterburn2.loaded) {
		SDL_HapticStopEffect(haptic, pAfterburn1.id);

		pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn1.eff.periodic.period = 200;
		pAfterburn1.eff.periodic.magnitude = 0x7FFF;
		pAfterburn1.eff.periodic.attack_length = 200;

		if ( SDL_HapticUpdateEffect(haptic, pAfterburn1.id, &pAfterburn1.eff) < 0 ) {
			mprintf(("HapticERROR:  Unable to update pAfterburn1:\n  %s\n", SDL_GetError()));
		}

		SDL_HapticStopEffect(haptic, pAfterburn2.id);

		pAfterburn2.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn2.eff.periodic.period = 200;
		pAfterburn2.eff.periodic.magnitude = 0x7FFF;
		pAfterburn2.eff.periodic.attack_length = 200;

		if ( SDL_HapticUpdateEffect(haptic, pAfterburn2.id, &pAfterburn2.eff) < 0 ) {
			mprintf(("HapticERROR:  Unable to update pAfterburn2:\n  %s\n", SDL_GetError()));
		}

		joy_ff_start_effect(&pAfterburn1, "Afterburn1 (Death Roll)");
		joy_ff_start_effect(&pAfterburn2, "Afterburn2 (Death Roll)");

		Joy_ff_afterburning = 1;
	}
}
