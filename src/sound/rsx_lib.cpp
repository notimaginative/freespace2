/*
 * $Logfile: /Freespace2/code/Sound/rsx_lib.cpp $
 * $Revision$
 * $Date$
 * $Author$
 *
 * C module for RSX sound lib
 *
 * $Log$
 * Revision 1.1  2002/05/03 03:28:10  root
 * Initial revision
 *
 * 
 * 2     10/07/98 10:54a Dave
 * Initial checkin.
 * 
 * 1     10/07/98 10:51a Dave
 * 
 * 3     10/14/97 11:33p Lawrance
 * get RSX implemented
 * 
 * 1     10/14/97 12:58p Lawrance
 * 
 * 1     10/14/97 10:35a Lawrance
 *
 * $NoKeywords: $
 */

#define	INITGUID
#include "pstypes.h"
#include <windows.h>
#include <rsx.h>
#include "rsx_lib.h"
#include "sound.h"
#include "osapi.h"

// gloabl variable to keep track of initalized status for RSX
int rsx_initialized = 0;

////////////////////////////////////////////
// Global RSX data
////////////////////////////////////////////
IUnknown					*lpRSXUnk;
//IRSX2						*lpRSX;
IRSX						*lpRSX;
IRSXDirectListener	*lpDL;

RSXENVIRONMENT			rsxEnv;

IRSXCachedEmitter		*lpCE[MAX_SOUNDS];

// initialize the listener.  Return -1 if failed to init listener, otherwise return 0
int rsx_init_listener()
{
//	HRESULT hr;

	// Create a listener and save the IRSXDirectListener interface
	RSXDIRECTLISTENERDESC rsxDL;            // listener description

	rsxDL.cbSize = sizeof(RSXDIRECTLISTENERDESC);
//	rsxDL.hMainWnd = NULL;
	rsxDL.hMainWnd = (HWND)os_get_window();
	rsxDL.dwUser = 0;
	rsxDL.lpwf = NULL;

/*
	hr = CoCreateInstance(CLSID_RSXDIRECTLISTENER, NULL, CLSCTX_INPROC_SERVER, IID_IRSXDirectListener, (void **) &lpDL);
	if ( SUCCEEDED(hr) ) {
		hr = lpDL->Initialize(&rsxDL, lpRSXUnk);
		if ( SUCCEEDED(hr) )
			return 0;
		else {
			Warning(LOCATION,"Unable to create the RSX listener\n");
			return -1;
		}
	} else {
		Warning(LOCATION,"Unable to create the RSX listener\n");
		return -1;
	}
*/

	if( SUCCEEDED(lpRSX->CreateDirectListener(&rsxDL, &lpDL, NULL)) ) {
		return 0;
	} else {
		return -1;
	}
}

// initialize the environment.  lpRSX must be non-null.  Return -1 if there is
// an error, otherwise return 0
int rsx_init_environment()
{
	Assert(lpRSX != NULL);

	rsxEnv.cbSize = sizeof(RSXENVIRONMENT);
	rsxEnv.dwFlags = RSXENVIRONMENT_SPEEDOFSOUND;
	rsxEnv.fSpeedOfSound = 200.0f;
	if ( lpRSX->SetEnvironment(&rsxEnv) == S_OK )
		return 0;
	else
		return -1;
}

// init the RSX sound system.  Return -1 if a failure occurs, otherwise return 0
int rsx_init()
{
	HRESULT	hr;
	int		i;

	if ( rsx_initialized )
		return 0;

	// Initialize COM Library
	hr  = CoInitialize(NULL);

	// Specify the class Id for RSX and the interface ID for the IRSX2 interface (use IID_IRSX20 if using IRSX)
	hr = CoCreateInstance(CLSID_RSX20, NULL, CLSCTX_INPROC_SERVER, IID_IRSX20, (void **) &lpRSX);
	if ( !SUCCEEDED(hr) ) {
		Warning(LOCATION,"Unable to create the RSX interface\n");
		return -1;
	}
/*
//	hr = CoCreateInstance(CLSID_RSX20, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void **) &lpRSXUnk);
	// Query the object for an IUnknown interface
   hr = lpRSX->QueryInterface(IID_IUnknown, (void**)&lpRSXUnk);
	if ( !SUCCEEDED(hr) ) {
		Warning(LOCATION,"Unable to create the RSX Unknown interface\n");
		return -1;
	}
*/

	if ( rsx_init_environment() == -1 ) {
		Warning(LOCATION,"Unable to initialize the RSX environment\n");
		return -1;
	}

	if ( rsx_init_listener() == -1 ) {
		Warning(LOCATION,"Unable to initialize the RSX direct listener\n");
		return -1;
	}

	for ( i = 0; i < MAX_SOUNDS; i++ ) {
		if ( lpCE[i] != NULL ) {
			lpCE[i]->Release();
			lpCE[i] = NULL;
		}
	}
	
	rsx_initialized = 1;
	return 0;
}

// called once per frame to update the listener position and orientation
void rsx_update_listener(vector *pos, matrix *orient)
{
	HRESULT hr;
	RSXVECTOR3D u, v;

	if ( !rsx_initialized )
		return;

	u.x = pos->x;
	u.y = pos->y;
	u.z = pos->z;
	hr = lpDL->SetPosition(&u);
	if (hr != S_OK)
		Warning(LOCATION,"Unable to set position for the RSX listener\n");
		
	u.x = orient->fvec.x;
	u.y = orient->fvec.y;
	u.z = orient->fvec.z;

	v.x = orient->uvec.x;
	v.y = orient->uvec.y;
	v.z = orient->uvec.z;
	
	hr = lpDL->SetOrientation(&u, &v);
	if (hr != S_OK)
		Warning(LOCATION,"Unable to set orientation for the RSX listener\n");
}

// uninitialize the RSX sound system
void rsx_close()
{
	int i;

	for(i = 0; i < MAX_SOUNDS; i++){
		if ( lpCE[i] != NULL) {
			lpCE[i]->Release();
			lpCE[i] = NULL;
		}
	}

	// TODO: release the channels
    
	// Release the listener
	if ( lpDL ) {
		lpDL->Release();
		lpDL = NULL;
	}

	// Release RSX
	if( lpRSX ) {
		lpRSX->Release();
      lpRSX = NULL;
	}

	CoUninitialize();
}

// return index into lpCE[] array, this is the Sounds[].sid member used to link the game-level
// description of the sound to the low-level cached emitter
int rsx_create_cached_emitter(char *filename, int is_3d, int use_doppler, int min, int max, float max_volume)
{
	RSXCACHEDEMITTERDESC	rsxCE; 	
	RSXEMITTERMODEL		rsxModel;
	int						ce_index;
//	HRESULT					hr;

	ZeroMemory(&rsxCE, sizeof(RSXCACHEDEMITTERDESC));
	rsxCE.cbSize = sizeof(RSXCACHEDEMITTERDESC);
	strcpy(rsxCE.szFilename, filename);

	ZeroMemory(&rsxModel, sizeof(RSXEMITTERMODEL));
	rsxModel.cbSize = sizeof(RSXEMITTERMODEL);

	for ( ce_index = 0; ce_index < MAX_SOUNDS; ce_index++ ) {
		if ( lpCE[ce_index] == NULL )
			break;
	}

	if ( ce_index == MAX_SOUNDS ) {
		Warning(LOCATION, "RSX has exceeded the maximum number of sounds\n");
		return -1;
	} 

	if ( is_3d ) {
		rsxCE.dwFlags |= RSXEMITTERDESC_NOREVERB;	// no reverb by default
		if ( !use_doppler )
			rsxCE.dwFlags |= RSXEMITTERDESC_NODOPPLER;

		rsxModel.fIntensity	= max_volume;		
//		rsxModel.fIntensity	= 1.0f;		
		rsxModel.fMinBack		= i2fl(min);
		rsxModel.fMinFront	= i2fl(min);
		rsxModel.fMaxBack		= i2fl(max);
		rsxModel.fMaxFront	= i2fl(max);
	} else {
		rsxCE.dwFlags = RSXEMITTERDESC_NOSPATIALIZE | RSXEMITTERDESC_NOATTENUATE | RSXEMITTERDESC_NODOPPLER | RSXEMITTERDESC_NOREVERB;
		rsxModel.fIntensity	= max_volume;		
	}

/*
	hr = CoCreateInstance(CLSID_RSXCACHEDEMITTER, NULL, CLSCTX_INPROC_SERVER, IID_IRSXCachedEmitter, (void **) &lpCE[ce_index]);
	if ( FAILED(hr) ) {
		Warning(LOCATION, "Creating an RSX cached emitter failed\n");
		return -1;
	}

	hr = lpCE[ce_index]->Initialize(&rsxCE, lpRSXUnk);
	if ( FAILED(hr) ) {
		Warning(LOCATION, "Initialization of an RSX cached emitter failed\n");
		return -1;
	}
*/

	if( !SUCCEEDED(lpRSX->CreateCachedEmitter(&rsxCE, &lpCE[ce_index], NULL)) ) {
		Warning(LOCATION, "Could not create a RSX cached emitter\n");
		return -1;
	}
	
	lpCE[ce_index]->SetModel(&rsxModel);
	return ce_index;
}

int rsx_play_3d(int sid, float priority, float volume, vector *pos, vector *sound_fvec)
{
	RSXQUERYMEDIAINFO qmi;
	qmi.cbSize = sizeof(RSXQUERYMEDIAINFO);
	lpCE[sid]->QueryMediaState(&qmi);

   if ( qmi.dwControl == RSX_PLAY )
		return -1;

	lpCE[sid]->SetPosition((RSXVECTOR3D*)pos);
//	lpCE[sid]->SetOrientation((RSXVECTOR3D*)sound_fvec);
	lpCE[sid]->ControlMedia(RSX_PLAY, 1, 0.0f);
	return 1;
}

int rsx_play( int sid, float priority, float volume)
{
	RSXQUERYMEDIAINFO qmi;
	HRESULT				hr;

	qmi.cbSize = sizeof(RSXQUERYMEDIAINFO);
	lpCE[sid]->QueryMediaState(&qmi);

   if ( qmi.dwControl == RSX_PLAY )
		return -1;

	hr = lpCE[sid]->ControlMedia(RSX_PLAY, 1, 0.0f);
	if ( hr != S_OK ) {
		Warning(LOCATION, "rsx_play() failure\n");
		return -1;
	}
	return 1;
}


void rsx_unload_buffer(int sid)
{
	if ( sid != -1 ) {
		if ( lpCE[sid] != NULL ) {
			lpCE[sid]->Release();
			lpCE[sid] = NULL;
		}
	}
}
