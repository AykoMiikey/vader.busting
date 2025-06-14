#pragma once
#include "../../source.hpp"

namespace Autowall
{
	class C_FireBulletData {
	public:
		Vector m_vecStart = Vector( 0, 0, 0 );
		Vector m_vecDirection = Vector( 0, 0, 0 );
		Vector m_vecPos = Vector( 0, 0, 0 );

		CGameTrace m_EnterTrace;

		int m_iPenetrationCount = 4;
		int m_iHitgroup = -1;

		float m_flTraceLength;
		float m_flCurrentDamage;

		// input data

		// distance to point
		float m_flMaxLength = 0.0f;
		float m_flPenetrationDistance = 0.0f;

		// should penetrate walls? 
		bool m_bPenetration = false;

		bool m_bShouldDrawImpacts = false;
		FloatColor m_colImpacts;

		bool m_bShouldIgnoreDistance = false;

		CTraceFilter* m_bTraceFilter = nullptr;
		ITraceFilter* m_Filter = nullptr; // TODO: implement
		C_CSPlayer* m_Player = nullptr; // attacker
		C_CSPlayer* m_TargetPlayer = nullptr;  // autowall target ( could be nullptr if just trace attack )
		C_WeaponCSBaseGun* m_Weapon = nullptr; // attacker weapon
		CCSWeaponInfo* m_WeaponData = nullptr;

		surfacedata_t* m_EnterSurfaceData;
	};

	bool IsBreakable( C_BaseEntity* entity );
	bool IsArmored( C_CSPlayer* player, int hitgroup );
	float ScaleDamage( C_CSPlayer* player, float damage, float armor_ratio, int hitgroup );
	void TraceLine( const Vector& start, const Vector& end, uint32_t mask, C_BasePlayer* ignore, CGameTrace* ptr );
	void ClipTraceToPlayers( const Vector& vecAbsStart, const Vector& vecAbsEnd, uint32_t mask, ITraceFilter* filter, CGameTrace* tr, float flMaxRange = 60.0f, float flMinRange = 0.0f );
	void ClipTraceToPlayer( const Vector vecAbsStart, const Vector& vecAbsEnd, uint32_t mask, ITraceFilter* filter, CGameTrace* tr, Encrypted_t<Autowall::C_FireBulletData> data );
	bool TraceToExit( CGameTrace* enter_trace, Vector start, Vector direction, CGameTrace* exit_trace, Vector out );
	bool HandleBulletPenetration( Encrypted_t<C_FireBulletData> data );
	float FireBullets( Encrypted_t<C_FireBulletData> data );
}
