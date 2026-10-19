class BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService
{
	protected static ref BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService s_Instance;

	protected static const string CONFIG_PATH = "$profile:BetterVehicleWindChill.json";
	protected static const string LOG_PREFIX = "[BetterVehicleWindChill] ";

	protected ref BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig m_Config;
	protected bool m_HasLoadedConfig;

	static BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService GetInstance()
	{
		if (!s_Instance)
			s_Instance = new BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService();

		return s_Instance;
	}

	//! Loads the config at server start, so the file exists before the first player joins
	void Preload()
	{
		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig config = GetConfig();

		string boatSummary = DescribeVehicle(config.BoatEnabled, config.BoatSpeedMinKmh, config.BoatSpeedMaxKmh, config.BoatMaxPenalty);
		string motorbikeSummary = DescribeVehicle(config.MotorbikeEnabled, config.MotorbikeSpeedMinKmh, config.MotorbikeSpeedMaxKmh, config.MotorbikeMaxPenalty);
		Log("Started. Boat " + boatSummary + ". Motorbike " + motorbikeSummary + ". Respect heat buffer " + config.RespectHeatBuffer.ToString() + ", debug log " + config.DebugLog.ToString() + ".");
	}

	float GetBoatPenalty(float speedKmh, float heatComfortOverride)
	{
		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig config = GetConfig();
		if (!config.BoatEnabled)
			return 0.0;

		return SpeedPenalty(speedKmh, config.BoatSpeedMinKmh, config.BoatSpeedMaxKmh, config.BoatMaxPenalty - heatComfortOverride);
	}

	float GetMotorbikePenalty(float speedKmh, float heatComfortOverride)
	{
		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig config = GetConfig();
		if (!config.MotorbikeEnabled)
			return 0.0;

		return SpeedPenalty(speedKmh, config.MotorbikeSpeedMinKmh, config.MotorbikeSpeedMaxKmh, config.MotorbikeMaxPenalty - heatComfortOverride);
	}

	//! Remaps the speed from speedMinKmh..speedMaxKmh to 0..1 on the vanilla ease-in cubic curve.
	//! Vanilla 1.30 divides the raw speed by (max - min) instead, which reaches the full penalty at 35 km/h on a boat.
	static float SpeedPenalty(float speedKmh, float speedMinKmh, float speedMaxKmh, float maxPenalty)
	{
		float speed = Math.Round(Math.AbsFloat(speedKmh));
		if (speed <= speedMinKmh || maxPenalty <= 0.0)
			return 0.0;

		float normalizedSpeed = Math.Clamp((speed - speedMinKmh) / (speedMaxKmh - speedMinKmh), 0.0, 1.0);
		return Easing.EaseInCubic(normalizedSpeed) * maxPenalty;
	}

	BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig GetConfig()
	{
		if (m_HasLoadedConfig && m_Config)
			return m_Config;

		m_HasLoadedConfig = true;
		m_Config = new BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig();

		if (!FileExist(CONFIG_PATH))
		{
			SaveConfig();
			return m_Config;
		}

		string errorMessage;
		if (!JsonFileLoader<BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig>.LoadFile(CONFIG_PATH, m_Config, errorMessage))
		{
			m_Config = new BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig();
			Log("Failed to load config from " + CONFIG_PATH + ", using defaults: " + errorMessage);
		}

		m_Config.Validate();
		return m_Config;
	}

	protected void SaveConfig()
	{
		string errorMessage;
		if (!JsonFileLoader<BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig>.SaveFile(CONFIG_PATH, m_Config, errorMessage))
			Log("Failed to write default config to " + CONFIG_PATH + ": " + errorMessage);
	}

	protected string DescribeVehicle(bool enabled, float speedMinKmh, float speedMaxKmh, float maxPenalty)
	{
		if (!enabled)
			return "off";

		return speedMinKmh.ToString() + " to " + speedMaxKmh.ToString() + " km/h, max penalty " + maxPenalty.ToString();
	}

	void Log(string message)
	{
		Print(LOG_PREFIX + message);
	}
}
