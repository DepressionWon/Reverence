//========= Copyright © 1996-2002, Valve LLC, All rights reserved. ============
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================

// studio_model.cpp
// routines for setting up to draw 3DStudio models

#include "hud.h"
#include "cl_util.h"
#include "const.h"
#include "com_model.h"
#include "studio.h"
#include "entity_state.h"
#include "cl_entity.h"
#include "dlight.h"
#include "triangleapi.h"
#include "lightlist.h"

#include <stdio.h>
#include <string.h>
#include <memory.h>
#include <math.h>

#include "studio_util.h"
#include "r_studioint.h"

#include "StudioModelRenderer.h"
#include "GameStudioModelRenderer.h"

#include "pmtrace.h"
#include "r_efx.h"
#include "event_api.h"
#include "event_args.h"
#include "in_defs.h"
#include "pm_defs.h"



// Global engine <-> studio model rendering code interface
extern engine_studio_api_t IEngineStudio;

void VectorRotate(const Vector in1, float in2[3][4], float* out)
{
	out[0] = DotProduct(in1, in2[0]);
	out[1] = DotProduct(in1, in2[1]);
	out[2] = DotProduct(in1, in2[2]);
}

__forceinline float Q_rsqrt(float number)
{
	long i;
	float x2, y;
	const float threehalfs = 1.5F;

	x2 = number * 0.5F;
	y = number;
	i = *(long*)&y;			   // evil floating point bit level hacking
	i = 0x5f3759df - (i >> 1); // what the fuck?
	y = *(float*)&i;
	y = y * (threehalfs - (x2 * y * y)); // 1st iteration

	return y;
}

void VectorNormalizeFast(float* v)
{
	float ilength = DotProduct(v, v);
	float sqroot = Q_rsqrt(ilength);
	VectorScale(v, sqroot, v);
}

/*
====================
StudioSetupShadows

====================
*/
void CStudioModelRenderer::StudioSetupShadows(void)
{
	if (IEngineStudio.IsHardware() != 1)
		return;

	// Determine the shading angle
	if (m_iClosestLight == -1 || (int)m_pCvarDrawStencilShadows->value == 2)
	{
		Vector shadeVector;
		shadeVector[0] = 0.3;
		shadeVector[1] = 0.5;
		shadeVector[2] = 1;

		VectorInverse(shadeVector);
		shadeVector = shadeVector.Normalize();

		m_vShadowLightVector = shadeVector;
		m_shadowLightType = SL_TYPE_LIGHTVECTOR;
	}
	else
	{
		elight_t* plight = m_pEntityLights[m_iClosestLight];
		m_vShadowLightOrigin = plight->origin;
		m_shadowLightType = SL_TYPE_POINTLIGHT;
	}
}

/*
====================
StudioSetupModelSVD

====================
*/
void CStudioModelRenderer::StudioSetupModelSVD(int bodypart)
{
	if (bodypart > m_pSVDHeader->numbodyparts)
		bodypart = 0;

	svdbodypart_t* pbodypart = (svdbodypart_t*)((byte*)m_pSVDHeader + m_pSVDHeader->bodypartindex) + bodypart;

	int index = m_pCurrentEntity->curstate.body / pbodypart->base;
	index = index % pbodypart->numsubmodels;

	m_pSVDSubModel = (svdsubmodel_t*)((byte*)m_pSVDHeader + pbodypart->submodelindex) + index;
}


/*
====================
StudioShouldDrawShadow

====================
*/
bool CStudioModelRenderer::StudioShouldDrawShadow(void)
{
	if (m_pCvarDrawStencilShadows->value < 1)
		return false;

	if (IEngineStudio.IsHardware() != 1)
		return false;

	if (!m_pRenderModel->visdata)
		return false;

	if (m_pCurrentEntity->curstate.renderfx == kRenderFxNoShadow)
		return false;

	// Fucking butt-ugly hack to make the shadows less annoying
	pmtrace_t tr;
	gEngfuncs.pEventAPI->EV_SetTraceHull(2);
	gEngfuncs.pEventAPI->EV_PlayerTrace(m_vRenderOrigin, m_pCurrentEntity->origin + Vector(0, 0, 1), PM_WORLD_ONLY, -1, &tr);

	if (tr.fraction != 1.0)
		return false;

	return true;
}

/*
====================
StudioDrawShadow

====================
*/
void CStudioModelRenderer::StudioDrawShadow(void)
{
	glPushClientAttrib(GL_CLIENT_VERTEX_ARRAY_BIT);

	// Disabable these to avoid slowdown bug
	glDisableClientState(GL_NORMAL_ARRAY);
	glDisableClientState(GL_COLOR_ARRAY);

	glClientActiveTexture(GL_TEXTURE1);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE2);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE3);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);
	glClientActiveTexture(GL_TEXTURE0);
	glDisableClientState(GL_TEXTURE_COORD_ARRAY);

	glVertexPointer(3, GL_FLOAT, sizeof(Vector), m_vertexTransform);
	glEnableClientState(GL_VERTEX_ARRAY);

	// Set SVD header
	m_pSVDHeader = (svdheader_t*)m_pRenderModel->visdata;

	glDepthMask(GL_FALSE);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE); // disable writes to color buffer

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_ALWAYS, 0, ~0);

	if (m_bTwoSideSupported)
	{
		glDisable(GL_CULL_FACE);
		glEnable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	for (int i = 0; i < m_pStudioHeader->numbodyparts; i++)
	{
		StudioSetupModelSVD(i);
		StudioDrawShadowVolume();
	}

	glDepthMask(GL_TRUE);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDisable(GL_STENCIL_TEST);

	if (m_bTwoSideSupported)
	{
		glEnable(GL_CULL_FACE);
		glDisable(GL_STENCIL_TEST_TWO_SIDE_EXT);
	}

	glDisableClientState(GL_VERTEX_ARRAY);

	glPopClientAttrib();
}

/*
====================
StudioDrawShadowVolume

====================
*/
void CStudioModelRenderer::StudioDrawShadowVolume(void)
{
	float plane[4];
	Vector lightdir;
	Vector *pv1, *pv2, *pv3;

	if (!m_pSVDSubModel->numfaces)
		return;

	Vector* psvdverts = (Vector*)((byte*)m_pSVDHeader + m_pSVDSubModel->vertexindex);
	byte* pvertbone = ((byte*)m_pSVDHeader + m_pSVDSubModel->vertinfoindex);

	// Extrusion distance
	float extrudeDistance = m_pCvarShadowVolumeExtrudeDistance->value;

	// Calculate vertex coords
	if (m_shadowLightType == SL_TYPE_POINTLIGHT)
	{
		// For point light sources
		for (int i = 0, j = 0; i < m_pSVDSubModel->numverts; i++, j += 2)
		{
			VectorTransform(psvdverts[i], (*m_pbonetransform)[pvertbone[i]], m_vertexTransform[j]);

			VectorSubtract(m_vertexTransform[j], m_vShadowLightOrigin, lightdir);
			VectorNormalizeFast(lightdir);

			VectorMA(m_vertexTransform[j], extrudeDistance, lightdir, m_vertexTransform[j + 1]);
		}
	}
	else
	{
		for (int i = 0, j = 0; i < m_pSVDSubModel->numverts; i++, j += 2)
		{
			VectorTransform(psvdverts[i], (*m_pbonetransform)[pvertbone[i]], m_vertexTransform[j]);
			VectorMA(m_vertexTransform[j], extrudeDistance, m_vShadowLightVector, m_vertexTransform[j + 1]);
		}
	}

	// Process the faces
	int numIndexes = 0;
	svdface_t* pfaces = (svdface_t*)((byte*)m_pSVDHeader + m_pSVDSubModel->faceindex);

	if (m_shadowLightType == SL_TYPE_POINTLIGHT)
	{
		// For point light sources
		for (int i = 0; i < m_pSVDSubModel->numfaces; i++)
		{
			pv1 = &m_vertexTransform[pfaces[i].vertex0];
			pv2 = &m_vertexTransform[pfaces[i].vertex1];
			pv3 = &m_vertexTransform[pfaces[i].vertex2];

			plane[0] = pv1->y * (pv2->z - pv3->z) + pv2->y * (pv3->z - pv1->z) + pv3->y * (pv1->z - pv2->z);
			plane[1] = pv1->z * (pv2->x - pv3->x) + pv2->z * (pv3->x - pv1->x) + pv3->z * (pv1->x - pv2->x);
			plane[2] = pv1->x * (pv2->y - pv3->y) + pv2->x * (pv3->y - pv1->y) + pv3->x * (pv1->y - pv2->y);
			plane[3] = -(pv1->x * (pv2->y * pv3->z - pv3->y * pv2->z) + pv2->x * (pv3->y * pv1->z - pv1->y * pv3->z) + pv3->x * (pv1->y * pv2->z - pv2->y * pv1->z));

			m_trianglesFacingLight[i] = (DotProduct(plane, m_vShadowLightOrigin) + plane[3]) > 0;
			if (m_trianglesFacingLight[i])
			{
				m_shadowVolumeIndexes[numIndexes] = pfaces[i].vertex0;
				m_shadowVolumeIndexes[numIndexes + 1] = pfaces[i].vertex2;
				m_shadowVolumeIndexes[numIndexes + 2] = pfaces[i].vertex1;

				m_shadowVolumeIndexes[numIndexes + 3] = pfaces[i].vertex0 + 1;
				m_shadowVolumeIndexes[numIndexes + 4] = pfaces[i].vertex1 + 1;
				m_shadowVolumeIndexes[numIndexes + 5] = pfaces[i].vertex2 + 1;

				numIndexes += 6;
			}
		}
	}
	else
	{
		// For a light vector
		for (int i = 0; i < m_pSVDSubModel->numfaces; i++)
		{
			pv1 = &m_vertexTransform[pfaces[i].vertex0];
			pv2 = &m_vertexTransform[pfaces[i].vertex1];
			pv3 = &m_vertexTransform[pfaces[i].vertex2];

			// Calculate normal of the face
			plane[0] = pv1->y * (pv2->z - pv3->z) + pv2->y * (pv3->z - pv1->z) + pv3->y * (pv1->z - pv2->z);
			plane[1] = pv1->z * (pv2->x - pv3->x) + pv2->z * (pv3->x - pv1->x) + pv3->z * (pv1->x - pv2->x);
			plane[2] = pv1->x * (pv2->y - pv3->y) + pv2->x * (pv3->y - pv1->y) + pv3->x * (pv1->y - pv2->y);

			m_trianglesFacingLight[i] = DotProduct(plane, m_vShadowLightVector) > 0;
			if (m_trianglesFacingLight[i])
			{
				m_shadowVolumeIndexes[numIndexes] = pfaces[i].vertex0;
				m_shadowVolumeIndexes[numIndexes + 1] = pfaces[i].vertex2;
				m_shadowVolumeIndexes[numIndexes + 2] = pfaces[i].vertex1;

				m_shadowVolumeIndexes[numIndexes + 3] = pfaces[i].vertex0 + 1;
				m_shadowVolumeIndexes[numIndexes + 4] = pfaces[i].vertex1 + 1;
				m_shadowVolumeIndexes[numIndexes + 5] = pfaces[i].vertex2 + 1;

				numIndexes += 6;
			}
		}
	}

	// Process the edges
	svdedge_t* pedges = (svdedge_t*)((byte*)m_pSVDHeader + m_pSVDSubModel->edgeindex);
	for (int i = 0; i < m_pSVDSubModel->numedges; i++)
	{
		if (m_trianglesFacingLight[pedges[i].face0])
		{
			if ((pedges[i].face1 != -1) && m_trianglesFacingLight[pedges[i].face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = pedges[i].vertex0;
			m_shadowVolumeIndexes[numIndexes + 1] = pedges[i].vertex1;
		}
		else
		{
			if ((pedges[i].face1 == -1) || !m_trianglesFacingLight[pedges[i].face1])
				continue;

			m_shadowVolumeIndexes[numIndexes] = pedges[i].vertex1;
			m_shadowVolumeIndexes[numIndexes + 1] = pedges[i].vertex0;
		}

		m_shadowVolumeIndexes[numIndexes + 2] = m_shadowVolumeIndexes[numIndexes] + 1;
		m_shadowVolumeIndexes[numIndexes + 3] = m_shadowVolumeIndexes[numIndexes + 2];
		m_shadowVolumeIndexes[numIndexes + 4] = m_shadowVolumeIndexes[numIndexes + 1];
		m_shadowVolumeIndexes[numIndexes + 5] = m_shadowVolumeIndexes[numIndexes + 1] + 1;
		numIndexes += 6;
	}

	if (m_bTwoSideSupported)
	{
		glActiveStencilFaceEXT(GL_BACK);
		glStencilOp(GL_KEEP, GL_INCR_WRAP_EXT, GL_KEEP);
		glStencilMask(~0);

		glActiveStencilFaceEXT(GL_FRONT);
		glStencilOp(GL_KEEP, GL_DECR_WRAP_EXT, GL_KEEP);
		glStencilMask(~0);

		glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);
	}
	else
	{
		if (m_shadowLightType != SL_TYPE_POINTLIGHT)
		{
			// draw back faces incrementing stencil values when z fails
			glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
			glCullFace(GL_FRONT);
			glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);

			// draw front faces decrementing stencil values when z fails
			glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
			glCullFace(GL_BACK);
			glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);
		}
		else
		{
			// draw back faces incrementing stencil values when z fails
			glStencilOp(GL_KEEP, GL_INCR, GL_KEEP);
			glCullFace(GL_BACK);
			glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);

			// draw front faces decrementing stencil values when z fails
			glStencilOp(GL_KEEP, GL_DECR, GL_KEEP);
			glCullFace(GL_FRONT);
			glDrawElements(GL_TRIANGLES, numIndexes, GL_UNSIGNED_SHORT, m_shadowVolumeIndexes);
		}
	}
}