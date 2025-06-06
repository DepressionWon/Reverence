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
// battery.cpp
//
// implementation of CHudBattery class
//

#include "hud.h"
#include "cl_util.h"
#include "parsemsg.h"

#include <string.h>
#include <stdio.h>

DECLARE_MESSAGE(m_Battery, Battery)

bool CHudBattery::Init()
{
	m_iBat = 0;
	m_fFade = 0;
	m_iFlags = 0;

	HOOK_MESSAGE(Battery);

	gHUD.AddHudElem(this);

	return true;
}


bool CHudBattery::VidInit()
{
	int HUD_suit_empty = gHUD.GetSpriteIndex("suit_empty");
	int HUD_suit_full = gHUD.GetSpriteIndex("suit_full");

	m_hSprite1 = m_hSprite2 = 0; // delaying get sprite handles until we know the sprites are loaded
	m_prc1 = &gHUD.GetSpriteRect(HUD_suit_empty);
	m_prc2 = &gHUD.GetSpriteRect(HUD_suit_full);
	m_iHeight = m_prc2->bottom - m_prc1->top;
	m_fFade = 0;

	m_HUD_DigitsBG1 = gHUD.GetSpriteIndex("number_dull1");
	m_HUD_DigitsBG2 = gHUD.GetSpriteIndex("number_dull2");

	int HUD_DigitsBG1 = gHUD.GetSpriteIndex("number_dull1");
	int HUD_DigitsBG2 = gHUD.GetSpriteIndex("number_dull2");

	m_prcDigitsBG1 = &gHUD.GetSpriteRect(HUD_DigitsBG1);
	m_prcDigitsBG2 = &gHUD.GetSpriteRect(HUD_DigitsBG2);
	return true;
}

bool CHudBattery::MsgFunc_Battery(const char* pszName, int iSize, void* pbuf)
{
	m_iFlags |= HUD_ACTIVE;

	BEGIN_READ(pbuf, iSize);
	int x = READ_SHORT();

	if (x != m_iBat)
	{
		m_fFade = FADE_TIME;
		m_iBat = x;
	}

	return true;
}


bool CHudBattery::Draw(float flTime)
{
	if ((gHUD.m_iHideHUDDisplay & HIDEHUD_HEALTH) != 0)
		return true;

	int r, g, b, x, y, a;
	Rect rc;

	rc = *m_prc2;

	rc.top += m_iHeight * ((float)(100 - (V_min(100, m_iBat))) * 0.01); // battery can go from 0 to 100 so * 0.01 goes from 0 to 1

	UnpackRGB(r, g, b, RGB_YELLOWISH);

	if (!gHUD.HasSuit())
		return true;

	// Has health changed? Flash the health #
	if (0 != m_fFade)
	{
		if (m_fFade > FADE_TIME)
			m_fFade = FADE_TIME;

		m_fFade -= (gHUD.m_flTimeDelta * 20);
		if (m_fFade <= 0)
		{
			a = 128;
			m_fFade = 0;
		}

		// Fade the health number back to dim

		a = MIN_ALPHA + (m_fFade / FADE_TIME) * 128;
	}
	else
		a = MIN_ALPHA;

	ScaleColors(r, g, b, a);

	int iOffset = (m_prc1->bottom - m_prc1->top) / 6;

	int width = (m_prc1->right - m_prc1->left);

	int bScale = 3;
	int yScale = 1;

	y = ScreenHeight - gHUD.m_iFontHeight - gHUD.m_iFontHeight / 2;
	x = bScale * width + 53;

	// make sure we have the right sprite handles
	if (0 == m_hSprite1)
		m_hSprite1 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_empty"));
	if (0 == m_hSprite2)
		m_hSprite2 = gHUD.GetSprite(gHUD.GetSpriteIndex("suit_full"));

	SPR_Set(m_hSprite1, r, g, b);
	SPR_DrawAdditive(0, x, y - iOffset - 10, m_prc1);

	if (rc.bottom > rc.top)
	{
		SPR_Set(m_hSprite2, r, g, b);
		SPR_DrawAdditive(0, x, y - iOffset - (10 * yScale) + (rc.top - m_prc2->top), &rc);
	}

	//x += width;
	//y += (int)(gHUD.m_iFontHeight * 0.2f);

	SPR_Set(m_hDigitsBG1, r, g, b);
	SPR_DrawAdditive(0, x, y, m_prcDigitsBG1);
	SPR_Set(m_hDigitsBG2, r, g, b);
	SPR_DrawAdditive(0, x + 20, y, m_prcDigitsBG2);
	SPR_Set(m_hDigitsBG1, r, g, b);
	SPR_DrawAdditive(0, x + 40, y, m_prcDigitsBG1);

	x = gHUD.DrawHudNumber(x, y, DHN_3DIGITS | DHN_DRAWZERO, m_iBat, r, g, b);

	return true;
}
