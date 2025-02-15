/*
 * Copyright (C) Volition, Inc. 1999.  All rights reserved.
 *
 * All source code herein is the property of Volition, Inc. You may not sell
 * or otherwise commercially exploit the source or things you created based on
 * the source.
 */

#include "pofview.h"

#include "pstypes.h"
#include "2d.h"
#include "3d.h"
#include "bmpman.h"
#include "systemvars.h"
#include "cfile.h"
#include "timer.h"
#include "key.h"
#include "mouse.h"

#include "floating.h"
#include "lighting.h"


float flFrametime;		// used by game lib; DO NOT REMOVE!!

extern bool in_dialog;


// Stuff for showing ship thrusters.
typedef struct thrust_anim {
	int	num_frames;
	int	first_frame;
	float time;				// in seconds
} thrust_anim;

#define NUM_THRUST_ANIMS				4

#define THRUST_ANIM_NORMAL				0
#define THRUST_ANIM_AFTERBURNER			1
#define THRUST_ANIM_GLOW_NORMAL			2
#define THRUST_ANIM_GLOW_AFTERBURNER	3

static thrust_anim Thrust_anims[NUM_THRUST_ANIMS];

static char Thrust_anim_names[NUM_THRUST_ANIMS][MAX_FILENAME_LEN] = {
	"thruster01",
	"thruster01a",
	"thrusterglow01",
	"thrusterglow01a"
};


///////////////////////////////////////////////////////////////////////////////

PofViewTimer::PofViewTimer(PofViewCanvas *canvas)
	: wxTimer()
{
	m_canvas = canvas;
	m_thrust_timer = -1;
}

PofViewTimer::~PofViewTimer()
{
}

void PofViewTimer::Notify()
{
	float frame_time;

	if (m_thrust_timer == -1) {
		m_thrust_timer = timer_get_milliseconds();
	}

	int tmp_time = timer_get_milliseconds();

	frame_time = ((float)(tmp_time-m_thrust_timer)) / 1000.0f;
	flFrametime = frame_time;

	m_thrust_timer = tmp_time;

	m_canvas->DoThrusterFrame(frame_time);
	m_canvas->MoveViewer(frame_time);
	m_canvas->Render();
}

wxBEGIN_EVENT_TABLE(PofViewCanvas, wxGLCanvas)
	EVT_SIZE(PofViewCanvas::OnSize)
	EVT_PAINT(PofViewCanvas::OnPaint)
	EVT_ERASE_BACKGROUND(PofViewCanvas::OnEraseBackground)
	EVT_MOUSE_EVENTS(PofViewCanvas::OnMouse)
wxEND_EVENT_TABLE()

PofViewCanvas::PofViewCanvas(wxWindow *parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxGLCanvas(parent, id, NULL, pos, size, style | wxFULL_REPAINT_ON_RESIZE, name)
{
	first_frame = true;
	m_ViewerZoom = 0.8f;
	m_ViewerPos = vmd_zero_vector;
	m_ViewerPos.xyz.z = -10.0f;
	m_ViewerOrient = vmd_identity_matrix;
	m_ObjectOrient = vmd_identity_matrix;

	physics_init( &m_ViewerPhysics );
	m_ViewerPhysics.flags |= PF_ACCELERATES | PF_SLIDE_ENABLED;

	memset( &m_Viewer_ci, 0, sizeof(control_info) );

	m_mouse_inited = 0;
	m_mouse_dx = 0;
	m_mouse_dy = 0;

	m_timer = new PofViewTimer(this);
	m_timer->Start(1000/30);

	thrust_anim_inited = false;

	model_thrust = 0.5f;
	model_afterburner = 0;

	shipp_thruster_bitmap = -1;
	shipp_thruster_frame = 0.0f;

	shipp_thruster_glow_bitmap = -1;
	shipp_thruster_glow_frame = 0.0f;
	shipp_thruster_glow_noise = 1.0f;
}

PofViewCanvas::~PofViewCanvas()
{
	delete m_timer;
}


extern float Ambient_light;
extern void opengl_tcache_frame();

static vector Global_light_world = { { { -0.208758f, -0.688253f, 0.694782f } } };


void PofViewCanvas::Render()
{
	PofViewFrame *parent = (PofViewFrame *)GetParent();

	int model_num = parent->GetModelnum();

	if (model_num < 0) {
		return;
	}

	polymodel *pm = model_get(model_num);
	wxASSERT( pm );

	if (first_frame) {
		m_ViewerPos.xyz.z = -pm->rad * 1.5f;
		first_frame = false;
	}

	int w, h;

	GetClientSize(&w, &h);

	gr_reset_clip();
	gr_set_clip(0, 0, w, h);

	gr_set_clear_color(0, 0, 0);
	gr_clear();
	gr_set_color(127, 127, 127);

	float saved_Ambient_light = Ambient_light;

	light_reset();

	if ( parent->LightingOn() ) {
		light_add_directional(&Global_light_world, 1.0f, 1.0f, 1.0f, 1.0f);
	} else {
		Ambient_light = 0.0f;
	}

	g3_start_frame(1);

	g3_set_view_matrix(&m_ViewerPos, &m_ViewerOrient, m_ViewerZoom);

	uint render_flags = 0;

	if ( !parent->UseLighting() ) {
		render_flags |= MR_NO_LIGHTING;
	}

	if ( parent->ShowOutline() ) {
		render_flags |= MR_SHOW_OUTLINE;
		model_set_outline_color(0,0,0);
	}

	if ( parent->ShowShields() ){
		render_flags |= MR_SHOW_SHIELDS;
	}

	if ( parent->ShowInvisible() )	{
		render_flags |= MR_SHOW_INVISIBLE_FACES;
	}
/*
	if ( parent->ShowOverwrite() ) {
		Tmap_show_layers = 1;
	} else {
		Tmap_show_layers = 0;
	}
*/
	if ( parent->ShowPivots() ) {
		render_flags |= MR_SHOW_PIVOTS;
	}

	if ( parent->ShowPaths() ) {
		render_flags |= MR_SHOW_PATHS;
	}

	if ( parent->ShowRadius() ) {
		render_flags |= MR_SHOW_RADIUS;
	}

	if ( !parent->UseSmoothing() ) {
		render_flags |= MR_NO_SMOOTHING;
	}

	if ( !parent->UseTexturing() ) {
		render_flags |= MR_NO_TEXTURING;
	}

	if ( parent->GetDetailLevel() > 0 ) {
		render_flags |= MR_LOCK_DETAIL;
		model_set_detail_level(parent->GetDetailLevel()-1);
	}

	if ( parent->ShowBayPaths() ) {
		render_flags |= MR_BAY_PATHS;
	}

	if ( parent->UseAutocenter() ) {
		render_flags |= MR_AUTOCENTER;
	}

	vector temp_pos = { { { 0.0f, 0.0f, 0.0f } } };

	model_clear_instance( model_num );

	model_show_damaged(model_num, parent->ShowDamaged() );

	if ( model_afterburner ){
		model_set_thrust( model_num, 1.0f, shipp_thruster_bitmap, shipp_thruster_glow_bitmap, shipp_thruster_glow_noise );
	} else {
		model_set_thrust( model_num, model_thrust, shipp_thruster_bitmap, shipp_thruster_glow_bitmap, shipp_thruster_glow_noise );
	}

	if ( parent->ShowThrusters() ) {
		render_flags |= MR_SHOW_THRUSTERS;
	}

	static int whee = -1;
	if(whee == -1){
		whee = bm_load("helvig.pcx");
	}
	if(whee != -1){
		model_set_insignia_bitmap(whee);
	}

	if ( parent->GetDetailLevel() > 0 )	{
		model_render( model_num, &m_ObjectOrient, &temp_pos, render_flags );
	} else {
		for (int i = 0; i < pm->num_debris_objects; i++) {
			vector tmp = { { { 0.0f, 0.0f, 0.0f } } };
			vector tmp1 = pm->submodel[pm->debris_objects[i]].offset;
			model_find_world_point(&tmp, &tmp1, model_num, -1,&m_ObjectOrient, &temp_pos );
			submodel_render( model_num, pm->debris_objects[i],&m_ObjectOrient, &tmp, render_flags );
		}
	}

	g3_end_frame();

	this->SwapBuffers();

	opengl_tcache_frame();

	Ambient_light = saved_Ambient_light;
}

// JAS - figure out which thruster bitmap will get rendered next
// time around.  ship_render needs to have shipp_thruster_bitmap set to
// a valid bitmap number, or -1 if we shouldn't render thrusters.
void PofViewCanvas::DoThrusterFrame(float frame_time)
{
	float rate;
	int framenum;
	thrust_anim *the_anim;

	if ( !thrust_anim_inited ) {
		InitThrusters();
	}

	if (model_afterburner) {
		the_anim = &Thrust_anims[THRUST_ANIM_AFTERBURNER];
		rate = 1.2f;		// go at 1.2x faster when afterburners on
	} else {
		the_anim = &Thrust_anims[THRUST_ANIM_NORMAL];
		// If thrust at 0, go at half as fast, full thrust; full framerate
		// so set rate from 0.5 to 1.0, depending on thrust from 0 to 1
		rate = 0.5f + model_thrust / 2.0f;
	}

	shipp_thruster_frame += frame_time * rate;

	// Sanity checks
	if ( shipp_thruster_frame < 0.0f )	shipp_thruster_frame = 0.0f;
	if ( shipp_thruster_frame > 100.0f ) shipp_thruster_frame = 0.0f;

	if ( shipp_thruster_frame > the_anim->time )	{
		shipp_thruster_frame -= the_anim->time;
	}
	framenum = fl2i( (shipp_thruster_frame*the_anim->num_frames) / the_anim->time );
	if ( framenum < 0 ) framenum = 0;
	if ( framenum >= the_anim->num_frames ) framenum = the_anim->num_frames-1;

	// Get the bitmap for this frame
	shipp_thruster_bitmap = the_anim->first_frame + framenum;

//	mprintf(( "TF: %.2f\n", shipp_thruster_frame ));


	// Do for glows

	if (model_afterburner) {
		the_anim = &Thrust_anims[THRUST_ANIM_GLOW_AFTERBURNER];
		rate = 1.2f;		// go at 1.2x faster when afterburners on
	} else {
		the_anim = &Thrust_anims[THRUST_ANIM_GLOW_NORMAL];
		// If thrust at 0, go at half as fast, full thrust; full framerate
		// so set rate from 0.5 to 1.0, depending on thrust from 0 to 1
		rate = 0.5f + model_thrust / 2.0f;
	}

	shipp_thruster_glow_frame += frame_time * rate;

	// Sanity checks
	if ( shipp_thruster_glow_frame < 0.0f )	shipp_thruster_glow_frame = 0.0f;
	if ( shipp_thruster_glow_frame > 100.0f ) shipp_thruster_glow_frame = 0.0f;

	while (shipp_thruster_glow_frame > the_anim->time) {
		shipp_thruster_glow_frame -= the_anim->time;
	}
	framenum = fl2i( (shipp_thruster_glow_frame*the_anim->num_frames) / the_anim->time );
	if ( framenum < 0 ) framenum = 0;
	if ( framenum >= the_anim->num_frames ) framenum = the_anim->num_frames-1;

	// Get the bitmap for this frame
	shipp_thruster_glow_bitmap = the_anim->first_frame;
	shipp_thruster_glow_noise = Noise[framenum];

//	mprintf(( "TF: %.2f\n", shipp_thruster_frame ));

}

void PofViewCanvas::MoveViewer(float frame_time)
{
	int detail_lvl = -1;

	if (in_dialog) {
		return;
	}

	if ( wxGetKeyState(wxKeyCode('1')) ) {
		detail_lvl = 1;
	} else if ( wxGetKeyState(wxKeyCode('2')) ) {
		detail_lvl = 2;
	} else if ( wxGetKeyState(wxKeyCode('3')) ) {
		detail_lvl = 3;
	} else if ( wxGetKeyState(wxKeyCode('4')) ) {
		detail_lvl = 4;
	} else if ( wxGetKeyState(wxKeyCode('5')) ) {
		detail_lvl = 5;
	} else if ( wxGetKeyState(wxKeyCode('6')) ) {
		detail_lvl = 6;
	}

	if (detail_lvl >= 0) {
		((PofViewFrame*)GetParent())->SetDetailLevel(detail_lvl);
	}

	if ( wxGetKeyState(wxKeyCode('-')) ) {
		// Scales the engines thrusters by this much
		model_thrust -= 0.1f;

		if (model_thrust < 0.0f) {
			model_thrust = 0.0f;
		}
	} else if ( wxGetKeyState(wxKeyCode('=')) ) {
		// Scales the engines thrusters by this much
		model_thrust += 0.1f;

		if (model_thrust > 1.0f) {
			model_thrust = 1.0f;
		}
	}

	if ( wxGetKeyState(WXK_BACK) ) {
		model_afterburner = 1;
	} else {
		model_afterburner = 0;
	}

	int model_num = ((PofViewFrame*)GetParent())->GetModelnum();

	if (model_num < 0) {
		return;
	}

	polymodel *pm = model_get(model_num);

	if (pm == NULL) {
		return;
	}

	control_info *ci = &m_Viewer_ci;
	float kh = 0.0f;

	float c_scale = pm->core_radius; //1.0f;

//	if (pm->core_radius < 200.0f) {
//		c_scale = 1.0f / 6.0f;
//	}

	float temp = ci->heading;
	float temp1 = ci->pitch;
	memset( ci, 0, sizeof(control_info) );
	ci->heading = temp;
	ci->pitch = temp1;

	if ( wxGetKeyState(WXK_NUMPAD6) ) {
		kh = frame_time;
	} else if ( wxGetKeyState(WXK_NUMPAD4) ) {
		kh = -frame_time;
	} else {
		kh = 0.0f;
	}

	if (kh == 0.0f) {
		ci->heading = 0.0f;
	} else if (kh > 0.0f) {
		if (ci->heading < 0.0f) {
			ci->heading = 0.0f;
		}
	} else { // kh < 0
		if (ci->heading > 0.0f) {
			ci->heading = 0.0f;
		}
	}

	ci->heading += kh;

	if ( wxGetKeyState(WXK_NUMPAD8) ) {
		kh = frame_time;
	} else if ( wxGetKeyState(WXK_NUMPAD2) ) {
		kh = -frame_time;
	} else {
		kh = 0.0f;
	}

	if (kh == 0.0f){
		ci->pitch = 0.0f;
	} else if (kh > 0.0f) {
		if (ci->pitch < 0.0f){
			ci->pitch = 0.0f;
		}
	} else { // kh < 0
		if (ci->pitch > 0.0f){
			ci->pitch = 0.0f;
		}
	}

	ci->pitch += kh;

	if ( wxGetKeyState(WXK_NUMPAD7) ) {
		ci->bank = frame_time / 8.0f;
	} else if ( wxGetKeyState(WXK_NUMPAD9) ) {
		ci->bank = -frame_time / 8.0f;
	}

	if ( wxGetKeyState(wxKeyCode('A')) ) {
		ci->forward = frame_time * c_scale;
	} else if ( wxGetKeyState(wxKeyCode('Z')) ) {
		ci->forward = -frame_time * c_scale;
	}

	if ( wxGetKeyState(WXK_NUMPAD3) ) {
		ci->sideways = frame_time * c_scale;
	} else if ( wxGetKeyState(WXK_NUMPAD1) ) {
		ci->sideways = -frame_time * c_scale;
	}

	if ( wxGetKeyState(WXK_NUMPAD_SUBTRACT) ) {
		ci->vertical = frame_time * c_scale;
	} else if ( wxGetKeyState(WXK_NUMPAD_ADD) ) {
		ci->vertical = -frame_time * c_scale;
	}

	physics_read_flying_controls( &m_ViewerOrient, &m_ViewerPhysics, &m_Viewer_ci, frame_time );

	physics_sim(&m_ViewerPos, &m_ViewerOrient, &m_ViewerPhysics, frame_time * 6.0f );
}

void PofViewCanvas::OnPaint( wxPaintEvent& WXUNUSED(event) )
{
	wxPaintDC dc(this);

	Render();
}

void PofViewCanvas::OnSize(wxSizeEvent& WXUNUSED(event))
{
	if ( !this->IsShownOnScreen() ) {
		return;
	}

	int x = 640, y = 480;

	this->GetClientSize(&x, &y);

	gr_set_viewport(x, y);
}

void PofViewCanvas::OnEraseBackground(wxEraseEvent& WXUNUSED(event))
{
	// Do nothing, to avoid flashing on MSW
}

void PofViewCanvas::OnMouse(wxMouseEvent& event)
{
	if ( event.Dragging() ) {
		if (m_mouse_inited) {
			m_mouse_dx = event.GetX() - m_mouse_x;
			m_mouse_dy = event.GetY() - m_mouse_y;
		} else {
			m_mouse_inited = 1;
		}

		m_mouse_x = event.GetX();
		m_mouse_y = event.GetY();

		matrix tempm, mousem;

		if ( m_mouse_dx || m_mouse_dy )	{
			vm_trackball( -m_mouse_dx, m_mouse_dy, &mousem );
			vm_matrix_x_matrix(&tempm, &m_ObjectOrient, &mousem);
			m_ObjectOrient = tempm;

			m_mouse_dx = m_mouse_dy = 0;
		}
	} else {
		m_mouse_inited = 0;
	}
}

// loads the animations for ship's afterburners
void PofViewCanvas::InitThrusters()
{
	int			fps, i;
	thrust_anim	*ta;

	if (thrust_anim_inited)
		return;

	for ( i = 0; i < NUM_THRUST_ANIMS; i++ ) {
		ta = &Thrust_anims[i];

		// two anims
		if (i < THRUST_ANIM_GLOW_NORMAL) {
			ta->first_frame = bm_load_animation(Thrust_anim_names[i],  &ta->num_frames, &fps, 1);
			if ( ta->first_frame == -1 ) {
				Error(LOCATION,"Error loading animation file: %s\n",Thrust_anim_names[i]);
				return;
			}
			SDL_assert(fps != 0);
			ta->time = i2fl(ta->num_frames)/fps;
		}
		// two glow bitmaps
		else {
			ta->num_frames = NOISE_NUM_FRAMES;
			fps = 15;
			ta->first_frame = bm_load( Thrust_anim_names[i] );
			if ( ta->first_frame == -1 ) {
				Error(LOCATION,"Error loading bitmap file: %s\n",Thrust_anim_names[i]);
				return;
			}
			SDL_assert(fps != 0);
			ta->time = i2fl(ta->num_frames)/fps;
		}
	}

	thrust_anim_inited = true;
}
