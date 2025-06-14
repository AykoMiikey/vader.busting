#include "KnifeBot.hpp"
#include "../../SDK/Classes/player.hpp"
#include "../../source.hpp"
#include "../../SDK/Classes/weapon.hpp"
#include "LagCompensation.hpp"
#include "TickbaseShift.hpp"

// aimware knifebot
namespace Interfaces
{
	struct KnifeBotData {
		C_CSPlayer* m_pCurrentTarget = nullptr;
		C_CSPlayer* m_pLocalPlayer = nullptr;
		C_WeaponCSBaseGun* m_pLocalWeapon = nullptr;
		Encrypted_t<CCSWeaponInfo> m_pWeaponInfo = nullptr;
		Encrypted_t<CUserCmd> m_pCmd = nullptr;
		Vector m_vecEyePos;
	};

	KnifeBotData _knife_bot_data;

	class CKnifeBot : public KnifeBot {
	public:
		CKnifeBot( ) : knifeBotData( &_knife_bot_data ) { }

		void Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) override;
	private:
		int GetMinimalHp( );
		bool TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket );
		int DeterminateHitType( bool stabType, Vector eyeDelta );

		Encrypted_t<KnifeBotData> knifeBotData;
	};

	KnifeBot* KnifeBot::Get( ) {
		static CKnifeBot instance;
		return &instance;
	}

	void CKnifeBot::Main( Encrypted_t<CUserCmd> pCmd, bool* sendPacket ) {
		if( !Interfaces::m_pEngine->IsInGame( ) || !g_Vars.misc.knife_bot )
			return;

		knifeBotData->m_pLocalPlayer = C_CSPlayer::GetLocalPlayer( );

		if( !knifeBotData->m_pLocalPlayer || knifeBotData->m_pLocalPlayer->IsDead( ) )
			return;

		knifeBotData->m_pLocalWeapon = ( C_WeaponCSBaseGun* )knifeBotData->m_pLocalPlayer->m_hActiveWeapon( ).Get( );

		if( !knifeBotData->m_pLocalWeapon || !knifeBotData->m_pLocalWeapon->IsWeapon( ) )
			return;

		knifeBotData->m_pWeaponInfo = knifeBotData->m_pLocalWeapon->GetCSWeaponData( );

		if( !knifeBotData->m_pWeaponInfo.IsValid( ) || !( knifeBotData->m_pWeaponInfo->m_iWeaponType == WEAPONTYPE_KNIFE ) )
			return;

		knifeBotData->m_pCmd = pCmd;

		if( knifeBotData->m_pLocalPlayer->m_flNextAttack( ) > Interfaces::m_pGlobalVars->curtime || knifeBotData->m_pLocalWeapon->m_flNextPrimaryAttack( ) > Interfaces::m_pGlobalVars->curtime )
			return;

		if( knifeBotData->m_pLocalWeapon->m_iItemDefinitionIndex( ) == WEAPON_ZEUS )
			return;

		knifeBotData->m_vecEyePos = knifeBotData->m_pLocalPlayer->GetEyePosition( );

		for( int i = 1; i <= Interfaces::m_pGlobalVars->maxClients; i++ ) {
			C_CSPlayer* Target = ( C_CSPlayer* )Interfaces::m_pEntList->GetClientEntity( i );
			if( !Target
				|| Target->IsDead( )
				|| Target->IsDormant( )
				|| !Target->IsPlayer( )
				|| Target->m_iTeamNum( ) == knifeBotData->m_pLocalPlayer->m_iTeamNum( )
				|| Target->m_bGunGameImmunity( ) || g_Vars.globals.player_list.white_list[i] )
				continue;

			auto lag_data = Engine::LagCompensation::Get( )->GetLagData( Target->m_entIndex );
			if( !lag_data.IsValid( ) || lag_data->m_History.empty( ) )
				continue;

			Engine::C_LagRecord* previousRecord = nullptr;
			Engine::C_LagRecord backup;
			backup.Setup( Target );
			for( auto& record : lag_data->m_History ) {
				if( !Engine::LagCompensation::Get( )->IsRecordOutOfBounds( record, 0.2f )
					|| record.m_bSkipDueToResolver
					|| !record.m_bIsValid || record.m_bTeleportDistance ) {
					continue;
				}

				if( !previousRecord
					|| previousRecord->m_vecOrigin != record.m_vecOrigin
					|| previousRecord->m_flEyeYaw != record.m_flEyeYaw
					|| previousRecord->m_angAngles.yaw != record.m_angAngles.yaw
					|| previousRecord->m_vecMaxs != record.m_vecMaxs
					|| previousRecord->m_vecMins != record.m_vecMins ) {
					previousRecord = &record;

					record.Apply( Target );
					if( TargetEntity( Target, sendPacket ) ) {
						knifeBotData->m_pCmd->tick_count = TIME_TO_TICKS( record.m_flSimulationTime + Engine::LagCompensation::Get( )->GetLerp( ) );
						break;
					}
				}
			}

			backup.Apply( Target );
		}
	}

	int CKnifeBot::GetMinimalHp( ) {
		if( Interfaces::m_pGlobalVars->curtime > ( knifeBotData->m_pLocalWeapon->m_flNextPrimaryAttack( ) + 0.4f ) )
			return 34;

		return 21;
	}

	bool CKnifeBot::TargetEntity( C_CSPlayer* pPlayer, bool* sendPacket ) {
		knifeBotData->m_pCurrentTarget = pPlayer;

		Vector vecOrigin = pPlayer->m_vecOrigin( );

		Vector vecOBBMins = pPlayer->m_Collision( )->m_vecMins;
		Vector vecOBBMaxs = pPlayer->m_Collision( )->m_vecMaxs;

		Vector vecMins = vecOBBMins + vecOrigin;
		Vector vecMaxs = vecOBBMaxs + vecOrigin;

		Vector vecEyePos = pPlayer->GetEyePosition( );

		Vector vecPos = Math::Clamp( vecMins, vecEyePos, vecMaxs );
		Vector vecDelta = vecPos - knifeBotData->m_vecEyePos;
		vecDelta.Normalize( );

		int attackType = DeterminateHitType( 0, vecDelta );
		if( attackType ) {
			if( g_Vars.misc.knife_bot_type == 1 ) {
				if( attackType == 2 && knifeBotData->m_pCurrentTarget->m_iHealth( ) <= 76 ) {
				first_attack:
					knifeBotData->m_pCmd->viewangles = vecDelta.ToEulerAngles( );
					knifeBotData->m_pCmd->buttons |= IN_ATTACK;
					
					*sendPacket = true;
					
					return true;
				}
			}
			else if( g_Vars.misc.knife_bot_type == 2 ) {
				if( knifeBotData->m_pCurrentTarget->m_iHealth( ) > 46 ) {
					goto first_attack;
				}
			}
			else {
				int hp = attackType == 2 ? 76 : GetMinimalHp( );
				if( hp >= knifeBotData->m_pCurrentTarget->m_iHealth( ) )
					goto first_attack;
			}
		}

		if( !DeterminateHitType( 1, vecDelta ) )
			return false;

		knifeBotData->m_pCmd->viewangles = vecDelta.ToEulerAngles( );
		knifeBotData->m_pCmd->buttons |= IN_ATTACK2;
		
		*sendPacket = true;
		
		return true;
	}

	int CKnifeBot::DeterminateHitType( bool stabType, Vector eyeDelta ) {
		float minDistance = stabType ? 32.0f : 48.0f;

		Vector vecEyePos = knifeBotData->m_vecEyePos;
		Vector vecEnd = vecEyePos + ( eyeDelta * minDistance );
		Vector vecOrigin = knifeBotData->m_pCurrentTarget->m_vecOrigin( );

		CTraceFilter filter;
		filter.pSkip = knifeBotData->m_pLocalPlayer;

		Ray_t ray;
		ray.Init( vecEyePos, vecEnd, Vector( -16.0f, -16.0f, -18.0f ), Vector( 16.0f, 16.0f, 18.0f ) );
		CGameTrace tr;
		Interfaces::m_pEngineTrace->TraceRay( ray, 0x200400B, &filter, &tr );

		if( !tr.hit_entity )
			return 0;

		if( knifeBotData->m_pCurrentTarget ) {
			if( tr.hit_entity != knifeBotData->m_pCurrentTarget )
				return 0;
		}
		else { // guess this only for trigger bot
			C_CSPlayer* ent = ToCSPlayer( tr.hit_entity->GetBaseEntity( ) );

			if( !ent || ent->IsDead( ) || ent->IsDormant( ) )
				return 0;

			if( ent->m_iTeamNum( ) == knifeBotData->m_pLocalPlayer->m_iTeamNum( ) )
				return 0;
		}

		QAngle angles = tr.hit_entity->GetAbsAngles( );
		float cos_pitch = cos( DEG2RAD( angles.x ) );
		float sin_yaw, cos_yaw;
		DirectX::XMScalarSinCos( &sin_yaw, &cos_yaw, DEG2RAD( angles.yaw ) );

		Vector delta = vecOrigin - vecEyePos;
		return ( ( ( cos_yaw * cos_pitch * delta.x ) + ( sin_yaw * cos_pitch * delta.y ) ) >= 0.475f ) + 1;
	}
}
