//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// Triangle rendering, if any
#include <algorithm>
#include <cmath>

#include "hud.h"
#include "cl_util.h"

// Triangle rendering apis are in gEngfuncs.pTriAPI

#include "const.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "triangleapi.h"
#include "Exports.h"

#include "particleman.h"
#include "tri.h"

#include "studio.h"
#include "com_model.h"
#include "r_studioint.h"

#include "pm_defs.h"
#include "pmtrace.h"
#include "event_api.h"

extern engine_studio_api_s IEngineStudio;

extern IParticleMan* g_pParticleMan;

// STENCIL SHADOWS BEGIN
#include "svd_render.h"
#include "lightlist.h"
// STENCIL SHADOWS END

/*
=================
HUD_DrawNormalTriangles

Non-transparent triangles-- add them here
=================
*/
void DLLEXPORT HUD_DrawNormalTriangles()
{
	//	RecClDrawNormalTriangles();
	// STENCIL SHADOWS BEGIN
	gLightList.DrawNormal();
	// STENCIL SHADOWS END

	gHUD.m_Spectator.DrawOverview();
}


/*
=================
HUD_DrawTransparentTriangles

Render any triangles with transparent rendermode needs here
=================
*/
void DLLEXPORT HUD_DrawTransparentTriangles()
{
	//	RecClDrawTransparentTriangles();
	// STENCIL SHADOWS BEGIN
	SVD_DrawTransparentTriangles();
	// STENCIL SHADOWS END

	if (g_pParticleMan)
		g_pParticleMan->Update();
}
