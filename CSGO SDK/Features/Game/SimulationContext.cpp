#include "../../source.hpp"
#include "SimulationContext.hpp"
#include "../../SDK/Displacement.hpp"
#include "../../SDK/Classes/Player.hpp"
#include "../../SDK/Classes/weapon.hpp"

static CTraceFilterWorldOnly world_filter;

inline void VectorMA( const Vector& start, float scale, const Vector& direction, Vector& dest ) {
	dest.x = start.x + scale * direction.x;
	dest.y = start.y + scale * direction.y;
	dest.z = start.z + scale * direction.z;
}

void SimulationContext::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, CGameTrace& pm ) {
	Ray_t ray;
	ray.Init( start, end, pMins, pMaxs );
	Interfaces::m_pEngineTrace->TraceRay( ray, fMask, filter, &pm );
}

void SimulationContext::InitSimulationContext( C_CSPlayer* player ) {
	static auto sv_maxspeed = Interfaces::m_pCvar->FindVar( XorStr( "sv_maxspeed" ) );
	static auto sv_stepsize = Interfaces::m_pCvar->FindVar( XorStr( "sv_stepsize" ) );

	//	 wihspeedClamped *= CS_PLAYER_SPEED_DUCK_MODIFIER;
	this->walking = false;
	this->m_vecOrigin = player->m_vecOrigin( );
	this->m_vecVelocity = player->m_vecVelocity( );
	this->simulationTicks = player->m_nTickBase( );
	this->trace = CGameTrace{ };
	this->gravity = g_Vars.sv_gravity->GetFloat( ) * Interfaces::m_pGlobalVars->interval_per_tick * 0.5f;
	this->sv_jump_impulse = ( g_Vars.sv_jump_impulse )->GetFloat( );
	this->stepsize = sv_stepsize->GetFloat( );
	this->pMins = player->m_Collision( )->m_vecMins;
	this->pMaxs = player->m_Collision( )->m_vecMaxs;
	this->player = player;
	this->filter = &world_filter;
	this->flags = player->m_fFlags( ) & 1 | player->m_fFlags( ) & 0xF8 | 2 * ( ( ( unsigned int )player->m_fFlags( ) >> 1 ) & 1 );

	//static auto get_max_speed_idx = *( int* ) ( Memory::Scan( XorStr( "client.dll" ), XorStr( "8B 80 ?? ?? ?? ?? FF D0 8B 47 08" ) ) + 2 ) / 4;

	//get_max_speed_idx
	float maxSpeed = Memory::VCall< float( __thiscall* )( void* ) >( player, 274 )( player );

	this->flMaxSpeed = std::fminf( maxSpeed, sv_maxspeed->GetFloat( ) );
}

void SimulationContext::ExtrapolatePlayer( float yaw ) {
	static auto g_GameRules = *( uintptr_t** )( Engine::Displacement.Data.m_GameRules );
	if( !g_GameRules || *( bool* )( *( uintptr_t* )g_GameRules + 0x20 ) || ( this->flags & ( 1 << 6 ) ) )
		return;
	
	CUserCmd cmd;
	cmd.viewangles.y = yaw;
	if( *( int* )( uintptr_t( this->player ) + Engine::Displacement.DT_CSPlayer.m_iMoveState ) )
		cmd.forwardmove = 450.0f;

	if( this->flags & FL_DUCKING ) {
		cmd.buttons |= IN_DUCK;
	}

	if( *( bool* )( uintptr_t( this->player ) + Engine::Displacement.DT_CSPlayer.m_bIsWalking ) ) {
		cmd.buttons |= IN_SPEED;
	}

	if( ( this->flags & 5 ) == 5 )
		cmd.buttons |= IN_JUMP;

	RebuildGameMovement( &cmd );
}

void SimulationContext::TryPlayerMove( ) {
	auto ClipVelocity = [ ] ( Vector& in, Vector& normal, Vector& out, float overbounce ) {
		auto  angle = normal[ 2 ];

		// Determine how far along plane to slide based on incoming direction.
		auto backoff = DotProduct( in, normal ) * overbounce;

		for( int i = 0; i < 3; i++ ) {
			auto change = normal[ i ] * backoff;
			out[ i ] = in[ i ] - change;
		}

		// iterate once to make sure we aren't still moving through the plane
		float adjust = DotProduct( out, normal );
		if( adjust < 0.0f ) {
			out -= ( normal * adjust );
		}
	};

#define MAX_CLIP_PLANES 5 

	int			bumpcount;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[ MAX_CLIP_PLANES ];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	CGameTrace	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;

	blocked = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	original_velocity = this->m_vecVelocity;  // Store original velocity
	primal_velocity = this->m_vecVelocity;

	allFraction = 0;
	time_left = Interfaces::m_pGlobalVars->interval_per_tick;   // Total time for this movement operation.

	new_velocity.Init( );

	for( bumpcount = 0; bumpcount < 4; bumpcount++ ) {
		if( this->m_vecVelocity.Length( ) == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.

		VectorMA( this->m_vecOrigin, time_left, this->m_vecVelocity, end );

		TracePlayerBBox( this->m_vecOrigin, end, MASK_PLAYERSOLID, pm );
		if( pm.fraction > 0.0f && pm.fraction < 0.0001f ) {
			pm.fraction = 0.0f;
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if( pm.allsolid ) {
			// entity is trapped in another solid
			this->m_vecVelocity.Set( );
			return;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 ) {
			if( pm.fraction == 1 ) {
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				CGameTrace stuck;
				TracePlayerBBox( pm.endpos, pm.endpos, MASK_PLAYERSOLID, stuck );
				if( stuck.startsolid || stuck.fraction != 1.0f ) {
					//Msg( XorStr( "Player will become stuck!!!\n" ) );
					this->m_vecVelocity.Set( );
					break;
				}
			}

			// actually covered some distance
			this->m_vecOrigin = pm.endpos;
			original_velocity = this->m_vecVelocity;
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if( pm.fraction == 1 ) {
			break;		// moved the entire distance
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if( numplanes >= MAX_CLIP_PLANES ) {
			// this shouldn't really happen
			//  Stop our movement if so.
			this->m_vecVelocity.Set( );
			//Con_DPrintf(XorStr( "Too many planes 4\n" ));

			break;
		}

		// Set up next clipping plane
		planes[ numplanes ] = pm.plane.normal;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if( numplanes == 1 &&
			player->m_MoveType( ) == MOVETYPE_WALK &&
			!( this->flags & FL_ONGROUND ) ) {
			for( i = 0; i < numplanes; i++ ) {
				if( planes[ i ][ 2 ] > 0.7f ) {
					// floor or slope
					ClipVelocity( original_velocity, planes[ i ], new_velocity, 1.0f );
					original_velocity = new_velocity;
				}
				else {
					ClipVelocity(
						original_velocity,
						planes[ i ],
						new_velocity,
						1.0f );
				}
			}

			this->m_vecVelocity = new_velocity;
			original_velocity = new_velocity;
		}
		else {
			for( i = 0; i < numplanes; i++ ) {
				ClipVelocity( original_velocity, planes[ i ], this->m_vecVelocity, 1 );

				for( j = 0; j < numplanes; j++ ) {
					if( j != i ) {
						// Are we now moving against this plane?
						if( this->m_vecVelocity.Dot( planes[ j ] ) < 0.f )
							break;	// not ok
					}
				}

				if( j == numplanes )  // DidnXorStr( 't have to clip, so we' )re ok
					break;
			}

			// Did we go all the way through plane set
			if( i == numplanes ) {
				if( numplanes != 2 ) {
					this->m_vecVelocity.Set( );
					break;
				}

				dir = planes[ 0 ].Cross( planes[ 1 ] );
				dir.Normalize( );
				d = dir.Dot( this->m_vecVelocity );
				this->m_vecVelocity = dir * d;
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = this->m_vecVelocity.Dot( primal_velocity );
			if( d <= 0.f ) {
				//Con_DPrintf(XorStr( "Back\n" ));
				this->m_vecVelocity.Set( );
				break;
			}
		}
	}

	if( allFraction == 0.f ) {
		this->m_vecVelocity.Set( );
	}
}

void SimulationContext::RebuildGameMovement( CUserCmd* ucmd ) {

#define CS_PLAYER_SPEED_WALK_MODIFIER 0.52f
#define CS_PLAYER_SPEED_DUCK_MODIFIER 0.34f

	static auto sv_enablebunnyhopping = Interfaces::m_pCvar->FindVar( XorStr( "sv_enablebunnyhopping" ) );
	static auto sv_walkable_normal = Interfaces::m_pCvar->FindVar( XorStr( "sv_enablebunnyhopping" ) );

	this->buttons = ucmd->buttons;
	if( !this->walking ) {
		this->walking = 1;
		if( ucmd->buttons & IN_SPEED ) {
			auto v7 = this->flMaxSpeed * 0.52f;
			if( v7 + 25.0f > this->m_vecVelocity.Length( ) )
				this->flMaxSpeed = v7;
		}
	}

	// StartGravity
	this->m_vecVelocity.z -= this->gravity;

	// CheckJumpButton
	if( ucmd->buttons & IN_JUMP && this->flags & FL_ONGROUND ) {
		this->flags &= ~FL_ONGROUND;

		if( !sv_enablebunnyhopping->GetBool( ) ) {
			auto v9 = this->flMaxSpeed * 1.1f;
			if( v9 > 0.0f ) {
				auto v10 = this->m_vecVelocity.Length( );
				if( v10 > v9 ) {
					auto v11 = v9 / v10;
					this->m_vecVelocity *= v11;
				}
			}
		}

		auto v12 = this->sv_jump_impulse;
		if( !( this->flags & FL_DUCKING ) )
			v12 += this->m_vecVelocity.z;
		this->m_vecVelocity.z = v12;
		this->m_vecVelocity.z = v12 - this->gravity;
	}

	// CheckParameters
	float  spd = ( ucmd->forwardmove * ucmd->forwardmove ) +
		( ucmd->sidemove * ucmd->sidemove ) +
		( ucmd->upmove * ucmd->upmove );

	auto move = Vector2D( ucmd->forwardmove, ucmd->sidemove );
	if( ( spd != 0.0 ) && ( spd > flMaxSpeed * flMaxSpeed ) ) {
		float fRatio = flMaxSpeed / sqrt( spd );
		move *= fRatio;
	}

	if( this->flags & FL_ONGROUND && ( this->flags & FL_DUCKING ) ) {
		float frac = 0.33333333f;
		move *= frac;
	}

	if( this->flags & FL_ONGROUND ) {
		this->flMaxSpeed *= player->m_flVelocityModifier( );

		this->m_vecVelocity.z = 0.0f;
		// Friction
		{
			static auto sv_stopspeed = Interfaces::m_pCvar->FindVar( XorStr( "sv_stopspeed" ) );
			static auto sv_friction = Interfaces::m_pCvar->FindVar( XorStr( "sv_friction" ) );

			auto speed = this->m_vecVelocity.Length2D( );
			if( speed < 0.1f ) {
				this->m_vecVelocity.x = 0.0f;
				this->m_vecVelocity.y = 0.0f;
				this->m_vecVelocity.z = 0.0f;
			}
			else {
				auto control = std::fmaxf( sv_stopspeed->GetFloat( ), speed );
				auto friction = sv_friction->GetFloat( );
				auto drop = ( control * friction ) * Interfaces::m_pGlobalVars->interval_per_tick;

				auto newspeed = std::fmaxf( speed - drop, 0.0f );
				if( newspeed != speed ) {
					newspeed = newspeed / speed;
					this->m_vecVelocity *= newspeed;
				}
			}
		}

		// WalkMove
		{

			auto bOnGround = this->flags & FL_ONGROUND;

			Vector right;
			auto forward = ucmd->viewangles.ToVectors( &right );

			if( forward.z != 0.0f ) {
				forward.z = 0.0f;
				forward.Normalize( );
			}

			if( right.z != 0.0f ) {
				right.z = 0.0f;
				right.Normalize( );
			}

			auto wishdir = forward * move.x + right * move.y;
			auto wishspeed = wishdir.Normalize( );

			if( wishspeed > this->flMaxSpeed )
				wishspeed = this->flMaxSpeed;

			// accelerate
			this->m_vecVelocity.z = 0.0f;
			{
				static auto sv_accelerate_use_weapon_speed = Interfaces::m_pCvar->FindVar( XorStr( "sv_accelerate_use_weapon_speed" ) );

				// See if we are changing direction a bit
				float currentspeed = this->m_vecVelocity.Dot( wishdir );

				// Reduce wishspeed by the amount of veer.
				float addspeed = wishspeed - currentspeed;

				if( addspeed > 0.0f ) {
					// soufiw rebuild

					bool ducked = true;

					if( !( this->buttons & IN_DUCK ) ) {
						if( /*!player->m_bDucking( ) && */!( player->m_fFlags( ) & FL_DUCKING ) )
							ducked = false;
					}
					bool in_speed = true;
					if( !( buttons & IN_SPEED ) || ducked )
						in_speed = false;

					float clampedSpeed = fmaxf( currentspeed, 0.0f );
					float wishspeedClamped = fmaxf( wishspeed, 250.0f );

					C_CSPlayer* Player = player;

					bool InZoom = false;

					float FinalSpeed = 0.0f;

					bool v60 = false;
					auto weapon = ( C_WeaponCSBaseGun* )player->m_hActiveWeapon( ).Get( );
					if( sv_accelerate_use_weapon_speed->GetBool( ) && weapon ) {
						auto weaponData = weapon->GetCSWeaponData( );
						float maxSpeed = ( weapon->m_weaponMode( ) == 0 ) ? weaponData->m_flMaxSpeed : weaponData->m_flMaxSpeed2;
						int unknown2 = *( int* )( ( uintptr_t )weaponData.Xor( ) + 456 ); // some zoom shit btw

						if( weapon->m_zoomLevel( ) <= 0 || unknown2 <= 1 ) {
							InZoom = false;
						}
						else {
							InZoom = maxSpeed * CS_PLAYER_SPEED_WALK_MODIFIER < 110.0f;
						}

						float maxSpeedNorm = maxSpeed * 0.004f; // 0.004 = 1.0 / 250.0
						float maxSpeedClamped = fminf( maxSpeedNorm, 1.0f );
						if( !ducked && !in_speed || InZoom )
							wishspeedClamped *= maxSpeedClamped;

						FinalSpeed = maxSpeedClamped * wishspeedClamped;
					}
					else {
						FinalSpeed = wishspeedClamped;
					}

					if( ducked ) {
						if( !InZoom )
							wishspeedClamped *= CS_PLAYER_SPEED_DUCK_MODIFIER;
						FinalSpeed *= CS_PLAYER_SPEED_DUCK_MODIFIER;
					}

					if( in_speed ) {
						if( !player->m_bHasHeavyArmor( ) /*&& !player->m_hCarriedHostage( ).Get( ) */ && !InZoom )
							wishspeedClamped *= CS_PLAYER_SPEED_WALK_MODIFIER;
						FinalSpeed *= CS_PLAYER_SPEED_WALK_MODIFIER;
					}

					auto accel = g_Vars.sv_accelerate->GetFloat( );
					float v27 = ( ( Interfaces::m_pGlobalVars->interval_per_tick * accel ) * wishspeedClamped );
					if( v60 && clampedSpeed > ( FinalSpeed - v27 ) ) {
						float v28 = 1.0f - ( fmaxf( clampedSpeed - ( FinalSpeed - v27 ), 0.0f ) / fmaxf( FinalSpeed - ( FinalSpeed - v27 ), 0.0f ) );
						if( v28 >= 0.0f ) {
							accel = fminf( v28, 1.0f ) * accel;
						}
						else {
							accel = 0.0f * accel;
						}
					}

					float v30 = 0.0f;
					if( in_speed && clampedSpeed > ( FinalSpeed - 5.0f ) ) {
						float v29 = fmaxf( clampedSpeed - ( FinalSpeed - 5.0f ), 0.0f ) / fmaxf( FinalSpeed - ( FinalSpeed - 5.0f ), 0.0f );
						if( ( 1.0f - v29 ) >= 0.0f )
							v30 = fminf( 1.0f - v29, 1.0f ) * accel;
						else
							v30 = 0.0f * accel;
					}
					else {
						v30 = accel;
					}

					float v31 = fminf( ( ( Interfaces::m_pGlobalVars->interval_per_tick * v30 ) * wishspeedClamped ), addspeed );
					addspeed = v31;

					this->m_vecVelocity += wishdir * v31;
				}
			}
			this->m_vecVelocity.z = 0.0f;

			auto speed = m_vecVelocity.Length( );
			if( speed > this->flMaxSpeed ) {
				m_vecVelocity = m_vecVelocity.Normalized( ) * this->flMaxSpeed;
			}

			// m_vecVelocity += m_vecBaseVelocity;

			// first try just moving to the destination	
			Vector dest;
			dest[ 0 ] = this->m_vecOrigin[ 0 ] + this->m_vecVelocity[ 0 ] * Interfaces::m_pGlobalVars->interval_per_tick;
			dest[ 1 ] = this->m_vecOrigin[ 1 ] + this->m_vecVelocity[ 1 ] * Interfaces::m_pGlobalVars->interval_per_tick;
			dest[ 2 ] = this->m_vecOrigin[ 2 ];

			// first try moving directly to the next spot
			TracePlayerBBox( this->m_vecOrigin, dest, MASK_PLAYERSOLID, this->trace );

			if( this->trace.fraction == 1.0f ) {
				// StayOnGround
				this->m_vecOrigin = this->trace.endpos;

				CGameTrace tr;
				Vector start( this->m_vecOrigin );
				Vector end( this->m_vecOrigin );
				start.z += 2.0f;
				end.z -= this->stepsize;

				// See how far up we can go without getting stuck
				TracePlayerBBox( this->m_vecOrigin, start, MASK_PLAYERSOLID, tr );
				start = tr.endpos;

				// using trace.startsolid is unreliable here, it doesn't get set when
				// tracing bounding box vs. terrain

				// Now trace down from a known safe position
				TracePlayerBBox( start, end, MASK_PLAYERSOLID, tr );
				if( tr.fraction > 0.0f && tr.fraction < 1.0f && !tr.startsolid && tr.plane.normal.z >= sv_walkable_normal->GetFloat( ) ) {
					float flDelta = std::fabsf( this->m_vecOrigin.z - tr.endpos.z );
					if( flDelta > 0.015625f ) {
						this->m_vecOrigin = tr.endpos;
					}
				}
			}
			else if( bOnGround ) {
				// Try sliding forward both on ground and up 16 pixels
				//  take the move that goes farthest
				auto vecPos = this->m_vecOrigin;
				auto vecVel = this->m_vecVelocity;

				// Slide move down.
				TryPlayerMove( );

				auto vecDownPos = this->m_vecOrigin;
				auto vecDownVel = this->m_vecVelocity;

				// Reset original values.
				this->m_vecOrigin = vecPos;
				this->m_vecVelocity = vecVel;

				// Move up a stair height.
				Vector vecEndPos;
				vecEndPos = this->m_vecOrigin;
				vecEndPos.z += this->stepsize + 0.03125f;

				CGameTrace trace;
				TracePlayerBBox( this->m_vecOrigin, vecEndPos, MASK_PLAYERSOLID, trace );
				if( !trace.startsolid && !trace.allsolid ) {
					this->m_vecOrigin = trace.endpos;
				}

				// Slide move up.
				TryPlayerMove( );

				vecEndPos = this->m_vecOrigin;
				vecEndPos.z -= this->stepsize + 0.03125f;

				TracePlayerBBox( this->m_vecOrigin, vecEndPos, MASK_PLAYERSOLID, trace );
				if( !trace.startsolid && !trace.allsolid ) {
					this->m_vecOrigin = trace.endpos;
				}

				// If we are not on the ground any more then use the original movement attempt.
				if( trace.plane.normal[ 2 ] < sv_walkable_normal->GetFloat( ) ) {
					this->m_vecOrigin = vecDownPos;
					this->m_vecVelocity = vecDownVel;
				}
				else {
					// If the trace ended up in empty space, copy the end over to the origin.
					if( !trace.startsolid && !trace.allsolid ) {
						this->m_vecOrigin = trace.endpos;
					}

					Vector vecUpPos;
					vecUpPos = this->m_vecOrigin;

					// decide which one went farther
					float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
					float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
					if( flDownDist > flUpDist ) {
						this->m_vecOrigin = vecDownPos;
						this->m_vecVelocity = vecDownVel;
					}
					else {
						// copy z value from slide move
						this->m_vecVelocity.z = vecDownVel.z;
					}
				}

				CGameTrace tr;
				Vector start( this->m_vecOrigin );
				Vector end( this->m_vecOrigin );
				start.z += 2.0f;
				end.z -= this->stepsize;

				// See how far up we can go without getting stuck
				TracePlayerBBox( this->m_vecOrigin, start, MASK_PLAYERSOLID, tr );
				start = tr.endpos;

				// using trace.startsolid is unreliable here, it doesn't get set when
				// tracing bounding box vs. terrain

				// Now trace down from a known safe position
				TracePlayerBBox( start, end, MASK_PLAYERSOLID, tr );
				if( tr.fraction > 0.0f && tr.fraction < 1.0f && !tr.startsolid && tr.plane.normal.z >= sv_walkable_normal->GetFloat( ) ) {
					float flDelta = std::fabsf( this->m_vecOrigin.z - tr.endpos.z );
					if( flDelta > 0.015625f ) {
						this->m_vecOrigin = tr.endpos;
					}
				}
			}
		}
	}
	else {
		// AirMove
		Vector right;
		auto forward = ucmd->viewangles.ToVectors( &right );

		right.z = 0.0f;
		forward.z = 0.0f;
		forward.Normalize( );
		right.Normalize( );

		auto wishdir = forward * move.x + right * move.y;
		auto wishspeed = wishdir.Normalize( );

		static auto sv_air_max_wishspeed = Interfaces::m_pCvar->FindVar( XorStr( "sv_air_max_wishspeed" ) );
		if( wishspeed > sv_air_max_wishspeed->GetFloat( ) )
			wishspeed = sv_air_max_wishspeed->GetFloat( );

		auto currentspeed = this->m_vecVelocity.Dot( wishdir );
		auto addspeed = wishspeed - currentspeed;
		if( addspeed > 0.0 ) {
			auto v169 = ( g_Vars.sv_airaccelerate )->GetFloat( ) * wishspeed;
			auto v170 = std::fminf( addspeed, v169 * Interfaces::m_pGlobalVars->interval_per_tick );
			this->m_vecVelocity += wishdir * v170;
		}

		TryPlayerMove( );
	}

	CGameTrace trace;
	TracePlayerBBox( this->m_vecOrigin, this->m_vecOrigin - Vector( 0.0f, 0.0f, 2.0f ), MASK_PLAYERSOLID, trace );
	if( trace.DidHit( ) && trace.plane.normal.z >= sv_walkable_normal->GetFloat( ) )
		this->flags |= 1u;
	else
		this->flags &= 0xFEu;

	if( this->flags & 1 )
		this->m_vecVelocity.z = 0.0f;
	else
		this->m_vecVelocity.z -= this->gravity;

	++this->simulationTicks;
}
