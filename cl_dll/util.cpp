/***
*
*	Copyright (c) 1996-2002, Valve LLC. All rights reserved.
*	
*	This product contains software technology licensed from Id 
*	Software, Inc. ("Id Technology").  Id Technology (c) 1996 Id Software, Inc. 
*	All Rights Reserved.
*
*   Use, distribution, and modification of this source code and/or resulting
*   object code is restricted to non-commercial enhancements to products from
*   Valve LLC.  All other use, distribution, or modification is prohibited
*   without written permission from Valve LLC.
*
****/
//
// util.cpp
//
// implementation of class-less helper functions
//

#include <cstdio>
#include <cstdlib>

#include "hud.h"
#include "cl_util.h"
#include <string.h>

HSPRITE LoadSprite(const char* pszName)
{
	int iRes;
	char sz[256]; 

	if (ScreenWidth > 2560 && ScreenHeight > 1600)
		iRes = 2560;
	else if (ScreenWidth >= 1280 && ScreenHeight > 720)
		iRes = 1280;
	else if (ScreenWidth >= 640)
		iRes = 640;
	else
		iRes = 320;

	sprintf(sz, pszName, iRes);

	return SPR_Load(sz);
}

// frac should always be multiplied by frametime
float lerp(float start, float end, float frac)
{
	// Exact, monotonic, bounded, determinate, and (for start=b=0) consistent:
	if (start <= 0 && end >= 0 || start >= 0 && end <= 0)
		return frac * end + (1.0f - frac) * start;

	if (frac == 1)
		return end; // exact
	// Exact at t=0, monotonic except near t=1,
	// bounded, determinate, and consistent:
	const float x = start + frac * (end - start);
	return frac > 1 == end > start ? V_max(end, x) : V_min(end, x); // monotonic near t=1
}

double dlerp(double start, double end, double frac)
{
	// Exact, monotonic, bounded, determinate, and (for start=b=0) consistent:
	if (start <= 0 && end >= 0 || start >= 0 && end <= 0)
		return frac * end + (1.0 - frac) * start;

	if (frac == 1)
		return end; // exact
	// Exact at t=0, monotonic except near t=1,
	// bounded, determinate, and consistent:
	const float x = start + frac * (end - start);
	return frac > 1 == end > start ? V_max(end, x) : V_min(end, x); // monotonic near t=1
}