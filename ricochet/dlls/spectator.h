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
// Spectator.h

class CBaseSpectator : public CBaseEntity 
{
public:
	void Spawn() override;
	void SpectatorConnect();
	void SpectatorDisconnect();
	void SpectatorThink();

private:
	void SpectatorImpulseCommand();
};