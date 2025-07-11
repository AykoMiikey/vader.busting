#pragma once
#include "../../SDK/sdk.hpp"

#define DOUBLE_TAP_CHARGE 14
#define BREAK_LC_CHARGE 14
#define BREAK_LC_CHARGE_HIDE_SHOTS 14
#define HIDE_SHOTS_CHARGE 8

namespace Engine
{
	class C_LagRecord;
}

namespace Interfaces
{
	class __declspec(novtable) Ragebot : public NonCopyable {
	public:
		enum SelectTarget_e : int {
			SELECT_HIGHEST_DAMAGE = 0,
			SELECT_FOV,
			SELECT_LOWEST_HP,
			SELECT_LOWEST_DISTANCE,
			SELECT_LOWEST_PING,
			SELECT_OPTIMIZED,
		};

		static Encrypted_t<Ragebot> Get();
		virtual bool Run(Encrypted_t<CUserCmd> cmd, C_CSPlayer* local, bool* sendPacket) = 0;
		virtual bool GetBoxOption(int hitbox, float& ps, bool override_hitscan, bool force_body) = 0;
		virtual void Multipoint(C_CSPlayer* player, Engine::C_LagRecord* record, int side, std::vector<std::pair<Vector, bool>>& points, mstudiobbox_t* hitbox, mstudiohitboxset_t* hitboxSet, float& ps, int hitboxIndex) = 0;
		virtual bool ShouldBodyAim(C_CSPlayer* player, Engine::C_LagRecord* record) = 0;
		virtual bool ShouldHeadAim(C_CSPlayer* player, Engine::C_LagRecord* record) = 0;
		virtual bool ShouldForceBodyAim(C_CSPlayer* player, Engine::C_LagRecord* record) = 0;
		virtual bool SetupRageOptions() = 0;
	protected:
		Ragebot() { };
		virtual ~Ragebot() { };
	};
}
