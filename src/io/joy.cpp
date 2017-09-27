/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pstypes.h"
#include "joy.h"
#include "fix.h"
#include "key.h"
#include "timer.h"
#include "osregistry.h"
#include "joy_ff.h"
#include "osapi.h"


static int Joy_inited = 0;
int Dead_zone_size = 10;
int Joy_sensitivity = 9;

static int Joy_last_x_reading = 0;
static int Joy_last_y_reading = 0;

typedef struct joy_button_info {
	int     actual_state;           // Set if the button is physically down
	int     state;                          // Set when the button goes from up to down, cleared on down to up.  Different than actual_state after a flush.
	int     down_count;
	int     up_count;
	int     down_time;
	uint    last_down_check;        // timestamp in milliseconds of last
} joy_button_info;

static Joy_info joystick;

SDL_Joystick *sdljoy;
static SDL_JoystickID joy_id = -1;

joy_button_info joy_buttons[JOY_TOTAL_BUTTONS];


int joystick_get_id()
{
	return joy_id;
}

void joy_close()
{
	if (!Joy_inited)
		return;

	joy_ff_shutdown();

	Joy_inited = 0;
	joy_id = -1;

	if (SDL_JoystickGetAttached(sdljoy)) {
		SDL_JoystickClose(sdljoy);
	}

	sdljoy = NULL;

	SDL_QuitSubSystem(SDL_INIT_JOYSTICK);
}

int joy_down(int btn)
{
	int tmp;

	if ( !Joy_inited ) {
		return 0;
	}

	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) {
		return 0;
	}

	tmp = joy_buttons[btn].state;

	return tmp;
}

int joy_down_count(int btn, int reset_count)
{
	int tmp;

	if ( !Joy_inited ) {
		return 0;
	}

	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) {
		return 0;
	}

	tmp = joy_buttons[btn].down_count;
	if ( reset_count ) {
		joy_buttons[btn].down_count = 0;
	}

	return tmp;
}

float joy_down_time(int btn)
{
	float                           rval;
	unsigned int    now, delta;
	joy_button_info         *bi;

	if ( !Joy_inited ) {
		return 0.0f;
	}

	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) {
		return 0.0f;
	}

	bi = &joy_buttons[btn];

	now = timer_get_milliseconds();
	delta = now - bi->last_down_check;

	if ( (now - bi->last_down_check) > 0)
		rval = i2fl((now - bi->down_time)) / delta;
	else
		rval = 0.0f;

	bi->down_time = 0;
	bi->last_down_check = now;

	if (rval < 0)
		rval = 0.0f;
	if (rval > 1)
		rval = 1.0f;

	return rval;
}

void joy_mark_button(int btn, int state)
{
	int i;
	joy_button_info *bi;

	if ( !Joy_inited ) {
		return;
	}

	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) {
		return;
	}

	bi = &joy_buttons[btn];

	if (state) {
		// button pressed
		bi->down_count++;

		bi->state = 1;
		bi->down_time = timer_get_milliseconds();

		// toggle off other positions if hat
		if (btn >= JOY_HATBACK) {
			for (i = JOY_HATBACK; i < (JOY_HATBACK+JOY_NUM_HAT_POS); i++) {
				if (btn != i) {
					joy_buttons[i].state = 0;
				}
			}
		}
	} else {
		// button released
	//	bi->up_count++;

		bi->state = 0;

		// special hat handling - make sure all hat pos are off
		if (btn == JOY_HATBACK) {
			for (i = JOY_HATBACK; i < (JOY_HATBACK+JOY_NUM_HAT_POS); i++) {
				joy_buttons[i].state = 0;
			}
		}
	}
}

void joy_flush()
{
	int                     i;
	joy_button_info *bi;

	if ( !Joy_inited ) {
		return;
	}

	for ( i = 0; i < JOY_TOTAL_BUTTONS; i++) {
		bi = &joy_buttons[i];
		bi->state               = 0;
		bi->down_count  = 0;
		bi->up_count    = 0;
		bi->down_time   = 0;
		bi->last_down_check = timer_get_milliseconds();
	}
}

int joy_get_unscaled_reading(int axn)
{
	int rng;

	if ( !Joy_inited ) {
		return 0;
	}

	if (axn >= joystick.num_axes) {
		return 0;
	}

	int raw = joystick.axis_current[axn];

	rng = joystick.axis_max[axn] - joystick.axis_min[axn];
	raw -= joystick.axis_min[axn];  // adjust for linear range starting at 0

	// cap at limits
	if (raw < 0)
		raw = 0;
	if (raw > rng)
		raw = rng;

	return (int) ((unsigned int) raw * (unsigned int) F1_0 / (unsigned int) rng);  // convert to 0 - F1_0 range.
}

// --------------------------------------------------------------
//	joy_get_scaled_reading()
//
//	input:	raw	=>	the raw value for an axis position
//				axn	=>	axis number, numbered starting at 0
//
// return:	joy_get_scaled_reading will return a value that represents
//				the joystick pos from -1 to +1 for the specified axis number 'axn', and
//				the raw value 'raw'
//
int joy_get_scaled_reading(int axn)
{
	int x, d, dead_zone, rng, raw;
	float percent, sensitivity_percent, non_sensitivity_percent;

	if ( !Joy_inited ) {
		return 0;
	}

	if (axn >= joystick.num_axes) {
		return 0;
	}

	raw = joystick.axis_current[axn] - joystick.axis_center[axn];

	dead_zone = (joystick.axis_max[axn] - joystick.axis_min[axn]) * Dead_zone_size / 100;

	if (raw < -dead_zone) {
		rng = joystick.axis_center[axn] - joystick.axis_min[axn] - dead_zone;
		d = -raw - dead_zone;

	} else if (raw > dead_zone) {
		rng = joystick.axis_max[axn] - joystick.axis_center[axn] - dead_zone;
		d = raw - dead_zone;

	} else {
		return 0;
	}

	if (d > rng)
		d = rng;

	SDL_assert(Joy_sensitivity >= 0 && Joy_sensitivity <= 9);

	// compute percentages as a range between 0 and 1
	sensitivity_percent = (float) Joy_sensitivity / 9.0f;
	non_sensitivity_percent = (float) (9 - Joy_sensitivity) / 9.0f;

	// find percent of max axis is at
	percent = (float) d / (float) rng;

	// work sensitivity on axis value
	percent = (percent * sensitivity_percent + percent * percent * percent * percent * percent * non_sensitivity_percent);

	x = (int) ((float) F1_0 * percent);

	//nprintf(("AI", "d=%6i, sens=%3i, percent=%6.3f, val=%6i, ratio=%6.3f\n", d, Joy_sensitivity, percent, (raw<0) ? -x : x, (float) d/x));

	if (raw < 0) {
		return -x;
	}

	return x;
}

// --------------------------------------------------------------
//	joy_get_pos()
//
//	input:	x		=>		OUTPUT PARAMETER: x-axis position of stick (-1 to 1)
//				y		=>		OUTPUT PARAMETER: y-axis position of stick (-1 to 1)
//				z		=>		OUTPUT PARAMETER: z-axis (throttle) position of stick (-1 to 1)
//				r		=>		OUTPUT PARAMETER: rudder position of stick (-1 to 1)
//
//	return:	success	=> 1
//				failure	=> 0
//
int joy_get_pos(int *x, int *y, int *z, int *rx)
{
	if (x) *x = 0;
	if (y) *y = 0;
	if (z) *z = 0;
	if (rx) *rx = 0;

	if ( !Joy_inited ) {
		return 0;
	}

	//	joy_get_scaled_reading will return a value represents the joystick
	//	pos from -1 to +1
	if (x && joystick.num_axes > 0) {
		*x = joy_get_scaled_reading(0);
		Joy_last_x_reading = *x;
	}

	if (y && joystick.num_axes > 1) {
		*y = joy_get_scaled_reading(1);
		Joy_last_y_reading = *y;
	}

	if (z && joystick.num_axes > 2) {
		*z = joy_get_unscaled_reading(2);
	}

	if (rx && joystick.num_axes > 3) {
		*rx = joy_get_scaled_reading(3);
	}

	return 1;
}

int joy_init()
{
	int i, num_sticks;
	const char *ptr = NULL;
	int Cur_joystick;

	if (Joy_inited) {
		return 0;
	}

	mprintf(("Initializing Joystick...\n"));

	if (SDL_InitSubSystem(SDL_INIT_JOYSTICK) < 0) {
		mprintf(("  Could not initialize joystick subsystem\n\n"));
		return 0;
	}

	num_sticks = SDL_NumJoysticks();

	if (num_sticks < 1) {
		mprintf(("  No joysticks found\n\n"));
		return 0;
	}

	Cur_joystick = 0;

	ptr = os_config_read_string("Controls", "CurrentJoystick", NULL);

	if ( ptr && SDL_strlen(ptr) ) {
		for (i = 0; i < num_sticks; i++) {
			const char *jname = SDL_JoystickNameForIndex(i);

			if ( jname && !SDL_strcasecmp(ptr, jname) ) {
				Cur_joystick = i;
				break;
			}
		}
	}

	sdljoy = SDL_JoystickOpen(Cur_joystick);

	if (sdljoy == NULL) {
		mprintf(("  Unable to init joystick %d (%s)\n\n", Cur_joystick, SDL_JoystickNameForIndex(Cur_joystick)));
		return 0;
	}

	mprintf(("  Name         : %s\n", SDL_JoystickName(sdljoy)));
	mprintf(("  Axes         : %d\n", SDL_JoystickNumAxes(sdljoy)));
	mprintf(("  Buttons      : %d\n", SDL_JoystickNumButtons(sdljoy)));
	mprintf(("  Hats         : %d\n", SDL_JoystickNumHats(sdljoy)));
	mprintf(("  Haptic       : %s\n", SDL_JoystickIsHaptic(sdljoy) ? "Yes" : "No"));

	joy_ff_init();

	mprintf(("\n"));

	Joy_inited = 1;

	joy_id = SDL_JoystickInstanceID(sdljoy);

	joystick.num_axes = SDL_JoystickNumAxes(sdljoy);

	joy_flush();

	// Fake a calibration
	joy_set_cen();

	for (i = 0; i < JOY_NUM_AXES; i++) {
		joystick.axis_min[i] = 0;
		joystick.axis_max[i] = 65536;
		joystick.axis_current[i] = joystick.axis_center[i];
	}

	return num_sticks;
}

void joy_set_cen()
{
	if ( !Joy_inited ) {
		return;
	}

	for (int i = 0; i < JOY_NUM_AXES; i++) {
		if (i < joystick.num_axes) {
			joystick.axis_center[i] = SDL_JoystickGetAxis(sdljoy, i) + 32768;
		} else {
			joystick.axis_center[i] = 32768;
		}
	}
}

int joystick_read_raw_axis(int num_axes, int *axis)
{
	int i;

	if ( !Joy_inited ) {
		return 0;
	}

	for (i = 0; i < num_axes; i++) {
		if (i < joystick.num_axes) {
			axis[i] = joystick.axis_current[i];
		} else {
			axis[i] = 32768;
		}
	}

	return 1;
}

bool joy_axis_valid(int axis)
{
	return (axis < joystick.num_axes);
}

void joystick_update_axis(int axis, int value)
{
	if (axis < JOY_NUM_AXES) {
		joystick.axis_current[axis] = value + 32768;
	}
}
