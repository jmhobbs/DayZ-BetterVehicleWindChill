//! Replaces the 1.30 boat and motorbike speed penalty on heat comfort with configurable values. Server only, like the vanilla Environment.
modded class Environment
{
	//! Heat comfort penalty for this tick. Zero unless the player is on a boat or motorbike above the minimum speed.
	protected float m_BetterVehicleWindChill_Penalty;
	//! Speedometer reading for the debug line, which is written once the heat comfort step is done
	protected float m_BetterVehicleWindChill_Speed;
	protected bool m_BetterVehicleWindChill_IsLogPending;

	//! Runs every tick, before ProcessHeatComfort or SetHeatcomfortDirectly
	override protected bool DetermineHeatcomfortBehavior()
	{
		bool isAltered = super.DetermineHeatcomfortBehavior();
		m_BetterVehicleWindChill_Penalty = 0.0;
		m_BetterVehicleWindChill_IsLogPending = false;

		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService service = BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService.GetInstance();
		int category;
		float speed;

		Boat boat = Boat.Cast(m_Player.GetParent());
		Motorbike motorbike = Motorbike.Cast(m_Player.GetParent());
		if (boat)
		{
			category = EEnvironmentHeatcomfortBehaviorCategory.BOAT;
			speed = boat.GetSpeedometerAbsolute();

			float boatOverride = 0.0;
			BoatScript boatScript = BoatScript.Cast(boat);
			if (boatScript)
				boatOverride = boatScript.GetHeatComfortOverride();

			m_BetterVehicleWindChill_Penalty = service.GetBoatPenalty(speed, boatOverride);
		}
		else if (motorbike)
		{
			category = EEnvironmentHeatcomfortBehaviorCategory.MOTORBIKE;
			speed = motorbike.GetSpeedometerAbsolute();

			float motorbikeOverride = 0.0;
			MotorbikeScript motorbikeScript = MotorbikeScript.Cast(motorbike);
			if (motorbikeScript)
				motorbikeOverride = motorbikeScript.GetHeatComfortOverride();

			m_BetterVehicleWindChill_Penalty = service.GetMotorbikePenalty(speed, motorbikeOverride);
		}
		else
		{
			return isAltered;
		}

		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig config = service.GetConfig();
		bool isDirect = m_BetterVehicleWindChill_Penalty > 0.0 && !config.RespectHeatBuffer;
		m_BetterVehicleWindChill_Speed = speed;
		m_BetterVehicleWindChill_IsLogPending = config.DebugLog;

		if (!isDirect)
		{
			//! The default path runs, and EnvTempToCoef subtracts the penalty before the heat buffer step
			m_HeatComfortBehaviorCategory = EEnvironmentHeatcomfortBehaviorCategory.DEFAULT;
			return false;
		}

		m_HeatComfortBehaviorCategory = category;
		return true;
	}

	override protected void SetHeatcomfortDirectly()
	{
		if (m_HeatComfortBehaviorCategory != EEnvironmentHeatcomfortBehaviorCategory.BOAT && m_HeatComfortBehaviorCategory != EEnvironmentHeatcomfortBehaviorCategory.MOTORBIKE)
		{
			super.SetHeatcomfortDirectly();
			return;
		}

		//! Same steps as the vanilla boat and motorbike cases, with the configured penalty
		ProcessHeatComfort();
		float targetHeatComfort = m_TargetHeatComfort - m_BetterVehicleWindChill_Penalty;

		float dynamicHeatComfort = m_AverageHeatComfortBuffer.Add(targetHeatComfort);
		m_HeatComfort = dynamicHeatComfort;

		SetTargetHeatComfort(targetHeatComfort);
		m_Player.GetStatHeatComfort().Set(dynamicHeatComfort);

		BetterVehicleWindChill_LogTick();
	}

	override protected void ProcessHeatComfort()
	{
		super.ProcessHeatComfort();

		//! The direct path calls this before subtracting the penalty, and logs from SetHeatcomfortDirectly instead
		if (m_HeatComfortBehaviorCategory == EEnvironmentHeatcomfortBehaviorCategory.DEFAULT)
			BetterVehicleWindChill_LogTick();
	}

	//! Vanilla calls this once per tick, as the last term of the target heat comfort
	override protected float EnvTempToCoef(float pTemp)
	{
		float coef = super.EnvTempToCoef(pTemp);

		//! In the direct path SetHeatcomfortDirectly subtracts the penalty instead
		if (m_HeatComfortBehaviorCategory == EEnvironmentHeatcomfortBehaviorCategory.DEFAULT)
			coef -= m_BetterVehicleWindChill_Penalty;

		return coef;
	}

	//! Writes the debug line at most once per tick, after the heat comfort step, so every value is from this tick
	protected void BetterVehicleWindChill_LogTick()
	{
		if (!m_BetterVehicleWindChill_IsLogPending)
			return;

		m_BetterVehicleWindChill_IsLogPending = false;

		string playerId = "unknown";
		if (m_Player.GetIdentity())
			playerId = m_Player.GetIdentity().GetPlainId();

		string vehicleType = "unknown";
		Object vehicle = Object.Cast(m_Player.GetParent());
		if (vehicle)
			vehicleType = vehicle.GetType();

		bool isDirect = m_HeatComfortBehaviorCategory != EEnvironmentHeatcomfortBehaviorCategory.DEFAULT;
		string mode = "before heat buffer";
		if (isDirect)
			mode = "direct";

		//! The snapshot holds the target before the heat buffer clamp. Only the default path has the penalty in it.
		float baseTarget = m_EnvironmentSnapshot.m_TargetHeatComfort;
		if (!isDirect)
			baseTarget += m_BetterVehicleWindChill_Penalty;

		//! Calls made inside the string.Format arguments can overwrite each other's results. The speed printed as the rounded heat comfort.
		float speedKmh = Math.Round(Math.AbsFloat(m_BetterVehicleWindChill_Speed));
		float penalty = BetterVehicleWindChill_Round2(m_BetterVehicleWindChill_Penalty);
		float target = BetterVehicleWindChill_Round2(m_TargetHeatComfort);
		float baseTargetRounded = BetterVehicleWindChill_Round2(baseTarget);
		float heatComfort = BetterVehicleWindChill_Round2(m_HeatComfort);
		float heatBufferPercent = Math.Round(GetApplicableHeatbuffer() * 1000) * 0.1;

		string message = string.Format("Player %1 on %2 at %3 km/h, penalty %4 (%5). Target heat comfort %6, %7 before the penalty and heat buffer. Heat comfort %8, heat buffer %9", playerId, vehicleType, speedKmh, penalty, mode, target, baseTargetRounded, heatComfort, heatBufferPercent);
		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService.GetInstance().Log(message + "%");
	}

	protected float BetterVehicleWindChill_Round2(float value)
	{
		return Math.Round(value * 100) * 0.01;
	}
}
