#include "pstypes.h"
#include "joy.h"

Joy_info joystick;
int Joy_sensitivity = 9;

void joy_close()
{
	STUB_FUNCTION;
}

int joy_down(int btn)
{
	STUB_FUNCTION;
	
	return 0;
}

int joy_down_count(int btn, int reset_count)
{
	STUB_FUNCTION;
	
	return 0;
}

float joy_down_time(int btn)
{
	STUB_FUNCTION;
	
	return 0.0f;
}

void joy_flush()
{
	STUB_FUNCTION;
}

int joy_get_pos(int *x, int *y, int *z, int *rx)
{
	if (x) *x = 0;
	if (y) *y = 0;
	if (z) *z = 0;
	if (rx) *rx = 0;
        
        STUB_FUNCTION;
        
        return 0;                        
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
	STUB_FUNCTION;
	
	return 0;
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
