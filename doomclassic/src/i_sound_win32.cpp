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
#include "i_sound.h"

// Carl: needed for Doom 3 BFG Edition's XA2_SoundHardware.cpp
/*
======================
I_InitSoundHardware

Called from the tech4x initialization code. Sets up Doom classic's
sound channels.
======================
*/
void I_InitSoundHardware( int numOutputChannels_, int channelMask ) {
#if 0
	::numOutputChannels = numOutputChannels_;

	// Initialize the X3DAudio
	//  Speaker geometry configuration on the final mix, specifies assignment of channels
	//  to speaker positions, defined as per WAVEFORMATEXTENSIBLE.dwChannelMask
	//  SpeedOfSound - not used by doomclassic
	X3DAudioInitialize( channelMask, 340.29f, X3DAudioInstance );

	for ( int i = 0; i < NUM_SOUNDBUFFERS; ++i ) {
		// Initialize source voices
		I_InitSoundChannel( i, numOutputChannels );
	}

	I_InitMusic();

	soundHardwareInitialized = true;
#endif
}


// Carl: needed for Doom 3 BFG Edition's XA2_SoundHardware.cpp
/*
======================
I_ShutdownitSoundHardware

Called from the tech4x shutdown code. Tears down Doom classic's
sound channels.
======================
*/
void I_ShutdownSoundHardware() {
#if 0
	soundHardwareInitialized = false;

	I_ShutdownMusic();

	for ( int i = 0; i < NUM_SOUNDBUFFERS; ++i ) {
		activeSound_t * sound = &activeSounds[i];

		if ( sound == NULL ) {
			continue;
		}

		if ( sound->m_pSourceVoice ) {
			sound->m_pSourceVoice->Stop();
			sound->m_pSourceVoice->FlushSourceBuffers();
			sound->m_pSourceVoice->DestroyVoice();
			sound->m_pSourceVoice = NULL;
		}

		if ( sound->m_DSPSettings.pMatrixCoefficients ) {
			delete [] sound->m_DSPSettings.pMatrixCoefficients;
			sound->m_DSPSettings.pMatrixCoefficients = NULL;
		}
	}
#endif
}

