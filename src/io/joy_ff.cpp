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
#include "timer.h"
#include "joy.h"


static int Joy_ff_enabled = 0;
static int Joy_ff_acquired = 0;
static SDL_Haptic *haptic = nullptr;
static SDL_Gamepad *gamepad = nullptr;
static int joy_ff_handling_scaler = 0;
static uint Joy_ff_directional_hit_effect_enabled = 1;
static int Joy_ff_afterburning = 0;

typedef struct haptic_effect_t {
	SDL_HapticEffect eff;
	int id;
	bool loaded;

	haptic_effect_t() : id(-1), loaded(false) {}
} haptic_effect_t;

static haptic_effect_t pHitEffect1;
static haptic_effect_t pHitEffect2;
static haptic_effect_t pAfterburn1;
static haptic_effect_t pAfterburn2;
static haptic_effect_t pShootEffect;
static haptic_effect_t pSecShootEffect;
static haptic_effect_t pSpring;
static haptic_effect_t pDock;

static void joy_ff_create_effects();
//static bool joy_ff_effect_playing(haptic_effect_t *eff);
static void joy_ff_start_effect(haptic_effect_t *eff, const char *name);


int joy_ff_init()
{
	if ( !os_config_read_uint("Controls", "EnableJoystickFF", 0) ) {
		return 0;
	}

	if (joystick_is_gamepad()) {
		gamepad = SDL_GetGamepadFromID(joystick_get_id());

		if ( !gamepad ) {
			mprintf(("  ERROR: Unable to get gamepad\n"));
			return -1;
		}

		auto props = SDL_GetGamepadProperties(gamepad);

		if ( !SDL_GetBooleanProperty(props, SDL_PROP_GAMEPAD_CAP_RUMBLE_BOOLEAN, false) ) {
			gamepad = nullptr;
			return 0;
		}
	} else {
		SDL_Joystick *sdljoy = SDL_GetJoystickFromID(joystick_get_id());

		if ( !SDL_IsJoystickHaptic(sdljoy) ) {
			return 0;
		}

		if ( !SDL_InitSubSystem(SDL_INIT_HAPTIC) ) {
			mprintf(("  ERROR: Unable to initialize haptic subsystem\n"));
			return -1;
		}

		haptic = SDL_OpenHapticFromJoystick(sdljoy);

		if (haptic == nullptr) {
			mprintf(("  ERROR: Unable to open haptic joystick\n"));
			SDL_QuitSubSystem(SDL_INIT_HAPTIC);
			return -1;
		}

		mprintf(("  Haptic Axes  : %d\n", SDL_GetNumHapticAxes(haptic)));
		mprintf(("  Max effects     : %d\n", SDL_GetMaxHapticEffects(haptic)));
		mprintf(("  Running effects : %d\n", SDL_GetMaxHapticEffectsPlaying(haptic)));
		
		joy_ff_create_effects();
	}

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

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0, 0, 0);
		gamepad = nullptr;
	} else {
		SDL_StopHapticEffects(haptic);
		SDL_CloseHaptic(haptic);
		haptic = nullptr;

		SDL_QuitSubSystem(SDL_INIT_HAPTIC);
	}

	Joy_ff_acquired = 0;
	Joy_ff_enabled = 0;
}

static void joy_ff_create_effects()
{
	// clear all SDL errors
	SDL_ClearError();

	if (joystick_is_gamepad()) {
		return;
	}

	unsigned int supported = 0;

	supported = SDL_GetHapticFeatures(haptic);

	if ( !(supported & SDL_HAPTIC_CONSTANT) ) {
		mprintf(("  Constant Force  : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SINE) ) {
		mprintf(("  Sine Wave       : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SAWTOOTHDOWN) ) {
		mprintf(("  Sawtooth Down   : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SPRING) ) {
		mprintf(("  Spring          : not supported\n"));
	}

	if ( !(supported & SDL_HAPTIC_SQUARE) ) {
		mprintf(("  Square          : not supported\n"));
	}


	if (supported & SDL_HAPTIC_SPRING) {
		// pSpring
		memset(&pSpring, 0, sizeof(haptic_effect_t));

		pSpring.eff.type = SDL_HAPTIC_SPRING;
		pSpring.eff.condition.type = SDL_HAPTIC_SPRING;
		pSpring.eff.condition.length = SDL_HAPTIC_INFINITY;

		for (int i = 0; i < 3; i++) {
			pSpring.eff.condition.right_sat[i] = 0x7FFF;
			pSpring.eff.condition.left_sat[i] = 0x7FFF;
			pSpring.eff.condition.right_coeff[i] = 0x147;
			pSpring.eff.condition.left_coeff[i] = 0x147;
		}

		pSpring.id = SDL_CreateHapticEffect(haptic, &pSpring.eff);

		if (pSpring.id < 0) {
			mprintf(("    Spring effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pSpring.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_SAWTOOTHDOWN) {
		// pShootEffect
		memset(&pShootEffect, 0, sizeof(haptic_effect_t));

		pShootEffect.eff.type = SDL_HAPTIC_SAWTOOTHDOWN;
		pShootEffect.eff.periodic.type = SDL_HAPTIC_SAWTOOTHDOWN;
		pShootEffect.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pShootEffect.eff.periodic.direction.dir[0] = 0;
		pShootEffect.eff.periodic.length = 160;
		pShootEffect.eff.periodic.period = 20;
		pShootEffect.eff.periodic.magnitude = 0x7FFF;
		pShootEffect.eff.periodic.fade_length = 120;

		pShootEffect.id = SDL_CreateHapticEffect(haptic, &pShootEffect.eff);

		if (pShootEffect.id < 0) {
			mprintf(("    Fire primary effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pShootEffect.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_CONSTANT) {
		// pSecShootEffect
		memset(&pSecShootEffect, 0, sizeof(haptic_effect_t));

		pSecShootEffect.eff.type = SDL_HAPTIC_CONSTANT;
		pSecShootEffect.eff.constant.type = SDL_HAPTIC_CONSTANT;
		pSecShootEffect.eff.constant.direction.type = SDL_HAPTIC_POLAR;
		pSecShootEffect.eff.constant.direction.dir[0] = 0;
		pSecShootEffect.eff.constant.length = 200;
		pSecShootEffect.eff.constant.level = 0x7FFF;
		pSecShootEffect.eff.constant.attack_length = 50;
		pSecShootEffect.eff.constant.attack_level = 0x7FFF;
		pSecShootEffect.eff.constant.fade_length = 100;
		pSecShootEffect.eff.constant.fade_level = 1;

		pSecShootEffect.id = SDL_CreateHapticEffect(haptic, &pSecShootEffect.eff);

		if (pSecShootEffect.id < 0) {
			mprintf(("    Fire secondary effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pSecShootEffect.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_SINE) {
		// pAfterburn1
		memset(&pAfterburn1, 0, sizeof(haptic_effect_t));

		pAfterburn1.eff.type = SDL_HAPTIC_SINE;
		pAfterburn1.eff.periodic.type = SDL_HAPTIC_SINE;
		pAfterburn1.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pAfterburn1.eff.periodic.direction.dir[0] = 0;
		pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn1.eff.periodic.period = 20;
		pAfterburn1.eff.periodic.magnitude = 0x3332;

		pAfterburn1.id = SDL_CreateHapticEffect(haptic, &pAfterburn1.eff);

		if (pAfterburn1.id < 0) {
			mprintf(("    Afterburn effect 1 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pAfterburn1.loaded = true;
		}

		// pAfterburn2
		memset(&pAfterburn2, 0, sizeof(haptic_effect_t));

		pAfterburn2.eff.type = SDL_HAPTIC_SINE;
		pAfterburn2.eff.periodic.type = SDL_HAPTIC_SINE;
		pAfterburn2.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pAfterburn2.eff.periodic.direction.dir[0] = 9000;
		pAfterburn2.eff.periodic.length = 125;
		pAfterburn2.eff.periodic.period = 100;
		pAfterburn2.eff.periodic.magnitude = 0x1999;

		pAfterburn2.id = SDL_CreateHapticEffect(haptic, &pAfterburn2.eff);

		if (pAfterburn2.id < 0) {
			mprintf(("    Afterburn effect 2 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pAfterburn2.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_CONSTANT) {
		// pHitEffect1
		memset(&pHitEffect1, 0, sizeof(haptic_effect_t));

		pHitEffect1.eff.type = SDL_HAPTIC_CONSTANT;
		pHitEffect1.eff.constant.type = SDL_HAPTIC_CONSTANT;
		pHitEffect1.eff.constant.direction.type = SDL_HAPTIC_POLAR;
		pHitEffect1.eff.constant.direction.dir[0] = 0;
		pHitEffect1.eff.constant.length = 300;
		pHitEffect1.eff.constant.level = 0x7FFF;
		pHitEffect1.eff.constant.attack_length = 0;
		pHitEffect1.eff.constant.attack_level = 0x7FFF;
		pHitEffect1.eff.constant.fade_length = 120;
		pHitEffect1.eff.constant.fade_level = 1;

		pHitEffect1.id = SDL_CreateHapticEffect(haptic, &pHitEffect1.eff);

		if (pHitEffect1.id < 0) {
			mprintf(("    Hit effect 1 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pHitEffect1.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_SINE) {
		// pHitEffect2
		memset(&pHitEffect2, 0, sizeof(haptic_effect_t));

		pHitEffect2.eff.type = SDL_HAPTIC_SINE;
		pHitEffect2.eff.periodic.type = SDL_HAPTIC_SINE;
		pHitEffect2.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pHitEffect2.eff.periodic.direction.dir[0] = 9000;
		pHitEffect2.eff.periodic.length = 300;
		pHitEffect2.eff.periodic.period = 100;
		pHitEffect2.eff.periodic.magnitude = 0x7FFF;
		pHitEffect2.eff.periodic.attack_length = 100;
		pHitEffect2.eff.periodic.fade_length = 100;

		pHitEffect2.id = SDL_CreateHapticEffect(haptic, &pHitEffect2.eff);

		if (pHitEffect2.id < 0) {
			mprintf(("    Hit effect 2 failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pHitEffect2.loaded = true;
		}
	}

	if (supported & SDL_HAPTIC_SQUARE) {
		// pDock
		memset(&pDock, 0, sizeof(haptic_effect_t));

		pDock.eff.type = SDL_HAPTIC_SQUARE;
		pDock.eff.periodic.type = SDL_HAPTIC_SQUARE;
		pDock.eff.periodic.direction.type = SDL_HAPTIC_POLAR;
		pDock.eff.periodic.direction.dir[0] = 9000;
		pDock.eff.periodic.length = 125;
		pDock.eff.periodic.period = 100;
		pDock.eff.periodic.magnitude = 0x3332;

		pDock.id = SDL_CreateHapticEffect(haptic, &pDock.eff);

		if (pDock.id < 0) {
			mprintf(("    Dock effect failed to load:\n      %s\n", SDL_GetError()));
		} else {
			pDock.loaded = true;
		}
	}
}

static bool joy_ff_can_play()
{
	return (Joy_ff_enabled && Joy_ff_acquired);
}

static void joy_ff_update_effect(haptic_effect_t *eff, const char *name)
{
	if ( !eff->loaded ) {
		return;
	}

	if ( !SDL_UpdateHapticEffect(haptic, eff->id, &eff->eff) ) {
		mprintf(("HapticERROR:  Unable to update %s:\n  %s\n", name, SDL_GetError()));
	}
}

static void joy_ff_start_effect(haptic_effect_t *eff, const char *name)
{
	if ( !eff->loaded ) {
		return;
	}

//	nprintf(("Joystick", "FF: Starting effect %s\n", name));

	if ( SDL_RunHapticEffect(haptic, eff->id, 1) ) {
		mprintf(("HapticERROR:  Unable to run %s:\n  %s\n", name, SDL_GetError()));
	}
}

void joy_ff_stop_effects()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0, 0, 0);
	} else {
		SDL_StopHapticEffects(haptic);
	}
}

void joy_ff_mission_init(vector v)
{
	v.xyz.z = 0.0f;

	joy_ff_handling_scaler = (int) ((vm_vec_mag(&v) + 1.3f) * 5.0f);

	joy_ff_adjust_handling(0);
	joy_ff_start_effect(&pSpring, "Spring");

	Joy_ff_afterburning = 0;

	// reset afterburn effects to default values

	pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn1.eff.periodic.period = 20;
	pAfterburn1.eff.periodic.magnitude = 0x3332;
	pAfterburn1.eff.periodic.attack_length = 0;

	joy_ff_update_effect(&pAfterburn1, "pAfterburn1 (init)");

	pAfterburn2.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn2.eff.periodic.period = 100;
	pAfterburn2.eff.periodic.magnitude = 0x1999;
	pAfterburn2.eff.periodic.attack_length = 0;

	joy_ff_update_effect(&pAfterburn2, "pAfterburn2 (init)");

	// reset primary shoot effect to default values

	pShootEffect.eff.periodic.direction.dir[0] = 0;
	pShootEffect.eff.periodic.length = 160;
	pShootEffect.eff.periodic.fade_length = 120;

	joy_ff_update_effect(&pShootEffect, "pShootEffect (init)");
}

void joy_reacquire_ff()
{
	if ( !Joy_ff_enabled ) {
		return;
	}

	if (Joy_ff_acquired) {
		return;
	}

	if (joystick_is_gamepad()) {
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

	if (joystick_is_gamepad()) {
		return;
	}

	joy_ff_stop_effects();

	Joy_ff_acquired = 0;
}

void joy_ff_play_vector_effect(vector *v, float scaler)
{
	vector vf;
	float x, y;

	if ( !joy_ff_can_play() ) {
		return;
	}

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
	static int hit_timeout = 1;
	int idegs;
	float degs, mag;
	Sint16 imag;

	if ( !joy_ff_can_play() ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0x2fff, 0x4fff, 300);
		return;
	}

	// allow for at least one of the effects to work
	if ( !pHitEffect1.loaded && !pHitEffect2.loaded ) {
		return;
	}

	if ( !timestamp_elapsed(hit_timeout) ) {
		nprintf(("Joystick", "FF: HitEffect already playing.  Skipping\n"));
		return;
	}

	hit_timeout = timestamp(pHitEffect1.eff.condition.length);

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

		mag = fl_sqrt(x * x + y * y) / 10000.0f;
		CAP(mag, 0.0f, 1.0f);

		imag = (Sint16)(32767.0f * mag);

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
			pHitEffect1.eff.constant.level = imag;

			joy_ff_update_effect(&pHitEffect1, "pHitEffect1");
		}

		idegs += 9000;
		if (idegs >= 36000)
			idegs -= 36000;

		if (pHitEffect2.loaded) {
			pHitEffect2.eff.periodic.direction.dir[0] = idegs;
			pHitEffect2.eff.periodic.magnitude = imag;

			joy_ff_update_effect(&pHitEffect2, "pHitEffect2");
		}
	}

	joy_ff_start_effect(&pHitEffect1, "HitEffect1");
	joy_ff_start_effect(&pHitEffect2, "HitEffect2");
}

void joy_ff_play_primary_shoot(int gain)
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	CAP(gain, 1, 10000);

	if (joystick_is_gamepad()) {
		ushort freq = 0x7fff * fl2i(gain / 10000.0f);
		SDL_RumbleGamepad(gamepad, freq, freq/2, 100);

		return;
	}

	if ( !pShootEffect.loaded ) {
		return;
	}

	if (pShootEffect.loaded) {
		SDL_StopHapticEffect(haptic, pShootEffect.id);

		static int primary_ff_level = 10000;

		if (gain != primary_ff_level) {
			pShootEffect.eff.periodic.magnitude = (Sint16)(32767.0f * (gain / 10000.0f));

			joy_ff_update_effect(&pShootEffect, "pShootEffect");

			primary_ff_level = gain;
		}

		joy_ff_start_effect(&pShootEffect, "ShootEffect");
	}
}

void joy_ff_play_secondary_shoot(int gain)
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	gain = gain * 100 + 2500;

	CAP(gain, 1, 10000);

	if (joystick_is_gamepad()) {
		ushort freq = 0xffff * fl2i(gain / 10000.0f);
		int duration = (150000 + gain * 25) / 1000;
		SDL_RumbleGamepad(gamepad, freq, freq/2, duration);

		return;
	}

	if ( !pSecShootEffect.loaded ) {
		return;
	}

	if (pSecShootEffect.loaded) {
		SDL_StopHapticEffect(haptic, pSecShootEffect.id);

		static int secondary_ff_level = 10000;

		if (gain != secondary_ff_level) {
			pSecShootEffect.eff.constant.level = (Sint16)(32767.0f * (gain / 10000.0f));
			pSecShootEffect.eff.constant.length = (150000 + gain * 25) / 1000;

			joy_ff_update_effect(&pSecShootEffect, "pSecShootEffect");

			secondary_ff_level = gain;
			nprintf(("Joystick", "FF: Secondary force = 0x%04x\n", pSecShootEffect.eff.constant.level));
		}

		joy_ff_start_effect(&pSecShootEffect, "SecShootEffect");
	}
}

void joy_ff_adjust_handling(int speed)
{
	int v;
	short coeff = 0;

	if ( !joy_ff_can_play() || joystick_is_gamepad() ) {
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

	CAP(v, 0, 10000);

	coeff = (short)(32767.0f * (v / 10000.0f));

	for (int i = 0; i < SDL_GetNumHapticAxes(haptic); i++) {
		pSpring.eff.condition.right_coeff[i] = coeff;
		pSpring.eff.condition.left_coeff[i] = coeff;
	}

//	nprintf(("Joystick", "FF: New handling force = 0x%04x\n", coeff));

	joy_ff_update_effect(&pSpring, "pSpring");
}
/*
static bool joy_ff_effect_playing(haptic_effect_t *eff)
{
	if ( !eff->loaded ) {
		return false;
	} else {
		return SDL_HapticGetEffectStatus(haptic, eff->id);
	}
}
*/
void joy_ff_docked()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0xffff, 0, 150);
		return;
	}

	if ( !pDock.loaded ) {
		return;
	}

	SDL_StopHapticEffect(haptic, pDock.id);

	pDock.eff.periodic.magnitude = 0x3332;

	joy_ff_update_effect(&pDock, "pDock");

	joy_ff_start_effect(&pDock, "Dock");
}

void joy_ff_play_reload_effect()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0x1fff, 0x7fff, 50);
		return;
	}

	if ( !pDock.loaded ) {
		return;
	}

	SDL_StopHapticEffect(haptic, pDock.id);

	pDock.eff.periodic.magnitude = 0x1999;

	joy_ff_update_effect(&pDock, "pDock (reload)");

	joy_ff_start_effect(&pDock, "Dock (Reload)");
}

void joy_ff_afterburn_on()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if (Joy_ff_afterburning) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0x7fff, 0, 1500);
		return;
	}

	if ( !(pAfterburn1.loaded && pAfterburn2.loaded) ) {
		return;
	}

	SDL_StopHapticEffect(haptic, pAfterburn1.id);

	pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn1.eff.periodic.magnitude = 0x3332;

	joy_ff_update_effect(&pAfterburn1, "pAfterburn1");

	SDL_StopHapticEffect(haptic, pAfterburn2.id);

	pAfterburn2.eff.periodic.length = SDL_HAPTIC_INFINITY;
	pAfterburn2.eff.periodic.magnitude = 0x1999;

	joy_ff_update_effect(&pAfterburn2, "pAfterburn2");

	joy_ff_start_effect(&pAfterburn1, "Afterburn1");
	joy_ff_start_effect(&pAfterburn2, "Afterburn2");

//	nprintf(("Joystick", "FF: Afterburn started\n"));

	Joy_ff_afterburning = 1;
}

void joy_ff_afterburn_off()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if ( !Joy_ff_afterburning ) {
		return;
	}

	Joy_ff_afterburning = 0;

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0, 0, 0);
		return;
	}

	if (pAfterburn1.loaded) {
		SDL_StopHapticEffect(haptic, pAfterburn1.id);
	}

	if (pAfterburn2.loaded) {
		SDL_StopHapticEffect(haptic, pAfterburn2.id);
	}

//	nprintf(("Joystick", "FF: Afterburn stopped\n"));
}

void joy_ff_explode()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0xffff, 0x7fff, 500);
		return;
	}

	if (pAfterburn1.loaded) {
		SDL_StopHapticEffect(haptic, pAfterburn1.id);
	}

	if (pAfterburn2.loaded) {
		SDL_StopHapticEffect(haptic, pAfterburn2.id);
	}

	if (pShootEffect.loaded) {
		SDL_StopHapticEffect(haptic, pShootEffect.id);

		pShootEffect.eff.periodic.direction.dir[0] = 9000;
		pShootEffect.eff.periodic.length = 500;
		pShootEffect.eff.periodic.magnitude = 0x7FFF;
		pShootEffect.eff.periodic.fade_length = 500;

		joy_ff_update_effect(&pShootEffect, "pShootEffect (explode)");

		joy_ff_start_effect(&pShootEffect, "ShootEffect (Explode)");
	}

	Joy_ff_afterburning = 0;
}

void joy_ff_fly_by(int mag)
{
	int gain;

	if ( !joy_ff_can_play() ) {
		return;
	}

	if (Joy_ff_afterburning) {
		return;
	}

	gain = mag * 120 + 4000;

	CAP(gain, 1, 10000);

	if (joystick_is_gamepad()) {
		int duration = (6000 * mag + 400000) / 1000;
		SDL_RumbleGamepad(gamepad, 0x3fff, 0x7fff, duration);
		return;
	}

	if ( !(pAfterburn1.loaded && pAfterburn2.loaded) ) {
		return;
	}

	SDL_StopHapticEffect(haptic, pAfterburn1.id);

	pAfterburn1.eff.periodic.length = (6000 * mag + 400000) / 1000;
	pAfterburn1.eff.periodic.magnitude = (Sint16)(26212.0f * (gain / 10000.0f));

	joy_ff_update_effect(&pAfterburn1, "pAfterburn1 (flyby)");

	SDL_StopHapticEffect(haptic, pAfterburn2.id);

	pAfterburn2.eff.periodic.length = (6000 * mag + 400000) / 1000;
	pAfterburn2.eff.periodic.magnitude = (Sint16)(13106.0f * (gain / 10000.0f));

	joy_ff_update_effect(&pAfterburn2, "pAfterburn2 (flyby)");

	joy_ff_start_effect(&pAfterburn1, "Afterburn1 (Fly by)");
	joy_ff_start_effect(&pAfterburn2, "Afterburn2 (Fly by)");
}

void joy_ff_deathroll()
{
	if ( !joy_ff_can_play() ) {
		return;
	}

	if (joystick_is_gamepad()) {
		SDL_RumbleGamepad(gamepad, 0x4fff, 0x1fff, 4000);
		return;
	}

	if (pAfterburn1.loaded && pAfterburn2.loaded) {
		SDL_StopHapticEffect(haptic, pAfterburn1.id);

		pAfterburn1.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn1.eff.periodic.period = 200;
		pAfterburn1.eff.periodic.magnitude = 0x7FFF;
		pAfterburn1.eff.periodic.attack_length = 200;

		joy_ff_update_effect(&pAfterburn1, "pAfterburn1 (deathroll)");

		SDL_StopHapticEffect(haptic, pAfterburn2.id);

		pAfterburn2.eff.periodic.length = SDL_HAPTIC_INFINITY;
		pAfterburn2.eff.periodic.period = 200;
		pAfterburn2.eff.periodic.magnitude = 0x7FFF;
		pAfterburn2.eff.periodic.attack_length = 200;

		joy_ff_update_effect(&pAfterburn2, "pAfterburn2 (deathroll)");

		joy_ff_start_effect(&pAfterburn1, "Afterburn1 (Death Roll)");
		joy_ff_start_effect(&pAfterburn2, "Afterburn2 (Death Roll)");

		Joy_ff_afterburning = 1;
	}
}
