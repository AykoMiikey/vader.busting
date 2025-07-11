#include "ExtendedEsp.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../source.hpp"
#include "ESP.hpp"

class CExtendedSound : public IExtendedEsp {
public:
   // Call before and after ESP.
   void Start( ) override;
   void Finish( ) override;

private:
   void AdjustPlayerBegin( C_CSPlayer* player );
   void AdjustPlayer(C_CSPlayer* player);
   void AdjustPlayerFinish( );
   void SetupAdjustPlayer( C_CSPlayer* player, SndInfo_t& sound );

   bool ValidSound( SndInfo_t& sound );

   CUtlVector<SndInfo_t> m_utlvecSoundBuffer;
   std::vector<SoundPlayer> m_arRestorePlayers;
};

Encrypted_t<IExtendedEsp> IExtendedEsp::Get( ) {
   static CExtendedSound instance;
   return &instance;
}

void CExtendedSound::Start( ) {
	if (!g_Vars.esp.fade_esp)
		return;

   CUtlVector<SndInfo_t> m_utlCurSoundList;
   Interfaces::m_pEngineSound->GetActiveSounds( m_utlCurSoundList ); 
   
   // No active sounds.
   if ( !m_utlCurSoundList.Count( ) )
	  return;

   C_CSPlayer* LocalPlayer = C_CSPlayer::GetLocalPlayer( );

   static auto flSpawnTime = 0.f;

   if ( !LocalPlayer )
	  return;

   if ( flSpawnTime != LocalPlayer->m_flSpawnTime( ) ) {
	  flSpawnTime = LocalPlayer->m_flSpawnTime( );
	  return;
   }

   // Accumulate sounds for esp correction
   for ( int iter = 0; iter < m_utlCurSoundList.Count( ); iter++ ) {
	  SndInfo_t& sound = m_utlCurSoundList[ iter ];
	  if ( sound.m_nSoundSource == 0 || // World
		 sound.m_nSoundSource > 64 )   // Most likely invalid
		 continue;

	  C_CSPlayer* player = C_CSPlayer::GetPlayerByIndex(sound.m_nSoundSource);

	  if ( !player || !sound.m_pOrigin || !player->IsPlayer() || player == LocalPlayer || player->IsTeammate(LocalPlayer) || sound.m_pOrigin->IsZero( ) )
		 continue;

	  if ( !ValidSound( sound ) )
		 continue;

	  SetupAdjustPlayer( player, sound );

	  m_cSoundPlayers[ sound.m_nSoundSource ].Override( sound );
   }

   for ( int iter = 1; iter < 64; ++iter ) {
	  C_CSPlayer* player = C_CSPlayer::GetPlayerByIndex(iter);
	  if ( !player || !player->IsPlayer() || player->IsDead( ) ) 
		 continue;	  

	  AdjustPlayerBegin(player);

	  if (!player->IsDormant()) {
		  for (auto& player : m_arRestorePlayers) {
			  if (player.playerindex == iter) {
				  player.m_bDormant = false;
			  }
		  }
	  }
	  else {
		  AdjustPlayer(player);
	  }
   }

   m_utlvecSoundBuffer = m_utlCurSoundList;
}

void CExtendedSound::Finish( ) {
	if( !g_Vars.esp.fade_esp )
		return;
	
	// Do any finishing code here. If we add smtn like sonar radar this will be useful.
   AdjustPlayerFinish( );
}

void CExtendedSound::AdjustPlayerBegin( C_CSPlayer* player ) {
	if (!player)
		return;

   // Adjusts player's origin and other vars so we can show full-ish esp.
   constexpr int EXPIRE_DURATION = 450; // miliseconds-ish?
   auto& sound_player = m_cSoundPlayers[ player->EntIndex( ) ];
   bool sound_expired = GetTickCount( ) - sound_player.m_iReceiveTime > EXPIRE_DURATION;
   if ( sound_expired )
	  return;

   SoundPlayer current_player;
   current_player.playerindex = player->m_entIndex;
   current_player.m_bDormant = true;
   current_player.m_nFlags = player->m_fFlags( ); 
   current_player.m_vecOrigin = player->m_vecOrigin( );
   current_player.m_vecAbsOrigin = player->GetAbsOrigin( );

   m_arRestorePlayers.emplace_back( current_player );
}

void CExtendedSound::AdjustPlayer(C_CSPlayer* player)
{
	if (!player)
		return;

	// Adjusts player's origin and other vars so we can show full-ish esp.
	constexpr int EXPIRE_DURATION = 450; // miliseconds-ish?
	auto& sound_player = m_cSoundPlayers[player->EntIndex()];
	bool sound_expired = GetTickCount() - sound_player.m_iReceiveTime > EXPIRE_DURATION;
	if (sound_expired)
		return;

	if (!sound_expired)
		*(bool*)((uintptr_t)player + 0xED) = false; // dormant check

	 //printf( "player %i seted\n", sound_player.m_iIndex );
	player->m_fFlags() = sound_player.m_nFlags;
	player->m_vecOrigin() = sound_player.m_vecOrigin;
	player->SetAbsOrigin(sound_player.m_vecOrigin);
}

void CExtendedSound::AdjustPlayerFinish( ) {
   // Restore and clear saved players for next loop.
   for ( auto& RestorePlayer : m_arRestorePlayers ) {
	  auto player = C_CSPlayer::GetPlayerByIndex( RestorePlayer.playerindex );
	  if ( !player )
		 continue;

     //printf( "player %i restored\n", player->entindex( ) );
	//  player->m_fFlags( ) = RestorePlayer.m_nFlags;
	//  player->m_vecOrigin( ) = RestorePlayer.m_vecOrigin;
	//  player->SetAbsOrigin( RestorePlayer.m_vecAbsOrigin );
	  *( bool* ) ( ( uintptr_t ) player + 0xED ) = RestorePlayer.m_bDormant; // dormant check
   }

   m_arRestorePlayers.clear( );
}

void CExtendedSound::SetupAdjustPlayer(C_CSPlayer* player, SndInfo_t& sound) {
	if (!player)
		return;

	Vector src3D, dst3D;
	CGameTrace tr;
	Ray_t ray;
	CTraceFilter filter;

	filter.pSkip = player;
	src3D = (*sound.m_pOrigin) + Vector(0, 0, 1); // So they dont dig into ground incase shit happens /shrug
	dst3D = src3D - Vector(0, 0, 100);
	ray.Init(src3D, dst3D);

	Interfaces::m_pEngineTrace->TraceRay(ray, MASK_PLAYERSOLID, &filter, &tr);

	// step = (tr.fraction < 0.20)
	// shot = (tr.fraction > 0.20)
	// stand = (tr.fraction > 0.50)
	// crouch = (tr.fraction < 0.50)

	/* Corrects origin and important flags. */

	// Player stuck, idk how this happened
	if (tr.allsolid) {
		m_cSoundPlayers[sound.m_nSoundSource].m_iReceiveTime = -1;
	}

	//*sound.m_pOrigin = (tr.fraction < 0.97 ? tr.endpos : *sound.m_pOrigin);
	m_cSoundPlayers[ sound.m_nSoundSource ].m_vecOrigin = ( tr.fraction < 0.97 ? tr.endpos : *sound.m_pOrigin );
	m_cSoundPlayers[sound.m_nSoundSource].m_nFlags = player->m_fFlags();
	m_cSoundPlayers[sound.m_nSoundSource].m_nFlags |= (tr.fraction < 0.50f ? FL_DUCKING : 0) | (tr.fraction != 1 ? FL_ONGROUND : 0);   // Turn flags on
	m_cSoundPlayers[sound.m_nSoundSource].m_nFlags &= (tr.fraction > 0.50f ? ~FL_DUCKING : 0) | (tr.fraction == 1 ? ~FL_ONGROUND : 0); // Turn flags off
}

bool CExtendedSound::ValidSound( SndInfo_t& sound ) {
   // Use only server dispatched sounds.
   //if (!sound.m_bFromServer)
   //   return false;

   //  We don't want the sound to keep following client's predicted origin.
   for ( int iter = 0; iter < m_utlvecSoundBuffer.Count( ); iter++ ) {
	  const SndInfo_t& cached_sound = m_utlvecSoundBuffer[ iter ];
	  if ( cached_sound.m_nGuid == sound.m_nGuid ) {
		 return false;
	  }
   }

   return true;
}
