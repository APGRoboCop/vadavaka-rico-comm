//=========== (C) Copyright 1999 Valve, L.L.C. All rights reserved. ===========
//
// The copyright to the contents herein is the property of Valve, L.L.C.
// The contents may be used and/or copied only with the written permission of
// Valve, L.L.C., or in accordance with the terms and conditions stipulated in
// the agreement/contract under which the contents have been supplied.
//
// Purpose: Handles the arena portion of discwar
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================

#include "extdll.h"
#include "util.h"
#include "cbase.h"
#include "player.h"
#include "gamerules.h"
#include "weapons.h"
#include "discwar.h"
#include "disc_arena.h"
#include "disc_objects.h"
//Add for Linux compile
#include <cstring>

int g_iNextArenaGroupInfo = 0;

void ClientUserInfoChanged( edict_t *pEntity, char *infobuffer );
edict_t *EntSelectSpawnPoint( CBaseEntity *pPlayer );
void respawn(entvars_t* pev, BOOL fCopyCorpse);
extern int gmsgStartRnd;
extern int gmsgEndRnd;
extern int gmsgTeamInfo;
extern int g_iPlayersPerTeam;
extern int g_iMapTurnedOffArena;

CDiscArena *g_pArenaList[ MAX_ARENAS ];

char *g_szCountDownVox[6] =
{
	"die",
	"one",
	"two",
	"three",
	"four",
};

char *g_szTeamNames[3] =
{
	"",
	"red",
	"blue",
};

int InArenaMode()
{
	if ( g_iMapTurnedOffArena )
		return FALSE;

	if ( gpGlobals->maxClients == 1 || CVAR_GET_FLOAT("rc_arena") == 0 )
		return FALSE;

	return TRUE;
}

LINK_ENTITY_TO_CLASS( disc_arena, CDiscArena );

// NOTE:
// We store queue position in pev->playerclass, so it's automatically pushed
// down to the client. The scoreboard uses pev->playerclass to display the
// positions of the players in the queue.
void CDiscArena::Spawn()
{
	pev->groupinfo = 1 << g_iNextArenaGroupInfo++;
	Reset();

	// Initialize
	m_iMaxRounds = CVAR_GET_FLOAT("rc_rounds");
	m_iPlayersPerTeam = g_iPlayersPerTeam;
	SetThink( NULL );
}

void CDiscArena::Reset()
{
	// Remove all clients in the queue
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->m_pCurrentArena == this && pPlayer->m_bHasDisconnected != TRUE )
		{
			RemoveClient( pPlayer );

			// Move her into spectator mode
			//MoveToSpectator( pPlayer );
		}
	}

	m_pPlayerQueue = nullptr;
	m_iPlayers = 0;
	m_flTimeLimitOver = 0;
	m_bShownTimeWarning = FALSE;
	m_iArenaState = ARENA_WAITING_FOR_PLAYERS;
	memset( m_hCombatants, 0, sizeof( m_hCombatants ) );

	SetThink( NULL );
	pev->nextthink = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Start a battle. Spawn all the players, and begin the countdown.
//-----------------------------------------------------------------------------
void CDiscArena::StartBattle()
{
	m_iCurrRound = 0;
	m_iTeamOneScore = m_iTeamTwoScore = 0;

	// First, set all players in this arena to "didn't play"
	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->m_pCurrentArena == this && pPlayer->m_bHasDisconnected != TRUE )
			pPlayer->m_iLastGameResult = GAME_DIDNTPLAY;
	}

	// Get the players in the battle
	for ( i = 0; i < m_iPlayersPerTeam * 2; i++ )
	{
		CBasePlayer *pCurr;

		// Check to see if this slot's already full
		if ( m_hCombatants[ i ] )
		{
			pCurr = (CBasePlayer*)(CBaseEntity*)m_hCombatants[ i ];
		}
		else
		{
			// Pop a new player from the queue
			pCurr = GetNextPlayer();
			if (!pCurr)
			{
				// Couldnt get enough players. Reset.
				Reset();
				return;
			}
		}

		// Set her team number
		if ( i < m_iPlayersPerTeam )
			pCurr->pev->team = 1;
		else
			pCurr->pev->team = 2;
		pCurr->pev->iuser4 = pCurr->pev->team;

		char sz[128];
		snprintf(sz, sizeof(sz), "Arena %d", pev->groupinfo);
		MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
			WRITE_BYTE( pCurr->entindex() );
			WRITE_STRING( sz );
		MESSAGE_END();

		// Add her to the list of combatants
		m_hCombatants[ i ] = pCurr;
		
		// Force her to update her clientinfo, so her colors match her team
		ClientUserInfoChanged( pCurr->edict(), g_engfuncs.pfnGetInfoKeyBuffer( pCurr->edict() ) );
	}

	// Start the first round
	StartRound();
}

//-----------------------------------------------------------------------------
// Purpose: Start the next round in the match
//-----------------------------------------------------------------------------
void CDiscArena::StartRound()
{
	m_iCurrRound++;
	m_iSecondsTillStart = ARENA_TIME_PREBATTLE;
	m_flTimeLimitOver = gpGlobals->time + ARENA_TIME_ROUNDLIMIT;

	RestoreWorldObjects();



	// Tell the clients
	for ( int iPlayerNum = 1; iPlayerNum <= gpGlobals->maxClients; iPlayerNum++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( iPlayerNum );

		if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE )
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStartRnd, nullptr, pPlayer->edict() );
				WRITE_BYTE( m_iCurrRound );
				WRITE_BYTE( m_iSecondsTillStart );
				WRITE_BYTE( m_iPlayersPerTeam * 2 );

				// Send down all the players in the round
				for ( int j = 0; j < m_iPlayersPerTeam * 2; j++ )
				{
					if (m_hCombatants[j])
						WRITE_SHORT( m_hCombatants[j]->entindex() );
				}

			MESSAGE_END();

			

		}
	}

	// Spawn all the players in the round
	for ( int i = 0; i < m_iPlayersPerTeam * 2; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];

		if ( pPlayer )
		{
			// make sure the player's groupinfo is set the arena's groupinfo
			pPlayer->pev->groupinfo = pev->groupinfo;

			// is the player an observer? 
			if ( pPlayer->IsObserver() )
			{
				SpawnCombatant( pPlayer );

				
			}

			// Remove any powerups
			pPlayer->RemoveAllPowerups();
			
		}
	}

	// Start counting down
	m_iArenaState = ARENA_COUNTDOWN;
	pev->nextthink = gpGlobals->time + 0.1f;
	SetThink( &CDiscArena::CountDownThink );
}

//-----------------------------------------------------------------------------
// Purpose: Make sure all the Combatants in the game a valid. Return FALSE if not.
//-----------------------------------------------------------------------------
int CDiscArena::ValidateCombatants()
{
	for ( int i = 0; i < m_iPlayersPerTeam * 2; i++ )
	{
		const CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];

		if ( !pPlayer )
			return FALSE;
		if ( pPlayer->m_bHasDisconnected )
			return FALSE;
	}
	
	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose: Restore all the world objects
//-----------------------------------------------------------------------------
void CDiscArena::RestoreWorldObjects()
{
	CBaseEntity *pFunc = nullptr;
	while ((pFunc = UTIL_FindEntityByClassname( pFunc, "func_plat_toggleremove" )) != nullptr)
	{
		((CPlatToggleRemove*)pFunc)->Reset();
	}
	while ((pFunc = UTIL_FindEntityByClassname( pFunc, "func_disctoggle" )) != nullptr)
	{
		((CDiscTarget*)pFunc)->Reset();
	}

	// Disable powerups
	while ((pFunc = UTIL_FindEntityByClassname( pFunc, "item_powerup" )) != nullptr)
	{
		((CDiscwarPowerup*)pFunc)->Disable();
	}
}

void CDiscArena::BattleThink()
{
	if ( gpGlobals->time >= m_flTimeLimitOver - 1.0f )
	{
		pev->nextthink = gpGlobals->time + 1.0f;
		SetThink( &CDiscArena::TimeOver );
		return;
	}

	if ( !CheckBattleOver() )
	{
		pev->nextthink = gpGlobals->time + 1.0f;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Countdown to the round start
//-----------------------------------------------------------------------------
void CDiscArena::CountDownThink()
{
	// Freeze everyone until there's 3 seconds to go
	if ( m_iSecondsTillStart == 3 )
	{
		for ( int i = 0; i < m_iPlayersPerTeam * 2; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];
	
			/* Miagi - I don't think we need this anymore since I found the true white disc.
			//.ASM : white disc haxxx...
			// if the player is "dead" and not null during the round countdown, 
			// then bring the bastard back... 
			if ( m_hCombatants[i] != NULL && !m_hCombatants[i]->IsAlive() && pPlayer->m_bHasDisconnected != TRUE )
			{
				pPlayer->Spawn();
				//Miagi Stop floaters
				pPlayer->StopObserver();
			}
			// end .ASM
			*/

				//Miagi stop observer the IF shouldn't be needed, but I'd rather be safe
				if ( m_hCombatants[i] != NULL && pPlayer->m_bHasDisconnected != TRUE )		
				{
				pPlayer->pev->iuser1=0;
				pPlayer->pev->iuser2=0;
				}


				//miagi for 2v2
				if ( pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE && m_hCombatants[i] != NULL )
				{
				// Get the players in the battle

				// Set scoreboard
		
				char sz[128];
				snprintf(sz, sizeof(sz), "Arena %d", pev->groupinfo);
				MESSAGE_BEGIN( MSG_ALL, gmsgTeamInfo );
				WRITE_BYTE( pPlayer->entindex() );
				WRITE_STRING( sz );
				MESSAGE_END();

				//UTIL_ClientPrintAll( HUD_PRINTNOTIFY, "Hit the 2v2 set scoreboard" );
				}
				//end miagi

			if (m_hCombatants[i])
				m_hCombatants[i]->pev->maxspeed = 320;
		}
	}

	m_iSecondsTillStart--;

	// Play countdown VOX
	if (m_iSecondsTillStart < 5)
	{
		// Speech
		for ( int i = 0; i < m_iPlayersPerTeam * 2; i++ )
		{
			if (m_hCombatants[i])
				((CBasePlayer*)(CBaseEntity*)m_hCombatants[i])->ClientHearVox( g_szCountDownVox[ m_iSecondsTillStart ] );
		}
	}

	// Send the message to the clients in the arena
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE)
		{
			MESSAGE_BEGIN( MSG_ONE, gmsgStartRnd, nullptr, pPlayer->edict() );
				WRITE_BYTE( m_iCurrRound );
				WRITE_BYTE( m_iSecondsTillStart );
				WRITE_BYTE( 0 );
			MESSAGE_END();
		}
	}

	if (m_iSecondsTillStart)
	{
		pev->nextthink = gpGlobals->time + 1.0f;
	}
	else
	{
		m_iArenaState = ARENA_BATTLE_IN_PROGRESS;

		// Enable powerups
		CBaseEntity *pFunc = nullptr;
		while ((pFunc = UTIL_FindEntityByClassname( pFunc, "item_powerup" )) != nullptr)
		{
			((CDiscwarPowerup*)pFunc)->Enable();
		}

		pev->nextthink = gpGlobals->time + 1.0f;
		SetThink( &CDiscArena::BattleThink );
	}
}

//-----------------------------------------------------------------------------
// Purpose: A player in the battle has died. See if we need to restart the battle.
//-----------------------------------------------------------------------------
void CDiscArena::PlayerKilled( CBasePlayer *pPlayer )
{
	// Check to see if the battle's over in 5 seconds
	if ( m_iArenaState == ARENA_BATTLE_IN_PROGRESS || m_iArenaState == ARENA_COUNTDOWN )
	{
		if ( !CheckBattleOver() )
		{
				//Play with this to see if white disc glitch can be fixed - desNotes
			pev->nextthink = gpGlobals->time + 0.5f;
			SetThink( &CDiscArena::CheckOverThink );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: A player in the battle has respawned. Move them to observer mode.
//-----------------------------------------------------------------------------
void CDiscArena::PlayerRespawned( CBasePlayer *pPlayer )
{
	if ( m_iArenaState == ARENA_BATTLE_IN_PROGRESS )
	{
		// Move the player into Spectator mode
		MoveToSpectator( pPlayer );
	}
	else if ( m_iArenaState == ARENA_SHOWING_SCORES && ( m_iTeamOneScore >= m_iMaxRounds || m_iTeamTwoScore >= m_iMaxRounds ) )
	{
		// Battle is over. If there's only 2 players in the game, just respawn.
		// Otherwise, move to spectator.
		int iNumPlayers = 0;
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			const CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );
			if (pPlayer && pPlayer->m_bHasDisconnected != TRUE)
				iNumPlayers++;
		}

		// Move the player into Spectator mode if there are more players
		if ( iNumPlayers > 2 )
			MoveToSpectator( pPlayer );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Move the player to a spectating position
//-----------------------------------------------------------------------------
void CDiscArena::MoveToSpectator( CBasePlayer *pPlayer )
{
	// Find the spectator spawn position
	const CBaseEntity *pSpot = UTIL_FindEntityByClassname(nullptr, "info_player_spectator");
	if ( pSpot )
	{
		pPlayer->StartObserver( pSpot->pev->origin, pSpot->pev->angles);
	}
	else
	{
		// Find any spawn point
		edict_t *pentSpawnSpot = EntSelectSpawnPoint( pPlayer );
		pPlayer->StartObserver( VARS(pentSpawnSpot)->origin, VARS(pentSpawnSpot)->angles);
	}

	//Miagi Causes white disc! This sets the team zero for people spectating
	//pPlayer->pev->team = 0;

	//Miagi This is not needed and is annoying.
	//pPlayer->Observer_SetMode( OBS_LOCKEDVIEW );
}

//-----------------------------------------------------------------------------
// Purpose: Battle is over.
//-----------------------------------------------------------------------------
void CDiscArena::BattleOver()
{
	pev->nextthink = gpGlobals->time + 3;
	SetThink( &CDiscArena::FinishedThink );

	m_iSecondsTillStart = ARENA_TIME_VIEWSCORES;
	m_iArenaState = ARENA_SHOWING_SCORES;

	RestoreWorldObjects();
}

//-----------------------------------------------------------------------------
// Purpose: Round timelimit has been hit
//-----------------------------------------------------------------------------
void CDiscArena::TimeOver()
{
	// Display the 10 second warning first
	if ( !m_bShownTimeWarning )
	{
		m_bShownTimeWarning = TRUE;

		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			const CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE)
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Time_Warning" );
		}

		pev->nextthink = gpGlobals->time + 10;
	}
	else
	{
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			const CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE )
				ClientPrint( pPlayer->pev, HUD_PRINTCENTER, "#Time_Over" );
		}

		// Increment both scores to force the game to end
		m_iTeamOneScore++;
		m_iTeamTwoScore++;

		BattleOver();
	}
}

bool CDiscArena::CheckBattleOver()
{
	bool bTeamOneAlive = false;
	bool bTeamTwoAlive = false;

	// See if the battle is finished
	int i;
	for ( i = 0; i < m_iPlayersPerTeam * 2; i++ )
	{
		if ( m_hCombatants[i] != NULL && ((CBasePlayer*)(CBaseEntity*)m_hCombatants[i])->IsAlive() )
		{
			if ( m_hCombatants[i]->pev->team == 1 )
				bTeamOneAlive = true;
			else if ( m_hCombatants[i]->pev->team == 2 )
				bTeamTwoAlive = true;
		}
	}

	if ( !bTeamOneAlive || !bTeamTwoAlive )
	{
		// Battle is finished.
		if (bTeamOneAlive)
		{
			m_iWinningTeam = 1;
			m_iTeamOneScore++;
		}
		else
		{
			m_iWinningTeam = 2;
			m_iTeamTwoScore++;
		}

		int iTeamInTheLead = 0;
		if ( m_iTeamOneScore > m_iTeamTwoScore )
			iTeamInTheLead = 1;
		else if ( m_iTeamOneScore < m_iTeamTwoScore )
			iTeamInTheLead = 2;

		// Send the message to the clients in the arena
		for ( int iPlayerNum = 1; iPlayerNum <= gpGlobals->maxClients; iPlayerNum++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer*)UTIL_PlayerByIndex( iPlayerNum );

			if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE )
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgEndRnd, nullptr, pPlayer->edict() );
					WRITE_BYTE( m_iCurrRound );
					WRITE_BYTE( 1 );
					WRITE_BYTE( m_iPlayersPerTeam );

					// Send down the winners of this round
					for (i = 0; i < m_iPlayersPerTeam * 2; i++)
					{
						CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];
						if ( !pPlayer || pPlayer->pev->team != m_iWinningTeam )
							continue;

						WRITE_SHORT( pPlayer->entindex() );
					}

					// Send down the team who's winning the battle now
					if ( iTeamInTheLead == 0 )
					{
						// It's a draw at the moment.
						// No need to send down player data.
						WRITE_BYTE( 0 );
					}
					else
					{
						WRITE_BYTE( m_iPlayersPerTeam );
						// Send down the winners of this round
						for (i = 0; i < m_iPlayersPerTeam * 2; i++)
						{
							const CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];
							if ( !pPlayer || pPlayer->pev->team != iTeamInTheLead )
								continue;

							WRITE_SHORT( m_hCombatants[i]->entindex() );
						}
					}

					// Send down the scores
					if ( iTeamInTheLead == 1 )
					{
						WRITE_BYTE( m_iTeamOneScore );
						WRITE_BYTE( m_iTeamTwoScore );
					}
					else
					{
						WRITE_BYTE( m_iTeamTwoScore );
						WRITE_BYTE( m_iTeamOneScore );
					}

					// Send down over or not
					if ( m_iTeamOneScore == m_iMaxRounds || m_iTeamTwoScore == m_iMaxRounds )
						WRITE_BYTE( 1 );
					else
						WRITE_BYTE( 0 );

				MESSAGE_END();
			}
		}

		BattleOver();
		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Check to see if the Battle is over
//-----------------------------------------------------------------------------
void CDiscArena::CheckOverThink()
{
	if ( !CheckBattleOver() )
	{
		if ( m_iArenaState == ARENA_COUNTDOWN )
		{
			pev->nextthink = gpGlobals->time + 0.1f;
			SetThink( &CDiscArena::CountDownThink );
		}
		else
		{
			pev->nextthink = gpGlobals->time + 0.1f;
			SetThink( &CDiscArena::BattleThink );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Show who won, and then restart the round
//-----------------------------------------------------------------------------
void CDiscArena::FinishedThink()
{
	m_iSecondsTillStart--;

	if (m_iSecondsTillStart)
	{
		pev->nextthink = gpGlobals->time + 1.0f;
	}
	else
	{
		SetThink( NULL );

		// Tell the clients to remove the "Won" window
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

			if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE)
			{
				MESSAGE_BEGIN( MSG_ONE, gmsgEndRnd, nullptr, pPlayer->edict() );
					WRITE_BYTE( m_iCurrRound );
					WRITE_BYTE( 0 );
					WRITE_BYTE( 0 );
				MESSAGE_END();
			}
		}

		// Round is over. See if the match is over too.
		if ( m_iTeamOneScore >= m_iMaxRounds || m_iTeamTwoScore >= m_iMaxRounds )
		{
			// Remove the losers from the combatants list
			for (int i = 0; i < m_iPlayersPerTeam * 2; i++)
			{
				CBasePlayer *pPlayer = (CBasePlayer*)(CBaseEntity*)m_hCombatants[i];
				if (!pPlayer)
					continue;
				if ( pPlayer->pev->team == m_iWinningTeam )
				{
					pPlayer->m_iDeaths += 1;
					pPlayer->m_iLastGameResult = GAME_WON;
					continue;
				}

				pPlayer->m_iLastGameResult = GAME_LOST;
				m_hCombatants[i] = nullptr;
			}

			// Then start the next Battle
			PostBattle();
		}
		else
		{
			// Start the next round
			StartRound();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Spawn a player in this battle
//-----------------------------------------------------------------------------
void CDiscArena::SpawnCombatant( CBasePlayer *pPlayer )
{
	// Make sure she's out of spectator mode
	pPlayer->StopObserver();
	
	// Spawn
	g_pGameRules->GetPlayerSpawnSpot( pPlayer );

	// Prevent movement for a couple of seconds
	pPlayer->pev->maxspeed = 1;
}

//-----------------------------------------------------------------------------
// Purpose: Start a Battle
//-----------------------------------------------------------------------------
void CDiscArena::StartBattleThink()
{
	StartBattle();
}

//-----------------------------------------------------------------------------
// Purpose: Prevent firing during prematch
//-----------------------------------------------------------------------------
bool CDiscArena::AllowedToFire()
{
	return m_iArenaState == ARENA_BATTLE_IN_PROGRESS;
}

//-----------------------------------------------------------------------------
// Purpose: New client was added to the arena
//-----------------------------------------------------------------------------
void CDiscArena::AddClient( CBasePlayer *pPlayer, BOOL bCheckStart )
{
	// Remove them from any arena they're currently in
	if ( pPlayer->m_pCurrentArena != nullptr)
		pPlayer->m_pCurrentArena->RemoveClient( pPlayer );

	m_iPlayers++;

	pPlayer->pev->groupinfo = pev->groupinfo;

	// Add her to the queue
	AddPlayerToQueue( pPlayer );
	pPlayer->m_pCurrentArena = this;

	// Only check a restart if the flag is set
	if ( bCheckStart )
	{
		// Start a game if there's none going, and we now have enough players
		// Allow starting of games if there aren't enough players, but there's more than 1 on the svr.
		//if ( (m_iPlayersPerTeam > 1) && (m_iPlayers > 1) && (m_iPlayers < (m_iPlayersPerTeam * 2)) )
			//m_iPlayersPerTeam = 1;
		// If we're in a battle, and the players-per-team isn't the map's setting, restart the battle
		if ( m_iArenaState == ARENA_WAITING_FOR_PLAYERS && m_iPlayers >= m_iPlayersPerTeam * 2 )
		{
			// Start a battle in a second to let the clients learn about this new player
			SetThink( &CDiscArena::StartBattleThink );
			pev->nextthink = gpGlobals->time + 1.0f;
		}
		else
		{
			// Move her into spectator mode
			MoveToSpectator( pPlayer );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Client was removed from the arena
//-----------------------------------------------------------------------------
void CDiscArena::RemoveClient( CBasePlayer *pPlayer )
{
	m_iPlayers--;

	pPlayer->pev->groupinfo = 0;
	pPlayer->m_pCurrentArena = nullptr;

	// Is she in the current battle?
	if ( pPlayer->pev->playerclass != 0 )
	{
		// No, she's in the queue, so remove her.
		RemovePlayerFromQueue( pPlayer );
	}
	else if ( m_iArenaState != ARENA_WAITING_FOR_PLAYERS )
	{
		// This team loses
		m_iWinningTeam = pPlayer->pev->team == 1 ? 2 : 1;
		if ( m_iWinningTeam == 1 )
			m_iTeamOneScore = m_iMaxRounds - 1;		// -1 because we'll get 1 point for winning this round in CheckOverThink
		else
			m_iTeamTwoScore = m_iMaxRounds - 1;		// -1 because we'll get 1 point for winning this round in CheckOverThink

		// Find the player in the combatant list
		for ( int i = 0; i < m_iPlayersPerTeam * 2; i++ )
		{
			// Check to see if this slot's already full
			if ( m_hCombatants[ i ] == pPlayer )
			{
				m_hCombatants[i] = nullptr;
				break;
			}
		}

		CheckOverThink();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Add a player to the end of the queue
//-----------------------------------------------------------------------------
void CDiscArena::AddPlayerToQueue( CBasePlayer *pPlayer )
{
	if ( !pPlayer )
		return;

	if ( m_pPlayerQueue )
	{
		CBasePlayer *pCurr = (CBasePlayer*)(CBaseEntity*)m_pPlayerQueue;
		while ( pCurr->m_pNextPlayer )
		{
			pCurr = (CBasePlayer*)(CBaseEntity*)pCurr->m_pNextPlayer;
		}

		pCurr->m_pNextPlayer = pPlayer;
		pPlayer->pev->playerclass = pCurr->pev->playerclass + 1;
	}
	else
	{
		m_pPlayerQueue = pPlayer;
		pPlayer->pev->playerclass = 1;
	}

	pPlayer->m_pNextPlayer = nullptr;
}

//-----------------------------------------------------------------------------
// Purpose: Remove a player from the queue
//-----------------------------------------------------------------------------
void CDiscArena::RemovePlayerFromQueue( CBasePlayer *pPlayer )
{
	bool bFoundHer = false;

	if ( !pPlayer )
		return;

	CBasePlayer *pCurr = (CBasePlayer*)(CBaseEntity*)m_pPlayerQueue;
	CBasePlayer *pPrev = nullptr;
	
	while ( pCurr )
	{
		if (pCurr == pPlayer )
		{
			bFoundHer = true;
			if (pPrev)
				pPrev->m_pNextPlayer = pCurr->m_pNextPlayer;
			else
				m_pPlayerQueue = pCurr->m_pNextPlayer;
		}
		else 
		{
			pPrev = pCurr;

			// Adjust all the following player's queue positions
			if (bFoundHer)
				pCurr->pev->playerclass--;
		}

		pCurr = (CBasePlayer*)(CBaseEntity*)pCurr->m_pNextPlayer;
	}

	pPlayer->m_pNextPlayer = nullptr;
	pPlayer->pev->playerclass = 0;
}

//-----------------------------------------------------------------------------
// Purpose: Get the next player from the queue, and shuffle the rest up
//-----------------------------------------------------------------------------
CBasePlayer *CDiscArena::GetNextPlayer()
{
	if ( m_pPlayerQueue == NULL )
		return nullptr;

	CBasePlayer *pCurr = (CBasePlayer*)(CBaseEntity*)m_pPlayerQueue;
	RemovePlayerFromQueue( (CBasePlayer*)(CBaseEntity*)m_pPlayerQueue );

	return pCurr;
}

//-----------------------------------------------------------------------------
// Returns TRUE if the Arena is full
int CDiscArena::IsFull()
{
	if ( m_iPlayers < m_iPlayersPerTeam * 2 )
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Returns the first player in the Arena's queue, if any
CBasePlayer *CDiscArena::GetFirstSparePlayer()
{
	if ( m_pPlayerQueue == NULL )
		return nullptr;

	return (CBasePlayer*)(CBaseEntity*)m_pPlayerQueue;
}

//-----------------------------------------------------------------------------
// Add a client to an Arena. Find the first arena that doesn't have two
// players in it, and add this player to that. If we find a third player
// Spectating another arena, grab them and add them to this player's arena.

//Modifying process to put new player in sepctate mode so reconnectors
//won't get to the front of the line - .asm 06/03/03
void AddClientToArena( CBasePlayer *pPlayer )
{
	// First, find an arena for this player to be put into
	for (int i = 0; i < MAX_ARENAS; i++)
	{
		if ( g_pArenaList[i]->IsFull() == FALSE )
		{
			int iArenaNumber = i;

			g_pArenaList[iArenaNumber]->AddClient( pPlayer, TRUE );

			// Now, if this arena's not full, try to find more player to join her
			//Commented out - .asm 06/03/03
		/*	while ( (g_pArenaList[iArenaNumber]->IsFull() == FALSE) && bFoundOne )
			{
				bFoundOne = FALSE;

				// Cycle through all the arenas and find a spare player
				for (int j = 0; j < MAX_ARENAS; j++)
				{
					CBasePlayer *pSparePlayer = g_pArenaList[j]->GetFirstSparePlayer();
					if (pSparePlayer && pSparePlayer != pPlayer)
					{
						g_pArenaList[j]->RemoveClient( pSparePlayer );
						g_pArenaList[iArenaNumber]->AddClient( pSparePlayer, TRUE );
						bFoundOne = TRUE;
						break;
					}
				}
			}	*/

			// If we couldn't find another player for this arena, just add them to an existing arena
			if ( g_pArenaList[iArenaNumber]->IsFull() == FALSE )
			{
				// Add to the first full arena 
				for (int j = 0; j < MAX_ARENAS; j++)
				{
					if ( g_pArenaList[j]->IsFull() )
					{
						// Remove from current
						g_pArenaList[iArenaNumber]->RemoveClient( pPlayer );

						// Add to full one
						iArenaNumber = j;
						g_pArenaList[iArenaNumber]->AddClient( pPlayer, TRUE );
						break;
					}
				}
			}
			//Beginning of added code to prevent reconnectors - .asm 06/03/03
			else
			{
		bool bFoundOne = FALSE;

		// Cycle through all the arenas and find a spare player
		for (int j = 0; j < MAX_ARENAS; j++)
		{
			CBasePlayer *pSparePlayer = g_pArenaList[j]->GetFirstSparePlayer();
			if (pSparePlayer && pSparePlayer != pPlayer)
			{
				g_pArenaList[j]->RemoveClient( pSparePlayer );
				g_pArenaList[iArenaNumber]->AddClient( pSparePlayer, TRUE );
				bFoundOne = TRUE;
				break;
			}
		}
	} 

		//End of added code to prevent reconnectors - .asm 06/03/03		


			//ALERT( at_console, "ADDED %s to Arena %d\n", STRING(pPlayer->pev->netname), iArenaNumber );
			return;
		}
	}
}

//-----------------------------------------------------------------------------
// Put the specified group of players into the current arena
int AddPlayers( int iPlayers, int iArenaNum )
{
	for ( int i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->m_pCurrentArena == nullptr && pPlayer->m_bHasDisconnected != TRUE )
		{
			if ( pPlayer->m_iLastGameResult != iPlayers )
				continue;

			g_pArenaList[iArenaNum]->AddClient( pPlayer, FALSE );
			if ( g_pArenaList[iArenaNum]->IsFull() )
				iArenaNum++;
		}
	}

	return iArenaNum;
}

//-----------------------------------------------------------------------------
// Take all the players not in battles and shuffle them into new groups
void ShufflePlayers()
{
	int iArenaNum = 0;
	int i;
	// Reset all Arenas
	for ( i = 0; i < MAX_ARENAS; i++)
	{
		g_pArenaList[i]->Reset();
	}

	// Play the winners off against the other winners first
	iArenaNum = AddPlayers( GAME_WON, iArenaNum );
	// First, add the players who didn't play at all
	iArenaNum = AddPlayers( GAME_DIDNTPLAY, iArenaNum );
	// Then add the losers
	iArenaNum = AddPlayers( GAME_LOST, iArenaNum );

	// Then tell all full arenas to start
	for (i = 0; i < MAX_ARENAS; i++)
	{
		if ( g_pArenaList[i]->IsFull() )
		{
			g_pArenaList[i]->StartBattle();
		}
		else
		{
			// If the arena has players in it, move them to the first full arena
			if ( g_pArenaList[i]->m_iPlayers != 0 )
			{
				for ( int j = 1; j <= gpGlobals->maxClients; j++ )
				{
					CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( j );

					if (pPlayer && pPlayer->m_pCurrentArena == g_pArenaList[i] && pPlayer->m_bHasDisconnected != TRUE )
					{
						// Add to the first arena
						g_pArenaList[0]->AddClient( pPlayer, TRUE );

						pPlayer->m_iLastGameResult = GAME_DIDNTPLAY;
					}
				}
			}
		}
	}

}

//-----------------------------------------------------------------------------
// The Battle is over. First, check to see if there are games going on in
// other arenas. If there are, these players go watch one of them for a bit.
// If all games finish in that time, shuffle all the players and start new
// games. Otherwise, shuffle all the players who aren't still playing, and
// start new games with them.
void CDiscArena::PostBattle()
{
	int iOtherGame = -1;
	int i;
	// First, see if there are any other games going on in other arenas
	for (i = 0; i < MAX_ARENAS; i++)
	{
		if ( g_pArenaList[i]->m_iArenaState != ARENA_WAITING_FOR_PLAYERS && g_pArenaList[i] != this )
		{
			iOtherGame = i;
			break;
		}
	}

	// If there are no other games, just shuffle and start again
	if ( iOtherGame == -1 )
	{
		ShufflePlayers();
		return;
	}

	// There's another game going on. Move all the players from this arena to it to spectate.
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CBasePlayer *pPlayer = (CBasePlayer *)UTIL_PlayerByIndex( i );

		if (pPlayer && pPlayer->pev->groupinfo & pev->groupinfo && pPlayer->m_bHasDisconnected != TRUE )
		{
			g_pArenaList[ iOtherGame ]->AddClient( pPlayer, TRUE );
		}
	}

	m_iArenaState = ARENA_WAITING_FOR_PLAYERS;
}
