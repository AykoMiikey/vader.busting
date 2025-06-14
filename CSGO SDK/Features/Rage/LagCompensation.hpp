#pragma once
#include "../../SDK/sdk.hpp"

#include "../../SDK/Classes/weapon.hpp"
#include "../../SDK/Classes/Player.hpp"

#include <map>
#include <deque>
#include "AnimationSystem.hpp"

#define RESOLVER_SIDE_FAKE 0
#define RESOLVER_SIDE_LEFT 1
#define RESOLVER_SIDE_RIGHT -1

#define ANIMATION_RESOLVER_FAKE 1
#define ANIMATION_RESOLVER_LEFT 2
#define ANIMATION_RESOLVER_RIGHT 4

class optimized_adjust_data
{
public:
	int i;
	C_CSPlayer* player;

	float simulation_time;
	float duck_amount;
	float speed;
	int resolvermode;

	float angles;
	Vector origin;

	optimized_adjust_data()
	{
		reset();
	}

	void reset()
	{
		i = 0;
		player = nullptr;

		simulation_time = 0.0f;
		duck_amount = 0.0f;
		speed = 0.0f;

		angles = 0.0f;
		origin.Zero;
	}
};

namespace Engine
{
	// base lag record, generally used for backup and restore
	class C_BaseLagRecord {
	public:
		C_CSPlayer* player = nullptr;
		Vector m_vecMins, m_vecMaxs;
		Vector m_vecOrigin;
		QAngle m_angAngles;
		float m_flAbsRotation;

		float m_flSimulationTime;

		alignas(16) matrix3x4_t m_BoneMatrix[128];

		void Setup(C_CSPlayer* player);
		void Apply(C_CSPlayer* player);
	};

	// full lag record, will be stored in lag compensation history
	class C_LagRecord : public C_BaseLagRecord {
	public:
		float m_flServerLatency;
		float m_flRealTime;
		float m_flLastShotTime;
		float m_flEyeYaw;
		float m_flEyePitch;
		float m_flAnimationVelocity;

		bool m_bIsShoting;
		bool m_bIsValid;
		bool m_bBonesCalculated;
		bool m_bExtrapolated;
		bool m_bResolved;
		bool m_bIsWalking;
		bool m_bIsRunning;
		bool m_bIsSideways;

		float m_flAbsRotation;

		int m_iFlags;
		int m_iLaggedTicks = 0;
		int m_iResolverMode;

		float m_flInterpolateTime = 0.f;

		// for sorting
		Vector m_vecVelocity;
		float m_flDuckAmount;

		std::array<C_AnimationLayer, 15> m_LayerRecords;

		bool m_bSkipDueToResolver = false; // skip record in hitscan
		bool m_bTeleportDistance = false; // teleport distance was broken

		float GetAbsYaw();
		matrix3x4_t* GetBoneMatrix();
		void Setup(C_CSPlayer* player);
		void Apply( C_CSPlayer* player);
	};

	class C_EntityLagData {
	public:
		C_EntityLagData();

		static void UpdateRecordData(Encrypted_t< C_EntityLagData > pThis, C_CSPlayer* player, const player_info_t& info, int updateType);

		void Clear();

		std::deque<Engine::C_LagRecord> m_History = {};
		int m_iUserID = -1;

		float m_flLastUpdateTime = 0.0f;
		float m_flRate = 0.0f;

		float m_old_sim = 0.0f;
		float m_cur_sim = 0.0f;
		float m_sim_cycle = 0.0f;
		float m_sim_rate = 0.0f;

		int m_iMissedShots = 0;
		int m_iMissedBruteShots = 0;
		int m_iMissedShotsLBYTEST = 0;
		int m_iMissedShotsDistort = 0;
		int m_iMissedShotsFreestand = 0;
		int m_iMissedShotsLBY = 0;
		int m_iMissedShotsInAir = 0;
		int m_iMissedShotsNOLBY = 0;
		int m_iMissedLBYLog = 0;
		int m_stand_index2 = 0;
		int m_lby_index = 0;
		int m_body_index = 0;
		int m_last_move = 0;
		int m_unknown_move = 0;
		int m_delta_index = 0;

		float m_old_body = 0.f;
		float m_body = 0.f;

		bool m_bInBruteOrder = false;
		bool m_bHitLastMove = false;
		std::deque< float > m_flLastMoveYaw;
		C_AnimationRecord m_walk_record;

		bool m_bGotAbsYaw = false;
		bool m_bGotAbsYawShot = false;
		bool m_bNotResolveIfShooting = false;
		bool m_bRateCheck = false;
		bool m_bRoundStart = false;
		float m_flAbsYawHandled = 0.f;
		float m_flLastMovingLowerBodyYawTarget = 0.f;
		float m_flLastMovingLowerBodyYawTargetTime = 0.f;
		float nextBodyUpdate = 0.f;
		float m_flLastLowerBodyYawTargetUpdateTime = 0.f;
		float m_flOldLowerBodyYawTarget = 0.f;
		float m_flLowerBodyYawTarget = 0.f;
		float m_flSavedLbyDelta = 0.f;
		bool fakewalking = false;
		bool m_bUnsafeVelocityTransition = false;

		// ragebot scan data
		float m_flLastSpread, m_flLastInaccuracy;
		float m_flLastScanTime;
		Vector m_vecLastScanPos;

		// autowall resolver stuff
		float m_flEdges[4];
		float m_flDirection;

		// player prediction, need many improvments
		static bool DetectAutoDirerction( Encrypted_t< C_EntityLagData > pThis, C_CSPlayer* player );
	};

	class __declspec(novtable) LagCompensation : public NonCopyable {
	public:
		static LagCompensation* Get();

		virtual void Update() = 0;
		virtual bool is_breaking_lagcomp(C_CSPlayer* player, const float& simtime) = 0;
		virtual bool IsRecordOutOfBounds(const Engine::C_LagRecord& record, float target_time = 0.2f, int tickbase_shift = -1, bool tick_count_check = true) const = 0;;
		virtual float GetLerp() const = 0;

		virtual Encrypted_t<C_EntityLagData> GetLagData(int entindex) = 0;

		virtual void ClearLagData() = 0;

		float m_flOutLatency;
		float m_flServerLatency;
	protected:
		LagCompensation() { };
		virtual ~LagCompensation() { };
	};
}
