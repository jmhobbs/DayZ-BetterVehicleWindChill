class BetterVehicleWindChill_Scripts_4_World_Classes_WindChillConfig
{
	protected static const string LOG_PREFIX = "[BetterVehicleWindChill] ";

	//! Apply the penalty before the heat buffer step, like any other cold, so the heat buffer protects the rider and drains faster.
	//! When false, the penalty is applied after it, as in vanilla 1.30, and the heat buffer is ignored.
	bool RespectHeatBuffer = true;

	//! When false, boats have no speed penalty, as in 1.29
	bool BoatEnabled = true;
	//! The penalty starts above this speedometer reading
	float BoatSpeedMinKmh = 15;
	//! Full penalty at and above this speedometer reading
	float BoatSpeedMaxKmh = 50;
	//! Subtracted from target heat comfort at BoatSpeedMaxKmh. Heat comfort runs from -1 to 1. Vanilla uses 0.8.
	float BoatMaxPenalty = 0.3;

	//! When false, motorbikes have no speed penalty, as in 1.29
	bool MotorbikeEnabled = true;
	float MotorbikeSpeedMinKmh = 20;
	float MotorbikeSpeedMaxKmh = 100;
	//! Windshield parts lower this, as in vanilla (the front shield by 0.15). Vanilla uses 0.8.
	float MotorbikeMaxPenalty = 0.5;

	//! Logs speed, penalty, heat comfort and heat buffer every environment tick (3 s) for each player on a boat or motorbike
	bool DebugLog = false;

	//! Out-of-range values fall back to the default. The file on disk is left as is.
	void Validate()
	{
		BoatSpeedMinKmh = CheckedFloat("BoatSpeedMinKmh", BoatSpeedMinKmh, 0, 200, 15);
		BoatSpeedMaxKmh = CheckedFloat("BoatSpeedMaxKmh", BoatSpeedMaxKmh, 1, 300, 50);
		BoatMaxPenalty = CheckedFloat("BoatMaxPenalty", BoatMaxPenalty, 0, 1, 0.3);
		if (BoatSpeedMaxKmh <= BoatSpeedMinKmh)
		{
			Print(LOG_PREFIX + "BoatSpeedMaxKmh must be above BoatSpeedMinKmh, using 15 and 50");
			BoatSpeedMinKmh = 15;
			BoatSpeedMaxKmh = 50;
		}

		MotorbikeSpeedMinKmh = CheckedFloat("MotorbikeSpeedMinKmh", MotorbikeSpeedMinKmh, 0, 200, 20);
		MotorbikeSpeedMaxKmh = CheckedFloat("MotorbikeSpeedMaxKmh", MotorbikeSpeedMaxKmh, 1, 300, 100);
		MotorbikeMaxPenalty = CheckedFloat("MotorbikeMaxPenalty", MotorbikeMaxPenalty, 0, 1, 0.5);
		if (MotorbikeSpeedMaxKmh <= MotorbikeSpeedMinKmh)
		{
			Print(LOG_PREFIX + "MotorbikeSpeedMaxKmh must be above MotorbikeSpeedMinKmh, using 20 and 100");
			MotorbikeSpeedMinKmh = 20;
			MotorbikeSpeedMaxKmh = 100;
		}
	}

	protected float CheckedFloat(string fieldName, float value, float minimum, float maximum, float fallback)
	{
		if (value >= minimum && value <= maximum)
			return value;

		Print(LOG_PREFIX + fieldName + " " + value.ToString() + " is outside " + minimum.ToString() + " to " + maximum.ToString() + ", using " + fallback.ToString());
		return fallback;
	}
}
