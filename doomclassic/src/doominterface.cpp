/*
===========================================================================

Doom 3 BFG Edition GPL Source Code
Copyright (C) 1993-2012 id Software LLC, a ZeniMax Media company. 

This file is part of the Doom 3 BFG Edition GPL Source Code ("Doom 3 BFG Edition Source Code").  

Doom 3 BFG Edition Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Doom 3 BFG Edition Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Doom 3 BFG Edition Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Doom 3 BFG Edition Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Doom 3 BFG Edition Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#include "Precompiled.h"

#include "globaldata.h"
#include "doominterface.h"
#include "sys/win32/win_local.h"

int ClassicWinMain(HINSTANCE hInstance, HINSTANCE nothing, LPSTR cmdline, int nCmdShow);

#define GLOBAL_IMAGE_SCALER	3

#define ORIGINAL_WIDTH		320
#define ORIGINAL_HEIGHT		200

#define WIDTH				( ORIGINAL_WIDTH * GLOBAL_IMAGE_SCALER )
#define HEIGHT				( ORIGINAL_HEIGHT * GLOBAL_IMAGE_SCALER )

#define SCREENWIDTH			WIDTH
#define SCREENHEIGHT		HEIGHT

extern Globals *globaldata[4];

DoomInterface::DoomInterface() {
	// numplayers = 0;
	// bFinished[0] = bFinished[1] = bFinished[2] = bFinished[3] = false;
	// lastTicRun = 0;
}

DoomInterface::~DoomInterface() {
	if (g)
		delete[] ::g->screens[0];
}


// Carl: needed for Doom 3 BFG Edition's Common.cpp
void DoomInterface::Startup(int playerscount, bool multiplayer)
{
	int i;
#if 0
	int localdargc = 1; // for the commandline

	numplayers			= playerscount;
	globalNetworking	= multiplayer;
	lastTicRun			= 0;

	if (DoomLib::Z_Malloc == NULL) {
		DoomLib::Z_Malloc = Z_Malloc;
	}

	// Splitscreen
	if ( !multiplayer && playerscount > 1 ) {
		localdargc += 2; // for the '-net' and the console number
		localdargc += playerscount;
	}

	if ( multiplayer ) {
		// Force online games to 1 local player for now.
		// TODO: We should support local splitscreen and online.
		numplayers = 1;
	}
#endif
	// Start up DooM Classic
	for ( i = 0; i < 1; ++i)
	{
		DoomLib::SetPlayer(i);

		//bFinished[i] = false;
		DoomLib::InitGlobals( NULL );
#if 0
		if ( globalNetworking ) {
			printf( "Starting mulitplayer game, argv = " );
			for ( int j = 0; j < mpArgc[0]; ++j ) {
				printf( " %s", mpArgVPtr[0][j] );
			}
			printf( "\n" );
			DoomLib::InitGame(mpArgc[i], mpArgVPtr[i] );
		} else {
			DoomLib::InitGame(localdargc, (char **)dargv[i] );
		}

		if( DoomLib::skipToLoad ) {
			G_LoadGame( DoomLib::loadGamePath );
			 DoomLib::skipToLoad = false;
			 ::g->menuactive = 0;
		}

		if( DoomLib::skipToNew ) {
			static int startLevel = 1;
			G_DeferedInitNew((skill_t)DoomLib::chosenSkill,DoomLib::chosenEpisode+1, startLevel);
			DoomLib::skipToNew = false;
			::g->menuactive = 0;
		}
#endif
		DoomLib::SetPlayer(-1);
	}
	byte*	base;

	base = (byte*) new byte[SCREENWIDTH * SCREENHEIGHT * 4];
	memset(base, 0, SCREENWIDTH * SCREENHEIGHT * 4);
	::g = (Globals *)DoomLib::GetGlobalData( 0 );
	for (i = 0; i < 4; i++)
	{
		::g->screens[i] = base + i * SCREENWIDTH * SCREENHEIGHT;
	}
	ClassicWinMain(win32.hInstance, NULL, "", SW_SHOW);
}

// Carl: needed for Doom 3 BFG Edition's Common.cpp
bool DoomInterface::Frame(int iTime, idUserCmdMgr * userCmdMgr)
{
#if 0
	int i;
	bool bAllFinished = true;

	if ( !globalNetworking || ( lastTicRun < iTime ) ) {

		drawFullScreen = false;

		DoomLib::SetPlayer( 0 );
		DoomLib::PollNetwork();

		for (i = 0; i < numplayers; ++i)
		{
			DoomLib::SetPlayer( i );

			I_SetTime( iTime );

			if (bFinished[i] == false) {
				bAllFinished = false;
				bFinished[i] = DoomLib::Poll();
			} else {

				if (::g->wipedone) {
					if ( !waitingForWipe ) {
						const bool didRunTic = DoomLib::Tic( userCmdMgr );
						if ( didRunTic == false ) {
							//printf( "Skipping tic and yielding because not enough time has passed.\n" );
						
							// Give lower priority threads a chance to run.
							Sys_Yield();
						}
					}
					DoomLib::Frame();
				}
				if (::g->wipe) {
					DoomLib::Wipe();
					// Draw the menus over the wipe.
					M_Drawer();
				}

				if( ::g->gamestate != GS_LEVEL && GetNumPlayers() > 2 ) {
					drawFullScreen = true;
				}
			}

			DoomLib::SetPlayer(-1);
		}

		DoomLib::SetPlayer( 0 );
		DoomLib::SendNetwork();
		DoomLib::RunSound();
		DoomLib::SetPlayer( -1 );

		lastTicRun = iTime;
	} else {
		printf( "Skipping this frame becase it's not time to run a tic yet.\n" );
	}

	return bAllFinished;
#endif
	return true;
}

// Carl: needed for Doom 3 BFG Edition's Common.cpp
void DoomInterface::Shutdown() {
#if 0	
	int i;

	for ( i=0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );
		D_QuitNetGame();
	}

	// Shutdown local network state
	I_ShutdownNetwork();

	for ( i=0; i < numplayers; i++ ) {
		DoomLib::SetPlayer( i );
		DoomLib::Shutdown();
	}

	DoomLib::SetPlayer( -1 );
	numplayers = 0;
	lastTicRun = 0;
#endif
}

