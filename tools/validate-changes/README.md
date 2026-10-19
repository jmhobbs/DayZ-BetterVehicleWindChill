
# How to check vehicle wind chill after a DayZ update

DayZ 1.30 added a speed penalty to heat comfort for boats and motorbikes, in vanilla `Environment` (`scripts/4_world/classes/environment/environment.c`).

| Override | What it does | Vanilla code it relies on |
|---|---|---|
| `DetermineHeatcomfortBehavior()` | Calls super, then for any `Boat` or `Motorbike` parent works out the configured penalty. It sets the category back to `DEFAULT` unless `RespectHeatBuffer` is false. | `EEnvironmentHeatcomfortBehaviorCategory.BOAT` / `MOTORBIKE`, `m_HeatComfortBehaviorCategory`, and the `Update()` tick choosing `SetHeatcomfortDirectly()` or `ProcessHeatComfort()` |
| `EnvTempToCoef()` | Subtracts the penalty while the category is `DEFAULT`, so it lands before the heat buffer step | `ProcessHeatComfort()` calling `EnvTempToCoef()` once, inside `targetHeatComfort`, before `ProcessHeatBuffer()` and the heat buffer clamp |
| `SetHeatcomfortDirectly()` | Replaces the vanilla `BOAT` and `MOTORBIKE` cases when `RespectHeatBuffer` is false. Other categories go to super. | The vanilla case steps: `ProcessHeatComfort()`, then subtract from `m_TargetHeatComfort`, `m_AverageHeatComfortBuffer.Add`, `SetTargetHeatComfort`, set the stat |
| `ProcessHeatComfort()` | Calls super, then writes the debug line on the default path. The direct path writes it at the end of `SetHeatcomfortDirectly()`. | `m_EnvironmentSnapshot.m_TargetHeatComfort` holding the target before the heat buffer clamp, and `GetApplicableHeatbuffer()` |
| `WindChillService.SpeedPenalty()` | `(speed - min) / (max - min)` on `Easing.EaseInCubic` | Nothing. It fixes the vanilla remap, which divides the raw speed by `(max - min)`. |

It also calls `GetSpeedometerAbsolute()` on `Boat` / `Motorbike` and `GetHeatComfortOverride()` on `BoatScript` / `MotorbikeScript`.

The mod never calls vanilla's boat or motorbike penalty. If Bohemia moves that penalty out of `SetHeatcomfortDirectly()`, both penalties could apply at once. Additionally they might actually fix their bugs and this mod will not be needed.

`baseline/VERSION` records the DayZ-Script-Diff commit from GitHub that the mod was last checked against.

## 1. Run the check against the new version commit hash

```bash
bash check.sh [commit hash]
```

Run it with the Bash tool (Git Bash). It needs `curl`, `python`, `diff` and `awk`, and internet access. Without a commit it checks the latest commit on `main` of https://github.com/BohemiaInteractive/DayZ-Script-Diff. Each commit message is
`Build <version>, Scripts Rev. <n>`. 

It checks:
- which of the watched files changed at all
- every vanilla member the mod uses, including that the fields are still `protected` (a missing one means a compile error)
- diffs of the code the mod replaces or depends on: the category enum, the `Update()` branch, `DetermineHeatcomfortBehavior`, `SetHeatcomfortDirectly`, `ProcessHeatComfort`, `SetTargetHeatComfort`, `ProcessHeatBuffer`, `EnvTempToCoef`, `GetSpeedometerAbsolute`, `GetHeatComfortOverride`, `EaseInCubic`
- the `ENVIRO_HEATCOMFORT_BOAT_*` / `MOTORBIKE_*` constants
- whether `BoatScript` now sets `m_HeatComfortOverride` anywhere but its constructor

Exit codes:
- 0: nothing relevant changed.
- 1: changes found, review needed. The output says where the downloaded files are.
- 2: the check could not run (offline, bad commit, GitHub rate limit of 60 requests an hour).

## 2. If nothing changed (exit 0)

Run `bash check.sh --update-baseline <commit>` to record the checked commit.  The mod is still valid and we are done.

## 3. If something changed (exit 1)

Read the diffs. Where a diff is not enough, compare the full files in the downloaded `base/` and `new/` folders. Then decide for each change:

| Vanilla change | What to do |
|---|---|
| The vanilla remap now subtracts the minimum, like ours | Our formula matches. Nothing to undo. The mod is still needed for the config and the heat buffer handling. |
| Boat or motorbike constants changed (thresholds, `TARGET_MAX`) | Our defaults don't depend on them, and existing server JSON files keep their own values. Tell the user the new vanilla values and ask whether to change our defaults in `WindChillConfig.c` and the README. |
| Vanilla penalty lowered a lot, or the boat penalty removed | Ask the user whether to retire the boat or motorbike part of the mod, or the whole mod. |
| `BOAT` or `MOTORBIKE` removed from the enum | Compile error in our `Environment.c`. If vanilla dropped the penalty, the mod may not be needed. Ask the user. |
| Penalty moved out of `SetHeatcomfortDirectly()` (for example into `ProcessHeatComfort()` or `GetEnvironmentTemperature()`) | Our override no longer turns it off, so both penalties apply. Find where it went and neutralise it there, or retire our penalty and keep only the config. |
| Vanilla now runs `ProcessHeatBuffer()` or the heat buffer clamp for vehicles | Vanilla now respects the heat buffer. `RespectHeatBuffer` may be redundant. Check that the penalty isn't applied twice, then ask whether to keep the option. |
| `ProcessHeatComfort()` no longer calls `EnvTempToCoef()`, calls it twice, or calls it after `ProcessHeatBuffer()` | The default path loses, doubles or misplaces the penalty. Move our subtraction to the new last term of `targetHeatComfort` before `ProcessHeatBuffer()`. |
| The vanilla `BOAT` / `MOTORBIKE` cases in `SetHeatcomfortDirectly()` changed in other ways (smoothing, clamps, stat handling) | Port the change into our direct path. Keep `m_BetterVehicleWindChill_Penalty` in place of the vanilla penalty. |
| The `Update()` branch changed (`DetermineHeatcomfortBehavior()` no longer chooses between the two paths) | Rework the override so it still picks the path once per tick. |
| New category in the enum (for example a new vehicle type) | Our override doesn't touch it. Tell the user, and offer config for it if it has a penalty. |
| `BoatScript` now sets `m_HeatComfortOverride` (boat parts that block wind) | Nothing. We already subtract it from `BoatMaxPenalty`. Mention it to the user. |
| A member renamed, removed or made private | Fix our references, or the server fails to compile scripts. |

After the mod is updated:

1. Update the root `README.md` if the behaviour, defaults or JSON keys changed.
2. Run `bash check.sh --update-baseline <commit>` so the next check diffs against the new build.
