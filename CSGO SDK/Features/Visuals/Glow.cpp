#include "Glow.hpp"
#include "../../source.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"

class C_GlowOutline : public GlowOutline {
public:
	virtual void Render( );
	virtual void Shutdown( );

	C_GlowOutline( ) { };
	virtual ~C_GlowOutline( ) { };
};

GlowOutline* GlowOutline::Get( ) {
	static C_GlowOutline glow;
	return &glow;
}

void C_GlowOutline::Render( ) {
	static bool weapon_enabled = false;

	auto pLocal = C_CSPlayer::GetLocalPlayer( );
	if( !pLocal )
		return;

	if( !pLocal || !Interfaces::m_pEngine->IsInGame( ) )
		return;

	auto players = g_Vars.esp.glow_local || g_Vars.esp.glow_enemy;
	if( !players && !g_Vars.esp.glow_grenade && !g_Vars.esp.glow_weapons )
		return;

	for( auto i = 0; i < Interfaces::m_pGlowObjManager->m_GlowObjectDefinitions.m_Size; i++ ) {
		auto& glowObject = Interfaces::m_pGlowObjManager->m_GlowObjectDefinitions.m_Memory.m_pMemory[ i ];
		auto entity = reinterpret_cast< C_CSPlayer* >( glowObject.m_pEntity );

		if( !&glowObject )
			continue;

		if( glowObject.IsUnused( ) )
			continue;

		if( !entity || entity->IsDormant( ) || entity->IsDead( ) )
			continue;

		auto class_id = entity->GetClientClass( )->m_ClassID;
		auto color = FloatColor{ };

		switch( class_id ) {
		case CCSPlayer:
		{
			if( !players || entity->IsDead( ) )
				continue;

			auto is_enemy = !entity->IsTeammate( pLocal );
			if( ( is_enemy && !g_Vars.esp.glow_enemy ) || ( entity->EntIndex( ) == pLocal->EntIndex( ) && !g_Vars.esp.glow_local ) || ( entity->IsTeammate( pLocal ) && entity->EntIndex( ) != pLocal->EntIndex( ) ) )
				continue;

			if (entity->EntIndex() == pLocal->EntIndex()) {
				glowObject.m_nGlowStyle = g_Vars.esp.glow_type_local;
				color = g_Vars.esp.glow_local_color;
			}

			if (is_enemy) {
				glowObject.m_nGlowStyle = g_Vars.esp.glow_type_enemy;
				color = g_Vars.esp.glow_enemy_color;
			}

			break;
		}
		case CMolotovProjectile:
		case CSmokeGrenadeProjectile:
		case CDecoyProjectile:
		case CBaseCSGrenadeProjectile:
		{
			if( g_Vars.esp.glow_grenade ) {
				glowObject.m_bRenderWhenOccluded = false;
				glowObject.m_bRenderWhenUnoccluded = false;
				glowObject.m_vGlowColor.w = 0.0f;
				glowObject.m_nGlowStyle = g_Vars.esp.glow_type_grenades;
				color = g_Vars.esp.glow_grenade_color;
			}
		}
		default:
		{
			if( entity->IsWeapon( ) ) {
				if( !g_Vars.esp.glow_weapons ) {
					if( weapon_enabled ) {
						glowObject.m_bRenderWhenOccluded = false;
						glowObject.m_bRenderWhenUnoccluded = false;
						glowObject.m_vGlowColor.w = 0.0f;
					}
					continue;
				}
				weapon_enabled = true;
				glowObject.m_nGlowStyle = g_Vars.esp.glow_type_weapons;
				color = g_Vars.esp.glow_weapons_color;
			}
		}
		}
		glowObject.m_vGlowColor = Vector4D( color.r, color.g, color.b, color.a );
		glowObject.m_bRenderWhenOccluded = true;
		glowObject.m_bRenderWhenUnoccluded = false;
	}


	if( weapon_enabled && !g_Vars.esp.glow_weapons ) {
		weapon_enabled = false;
	}
}

void C_GlowOutline::Shutdown( ) {
	// Remove glow from all entities
	for( auto i = 0; i < Interfaces::m_pGlowObjManager->m_GlowObjectDefinitions.m_Size; i++ ) {
		auto& glowObject = Interfaces::m_pGlowObjManager->m_GlowObjectDefinitions.m_Memory.m_pMemory[ i ];
		auto entity = reinterpret_cast< C_BasePlayer* >( glowObject.m_pEntity );

		if( glowObject.IsUnused( ) )
			continue;

		if( !entity || entity->IsDormant( ) )
			continue;

		glowObject.m_vGlowColor.w = 0.0f;
	}
}