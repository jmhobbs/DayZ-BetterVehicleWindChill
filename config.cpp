class CfgPatches
{
	class BetterVehicleWindChill
	{
		units[] = {};
		weapons[] = {};
		requiredVersion = 0.1;
		requiredAddons[] = {
			"DZ_Data",
			"DZ_Scripts"
		};
	};
};

class CfgMods
{
	class BetterVehicleWindChill
	{
		type = "mod";
		author = "Hopps";
		name = "BetterVehicleWindChill";
		dir = "BetterVehicleWindChill";
		dependencies[] = {"World", "Mission"};
		class defs
		{
			class worldScriptModule
			{
				value = "";
				files[] = {"BetterVehicleWindChill/scripts/4_world"};
			};
			class missionScriptModule
			{
				value = "";
				files[] = {"BetterVehicleWindChill/scripts/5_mission"};
			};
		};
	};
};
