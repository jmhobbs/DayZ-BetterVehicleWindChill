# BetterVehicleWindChill

Server-side DayZ mod that makes the 1.30 boat and motorbike wind chill configurable, and milder by default.

## Why

DayZ 1.30 added a speed penalty to heat comfort on boats and motorbikes. 1.29 had none. Above 15 km/h on a boat, or 20 km/h on a motorbike, vanilla subtracts up to 0.8 from the player's target heat comfort. Heat comfort runs from -1 to 1, and the HUD turns dark blue at -0.45.

Three things make it harsher than the constants suggest:

- The speed remap divides the raw speed by `SPEED_MAX - SPEED_MIN` instead of subtracting the minimum first. On a boat that is 35, so the full penalty arrives at 35 km/h, not the documented 50. The motorbike reaches it at 80 km/h, not 100.
- The penalty is applied after the heat buffer step, so a player warmed by a fire gets no protection. The heat buffer also stops draining or filling during the ride.
- Boats have no way to lower it. `BoatScript.m_HeatComfortOverride` is always 0. On a motorbike the front shield lowers the maximum by 0.15.

## What the mod changes

- The penalty uses the configured speeds and maximum, remapped from the minimum to the maximum speed on the same ease-in cubic curve as vanilla.
- With `RespectHeatBuffer` on (the default), the penalty lowers target heat comfort before the heat buffer step, like any other cold. A warm player keeps the heat buffer protection while it lasts, and the buffer drains faster in the wind.
- With `RespectHeatBuffer` off, the penalty is applied the vanilla 1.30 way, after the heat buffer step, using the configured numbers.
- Setting `BoatEnabled` or `MotorbikeEnabled` to false turns that vehicle's penalty off, as in 1.29. Normal wind chill still applies.

Penalty subtracted from target heat comfort:

| Boat speed | Vanilla 1.30 | Default config |
|---|---|---|
| 20 km/h | 0.15 | 0.00 |
| 25 km/h | 0.29 | 0.01 |
| 30 km/h | 0.50 | 0.02 |
| 35 km/h | 0.80 | 0.06 |
| 40 km/h | 0.80 | 0.11 |
| 45 km/h | 0.80 | 0.19 |
| 50 km/h | 0.80 | 0.30 |

| Motorbike speed | Vanilla 1.30 | Default config |
|---|---|---|
| 40 km/h | 0.10 | 0.01 |
| 60 km/h | 0.34 | 0.06 |
| 80 km/h | 0.80 | 0.21 |
| 100 km/h | 0.80 | 0.50 |

The motorbike values are without the front shield, which lowers the maximum by 0.15 in both.

## Install

- Load it with `-servermod`, for example `-servermod=@BetterVehicleWindChill`.
- Clients do not need it, and it needs no key. Heat comfort is only calculated on the server.

## Configuration

The config file is created on server startup at:

```text
$profile:BetterVehicleWindChill.json
```

Default config:

```json
{
  "RespectHeatBuffer": 1,
  "BoatEnabled": 1,
  "BoatSpeedMinKmh": 15,
  "BoatSpeedMaxKmh": 50,
  "BoatMaxPenalty": 0.3,
  "MotorbikeEnabled": 1,
  "MotorbikeSpeedMinKmh": 20,
  "MotorbikeSpeedMaxKmh": 100,
  "MotorbikeMaxPenalty": 0.5,
  "DebugLog": 0
}
```

| Field | Type | Range | Meaning |
|---|---|---|---|
| `RespectHeatBuffer` | `bool` | | Applies the penalty before the heat buffer step. When off, it is applied after it, as in vanilla 1.30. |
| `BoatEnabled` | `bool` | | When off, boats have no speed penalty, as in 1.29. |
| `BoatSpeedMinKmh` | `float` | 0 to 200 | The penalty starts above this speed. Vanilla is 15. |
| `BoatSpeedMaxKmh` | `float` | 1 to 300 | Full penalty at and above this speed. Must be above `BoatSpeedMinKmh`. Vanilla is 50. |
| `BoatMaxPenalty` | `float` | 0 to 1 | Subtracted from target heat comfort at full speed. Vanilla is 0.8. |
| `MotorbikeEnabled` | `bool` | | When off, motorbikes have no speed penalty, as in 1.29. |
| `MotorbikeSpeedMinKmh` | `float` | 0 to 200 | The penalty starts above this speed. Vanilla is 20. |
| `MotorbikeSpeedMaxKmh` | `float` | 1 to 300 | Full penalty at and above this speed. Must be above `MotorbikeSpeedMinKmh`. Vanilla is 100. |
| `MotorbikeMaxPenalty` | `float` | 0 to 1 | Subtracted from target heat comfort at full speed. Windshield parts lower it, as in vanilla. Vanilla is 0.8. |
| `DebugLog` | `bool` | | Logs speed, penalty, heat comfort and heat buffer every 3 s for each player on a boat or motorbike. |

Out-of-range values fall back to the default and log one line. If a maximum speed is not above its minimum, both fall back to the defaults. The file on disk is not changed. Restart the server to load changes.

Speeds are the vehicle's speedometer reading. For close to the vanilla boat curve with the heat buffer respected, use `BoatSpeedMinKmh` 0, `BoatSpeedMaxKmh` 35 and `BoatMaxPenalty` 0.8. The only difference is that vanilla has no penalty at or below 15 km/h. For the vanilla motorbike penalty at top speed, use `MotorbikeMaxPenalty` 0.8.

## Log lines

All lines start with `[BetterVehicleWindChill]` and go to the script log.

```text
[BetterVehicleWindChill] Started. Boat 15 to 50 km/h, max penalty 0.3. Motorbike 20 to 100 km/h, max penalty 0.5. Respect heat buffer true, debug log false.
[BetterVehicleWindChill] Player 76561190000000000 on Boat_01_Orange at 38 km/h, penalty 0.09 (before heat buffer). Target heat comfort 0, -0.2 before the penalty and heat buffer. Heat comfort 0.12, heat buffer 41.5%
```

Every value in a debug line is from the same tick:

- The target is what heat comfort moves toward. While the heat buffer is active, vanilla holds it at 0 or above, as in the example.
- The second value is the same target without the penalty and before the heat buffer, so it shows the cold from weather and clothing alone.
- Heat comfort follows the target as an average over the last 60 s, so it lags behind changes in speed.
- The heat buffer is a percent of its maximum. With the penalty before the heat buffer, it should drop faster at speed.

## Notes

- Other mods that override `Environment` keep working as long as they call super.
- Cars with the engine on are unchanged: vanilla still holds heat comfort at neutral inside them.
