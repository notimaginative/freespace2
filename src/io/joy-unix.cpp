#include "pstypes.h"
#include "joy.h"
#include "fix.h"
#include "key.h"
#include "timer.h"
#include "osregistry.h"
#include "joy_ff.h"
#include "osapi.h"

static int Joy_inited = 0;
int joy_num_sticks = 0;
int Dead_zone_size = 10;
int Cur_joystick = -1;
int Joy_sensitivity = 9;

int joy_pollrate = 1000 / 18;  // poll at 18Hz

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

Joy_info joystick;

int JOYSTICKID1 = 0; 	// DDOI - temporary
static SDL_Joystick *sdljoy;

joy_button_info joy_buttons[JOY_TOTAL_BUTTONS];


void joy_close()
{
	if (!Joy_inited)
		return;

	Joy_inited = 0;
	joy_num_sticks = 0;

	SDL_QuitSubSystem (SDL_INIT_JOYSTICK);
}

void joy_get_caps (int max)
{
	SDL_Joystick *joy;
	int j;

	for (j=0; j < JOY_NUM_AXES; j++)
		joystick.axis_valid[j] = 0;

	for (j=JOYSTICKID1; j<JOYSTICKID1+max; j++) {
		joy = SDL_JoystickOpen (j);
		if (joy)
		{
			nprintf (("JOYSTICK", "Joystick #%d: %s\n", j - JOYSTICKID1 + 1, SDL_JoystickName(j)));
			if (j == Cur_joystick) {
				for (int i = 0; i < SDL_JoystickNumAxes(joy); i++)
				{
					joystick.axis_valid[i] = 1;
				}
			}
			SDL_JoystickClose (joy);
		}
	}
}

int joy_down(int btn)
{
	int tmp;

	if ( joy_num_sticks < 1 ) return 0;
	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS )) return 0;

	tmp = joy_buttons[btn].state;

	return tmp;
}

int joy_down_count(int btn, int reset_count)
{
	int tmp;

	if ( joy_num_sticks < 1 ) return 0;
	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) return 0;

	tmp = joy_buttons[btn].down_count;
	if ( reset_count ) {
		joy_buttons[btn].down_count = 0;
	}

	return tmp;
}

float joy_down_time(int btn)
{
	float                           rval;
	unsigned int    now;
	joy_button_info         *bi;

	if ( joy_num_sticks < 1 ) return 0.0f;
	if ( (btn < 0) || (btn >= JOY_TOTAL_BUTTONS)) return 0.0f;
	bi = &joy_buttons[btn];

	now = timer_get_milliseconds();

	if ( bi->down_time == 0 && joy_down(btn) ) {
		bi->down_time += joy_pollrate;
	}

	if ( (now - bi->last_down_check) > 0)
		rval = i2fl(bi->down_time) / (now - bi->last_down_check);
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

void joy_flush()
{
	int                     i;
	joy_button_info *bi;

	if ( joy_num_sticks < 1 ) return;

	for ( i = 0; i < JOY_TOTAL_BUTTONS; i++) {
		bi = &joy_buttons[i];
		bi->state               = 0;
		bi->down_count  = 0;
		bi->up_count    = 0;
		bi->down_time   = 0;
		bi->last_down_check = timer_get_milliseconds();
	}
}

int joy_get_pos(int *x, int *y, int *z, int *rx)
{
	int axis[JOY_NUM_AXES];
	
	if (x) *x = 0;
	if (y) *y = 0;
	if (z) *z = 0;
	if (rx) *rx = 0;
        
	if (joy_num_sticks < 1) return 0;

	joystick_read_raw_axis( 6, axis );

	//      joy_get_scaled_reading will return a value represents the joystick pos from -1 to +1
	if (x && joystick.axis_valid[0])
		*x = joy_get_scaled_reading(axis[0], 0);
	if (y && joystick.axis_valid[1])
		*y = joy_get_scaled_reading(axis[1], 1);
	if (z && joystick.axis_valid[2])
		*z = joy_get_unscaled_reading(axis[2], 2);
	if (rx && joystick.axis_valid[3])
		*rx = joy_get_scaled_reading(axis[3], 3);

	if (x)
		Joy_last_x_reading = *x;

	if (y)
		Joy_last_x_reading = *y;

	return 1;
}

int joy_get_scaled_reading(int raw, int axn)
{
	STUB_FUNCTION;
	
	return 0;
}

int joy_get_unscaled_reading(int raw, int axn)
{
	STUB_FUNCTION;
	
	return 0;
}

int joy_init()
{
	int i, n, count;

	if (Joy_inited)
		return 0;

	if (SDL_InitSubSystem (SDL_INIT_JOYSTICK)<0)
	{
		mprintf(("Could not initialize joystick\n"));
		return 0;
	}

	Joy_inited = 1;
	n = SDL_NumJoysticks ();

	// DDOI - FIXME
	//Cur_joystick = os_config_read_unit (NULL, "CurrentJoystick", JOYSTICKID1);
	Cur_joystick = 0;

	joy_get_caps(n);

	if (n < 1) {
		mprintf(("No joystick driver detected\n"));
		return 0;
	}

	joy_flush ();

	joy_num_sticks = n;

	// Fake a calibration
	if (joy_num_sticks > 0) {
		joy_set_cen();
		for (i=0; i<4; i++) {
			joystick.axis_min[i] = 0;
			joystick.axis_max[i] = joystick.axis_center[i]*2;
		}
	}

	return joy_num_sticks;
}

void joy_set_cen()
{
	joystick_read_raw_axis( 2, joystick.axis_center );
}

int joystick_read_raw_axis(int num_axes, int *axis)
{
	return 0;
}

void joy_ff_adjust_handling(int speed)
{
	STUB_FUNCTION;
}

void joy_ff_afterburn_off()
{
	STUB_FUNCTION;
}

void joy_ff_afterburn_on()
{
	STUB_FUNCTION;
}

void joy_ff_deathroll()
{
	STUB_FUNCTION;
}

void joy_ff_docked()
{
	STUB_FUNCTION;
}

void joy_ff_explode()
{
	STUB_FUNCTION;
}

void joy_ff_fly_by(int mag)
{
	STUB_FUNCTION;
}

void joy_ff_mission_init(vector v)
{
	STUB_FUNCTION;
}

void joy_ff_play_dir_effect(float x, float y)
{
	STUB_FUNCTION;
}

void joy_ff_play_primary_shoot(int gain)
{
	STUB_FUNCTION;
}

void joy_ff_play_reload_effect()
{
	STUB_FUNCTION;
}

void joy_ff_play_secondary_shoot(int gain)
{
	STUB_FUNCTION;
}

void joy_ff_play_vector_effect(vector *v, float scaler)
{
	STUB_FUNCTION;
}

void joy_ff_stop_effects()
{
	joy_ff_afterburn_off();
}
