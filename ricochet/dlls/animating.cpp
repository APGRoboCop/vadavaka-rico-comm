/***
*
*	Copyright (c) 1999, 2000 Valve LLC. All rights reserved.
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
/*

===== monsters.cpp ========================================================

  Monster-related utility code

*/

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "animation.h"
#include "saverestore.h"

TYPEDESCRIPTION	CBaseAnimating::m_SaveData[] = 
{
	DEFINE_FIELD( CBaseMonster, m_flFrameRate, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flGroundSpeed, FIELD_FLOAT ),
	DEFINE_FIELD( CBaseMonster, m_flLastEventCheck, FIELD_TIME ),
	DEFINE_FIELD( CBaseMonster, m_fSequenceFinished, FIELD_BOOLEAN ),
	DEFINE_FIELD( CBaseMonster, m_fSequenceLoops, FIELD_BOOLEAN ),
};

IMPLEMENT_SAVERESTORE( CBaseAnimating, CBaseDelay );


//=========================================================
// StudioFrameAdvance - advance the animation frame up to the current time
// if an flInterval is passed in, only advance animation that number of seconds
//=========================================================
float CBaseAnimating :: StudioFrameAdvance ( float flInterval )
{
	if (flInterval == 0.0f)
	{
		flInterval = gpGlobals->time - pev->animtime;
		if (flInterval <= 0.001f)
		{
			pev->animtime = gpGlobals->time;
			return 0.0f;
		}
	}
	if (! pev->animtime)
		flInterval = 0.0f;
	
	pev->frame += flInterval * m_flFrameRate * pev->framerate;
	pev->animtime = gpGlobals->time;

	if (pev->frame < 0.0f || pev->frame >= 256.0f) 
	{
		if (m_fSequenceLoops)
			pev->frame -= static_cast<int>(pev->frame / 256.0f) * 256.0f;
		else
			pev->frame = pev->frame < 0.0f ? 0 : 255;
		m_fSequenceFinished = TRUE;	// just in case it wasn't caught in GetEvents
	}

	return flInterval;
}

//=========================================================
// LookupActivity
//=========================================================
int CBaseAnimating :: LookupActivity ( int activity )
{
	ASSERT( activity != 0 );
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupActivity( pmodel, pev, activity );
}

//=========================================================
// LookupActivityHeaviest
//
// Get activity with highest 'weight'
//
//=========================================================
int CBaseAnimating :: LookupActivityHeaviest ( int activity )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupActivityHeaviest( pmodel, pev, activity );
}

//=========================================================
//=========================================================
int CBaseAnimating :: LookupSequence ( const char *label )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::LookupSequence( pmodel, label );
}


//=========================================================
//=========================================================
void CBaseAnimating :: ResetSequenceInfo ( )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	GetSequenceInfo( pmodel, pev, &m_flFrameRate, &m_flGroundSpeed );
	m_fSequenceLoops = (GetSequenceFlags() & STUDIO_LOOPING) != 0;
	pev->animtime = gpGlobals->time;
	pev->framerate = 1.0f;
	m_fSequenceFinished = FALSE;
	m_flLastEventCheck = gpGlobals->time;
}



//=========================================================
//=========================================================
BOOL CBaseAnimating :: GetSequenceFlags( )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::GetSequenceFlags( pmodel, pev );
}

//=========================================================
// DispatchAnimEvents
//=========================================================
void CBaseAnimating :: DispatchAnimEvents ( float flInterval )
{
	MonsterEvent_t	event;

	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	if ( !pmodel )
	{
		ALERT( at_aiconsole, "Gibbed monster is thinking!\n" );
		return;
	}

	// FIXME: I have to do this or some events get missed, and this is probably causing the problem below
	flInterval = 0.1f;

	// FIX: this still sometimes hits events twice
	const float flStart = pev->frame + (m_flLastEventCheck - pev->animtime) * m_flFrameRate * pev->framerate;
	const float flEnd = pev->frame + flInterval * m_flFrameRate * pev->framerate;
	m_flLastEventCheck = pev->animtime + flInterval;

	m_fSequenceFinished = FALSE;
	if (flEnd >= 256 || flEnd <= 0.0f) 
		m_fSequenceFinished = TRUE;

	int index = 0;

	while ( (index = GetAnimationEvent( pmodel, pev, &event, flStart, flEnd, index ) ) != 0 )
	{
		HandleAnimEvent( &event );
	}
}


//=========================================================
//=========================================================
float CBaseAnimating :: SetBoneController ( int iController, float flValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return SetController( pmodel, pev, iController, flValue );
}

//=========================================================
//=========================================================
void CBaseAnimating :: InitBoneControllers ()
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	SetController( pmodel, pev, 0, 0.0f );
	SetController( pmodel, pev, 1, 0.0f );
	SetController( pmodel, pev, 2, 0.0f );
	SetController( pmodel, pev, 3, 0.0f );
}

//=========================================================
//=========================================================
float CBaseAnimating :: SetBlending ( int iBlender, float flValue )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );

	return ::SetBlending( pmodel, pev, iBlender, flValue );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetBonePosition ( int iBone, Vector &origin, Vector &angles )
{
	GET_BONE_POSITION( ENT(pev), iBone, origin, angles );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetAttachment ( int iAttachment, Vector &origin, Vector &angles )
{
	GET_ATTACHMENT( ENT(pev), iAttachment, origin, angles );
}

//=========================================================
//=========================================================
int CBaseAnimating :: FindTransition( int iEndingSequence, int iGoalSequence, int *piDir )
{
	void *pmodel = GET_MODEL_PTR( ENT(pev) );
	
	if (piDir == nullptr)
	{
		int iDir;
		const int sequence = ::FindTransition( pmodel, iEndingSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransition( pmodel, iEndingSequence, iGoalSequence, piDir );
}

//=========================================================
//=========================================================
void CBaseAnimating :: GetAutomovement( Vector &origin, Vector &angles, float flInterval )
{

}

void CBaseAnimating :: SetBodygroup( int iGroup, int iValue )
{
	::SetBodygroup( GET_MODEL_PTR( ENT(pev) ), pev, iGroup, iValue );
}

int CBaseAnimating :: GetBodygroup( int iGroup )
{
	return ::GetBodygroup( GET_MODEL_PTR( ENT(pev) ), pev, iGroup );
}


int CBaseAnimating :: ExtractBbox( int sequence, float *mins, float *maxs )
{
	return ::ExtractBbox( GET_MODEL_PTR( ENT(pev) ), sequence, mins, maxs );
}

//=========================================================
//=========================================================

void CBaseAnimating :: SetSequenceBox()
{
	Vector mins, maxs;

	// Get sequence bbox
	if ( ExtractBbox( pev->sequence, mins, maxs ) )
	{
		// expand box for rotation
		// find min / max for rotations
		const float yaw = pev->angles.y * (M_PI / 180.0);
		
		Vector xvector, yvector;
		xvector.x = cos(yaw);
		xvector.y = sin(yaw);
		yvector.x = -sin(yaw);
		yvector.y = cos(yaw);
		Vector bounds[2];

		bounds[0] = mins;
		bounds[1] = maxs;
		
		Vector rmin( 9999, 9999, 9999 );
		Vector rmax( -9999, -9999, -9999 );
		Vector transformed;

		for (int i = 0; i <= 1; i++ )
		{
			Vector base;
			base.x = bounds[i].x;
			for ( int j = 0; j <= 1; j++ )
			{
				base.y = bounds[j].y;
				for ( int k = 0; k <= 1; k++ )
				{
					base.z = bounds[k].z;
					
				// transform the point
					transformed.x = xvector.x*base.x + yvector.x*base.y;
					transformed.y = xvector.y*base.x + yvector.y*base.y;
					transformed.z = base.z;
					
					for ( int l = 0; l < 3; l++ )
					{
						if (transformed[l] < rmin[l])
							rmin[l] = transformed[l];
						if (transformed[l] > rmax[l])
							rmax[l] = transformed[l];
					}
				}
			}
		}
		rmin.z = 0;
		rmax.z = rmin.z + 1;
		UTIL_SetSize( pev, rmin, rmax );
	}
}

