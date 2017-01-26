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
#include "doomlib.h"
#include "sys/sys_session.h"
#include <sys/types.h>

// Carl: needed for compiling in Debug mode
#ifdef _DEBUG
bool	debugOutput			= true;
#else
bool	debugOutput			= false;
#endif

namespace DoomLib
{

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	int classicRemap[K_LAST_KEY];

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	void				  SetCurrentExpansion(int expansion)  {
		expansionDirty = true; 
		// expansionSelected = expansion; 
	}

	// Carl: needed for Doom 3 BFG Edition's common_frame.cpp
	Globals *globaldata[4];

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	DoomInterface					Interface;

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	bool							expansionDirty = true;

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	bool							skipToLoad = false;

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	bool							skipToNew = false;

	// Carl: needed for Doom 3 BFG Edition's Common.cpp
	idMatchParameters				matchParms;
}

// Carl: needed for Doom 3 BFG Edition's common_frame.cpp
void DoomLib::InitGlobals( void *ptr /* = NULL */ )
{
	if (ptr == NULL)
		ptr = new Globals;

	int currentplayer = 0;
	globaldata[currentplayer] = static_cast<Globals*>(ptr);

	memset( globaldata[currentplayer], 0, sizeof(Globals) );
	g = globaldata[currentplayer];
	g->InitGlobals();
	
}

void *DoomLib::GetGlobalData( int player ) {
	return globaldata[player];
}

// Carl: needed for Doom 3 BFG Edition's Common.cpp
keyNum_t DoomLib::RemapControl( keyNum_t key ) {

	if( classicRemap[ key ] == K_NONE ) {
		return key;
	} else {

		//if( ::g->menuactive && ( key == K_JOY2 || key == K_JOY18 ) ) {
		//	return K_BACKSPACE;
		//}

		return (keyNum_t)classicRemap[ key ];
	}

}

// Carl: needed for Doom 3 BFG Edition's Common.cpp
// static
void DoomLib::SetPlayer( int id )
{
#if 0
	currentplayer = id;

	if ( id < 0 || id >= MAX_PLAYERS ) {
		g = NULL;
	}
	else {
	
		// Big Fucking hack.
		if( globalNetworking && session->GetGameLobbyBase().GetMatchParms().matchFlags | MATCH_ONLINE ) {
			currentplayer = 0;
		} 
		
		g = globaldata[currentplayer];
	}
#endif
}

