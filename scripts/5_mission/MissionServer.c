modded class MissionServer
{
	override void OnInit()
	{
		super.OnInit();

		BetterVehicleWindChill_Scripts_4_World_Classes_WindChillService.GetInstance().Preload();
	}
}
