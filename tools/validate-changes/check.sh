#!/usr/bin/env bash
# Compares the vanilla scripts that BetterVehicleWindChill overrides between the
# baseline DayZ build and a newer build in BohemiaInteractive/DayZ-Script-Diff.
#
# Usage: check.sh [--update-baseline] [commit]
#   commit             DayZ-Script-Diff commit to check (default: latest on the default branch)
#   --update-baseline  record that commit as the new baseline (after review)
# Exit: 0 = unchanged (or baseline updated), 1 = changed (review needed), 2 = could not run.

set -u

UPDATE=0
if [ "${1:-}" = "--update-baseline" ]; then
	UPDATE=1
	shift
fi

SKILL_DIR="$(cd "$(dirname "$0")" && pwd)"
VERSION_FILE="$SKILL_DIR/baseline/VERSION"
REPO="BohemiaInteractive/DayZ-Script-Diff"
API="https://api.github.com/repos/$REPO"
RAW="https://raw.githubusercontent.com/$REPO"

ENVIRONMENT="scripts/4_world/classes/environment/environment.c"
CONSTANTS="scripts/3_game/constants.c"
BOAT="scripts/3_game/vehicles/boat.c"
MOTORBIKE="scripts/3_game/vehicles/motorbike.c"
BOATSCRIPT="scripts/4_world/entities/vehicles/boatscript.c"
MOTORBIKESCRIPT="scripts/4_world/entities/vehicles/motorbikescript.c"
EASING="scripts/3_game/static/easing.c"
FILES="$ENVIRONMENT $CONSTANTS $BOAT $MOTORBIKE $BOATSCRIPT $MOTORBIKESCRIPT $EASING"

for tool in curl python diff awk; do
	command -v "$tool" > /dev/null || { echo "ERROR: $tool not found"; exit 2; }
done
[ -f "$VERSION_FILE" ] || { echo "ERROR: missing $VERSION_FILE"; exit 2; }

BASE_SHA="$(sed -n 's/^commit=//p' "$VERSION_FILE" | tr -d '\r')"
[ -n "$BASE_SHA" ] || { echo "ERROR: no commit= line in $VERSION_FILE"; exit 2; }

# Prints "<full sha> <first line of the commit message>" from a commit or commit list response
commit_info() {
	curl -sf "$1" | python -I -c "import sys, json; c = json.load(sys.stdin); c = c[0] if isinstance(c, list) else c; print(c['sha'], c['commit']['message'].splitlines()[0])"
}

if [ -n "${1:-}" ]; then
	NEW_INFO="$(commit_info "$API/commits/$1")"
else
	NEW_INFO="$(commit_info "$API/commits?per_page=1")"
fi
[ -n "$NEW_INFO" ] || { echo "ERROR: could not read the commit from the GitHub API (offline, bad commit, or rate limited)"; exit 2; }
NEW_SHA="${NEW_INFO%% *}"
NEW_BUILD="${NEW_INFO#* }"

echo "== DayZ-Script-Diff"
echo "baseline: $(tr -d '\r' < "$VERSION_FILE" | tr '\n' ' ')"
echo "checking: $NEW_SHA $NEW_BUILD"

if [ "$UPDATE" -eq 1 ]; then
	printf 'commit=%s\nbuild=%s\ncaptured=%s\n' "$NEW_SHA" "$NEW_BUILD" "$(date +%Y-%m-%d)" > "$VERSION_FILE"
	echo
	echo "RESULT: baseline updated to $NEW_SHA"
	exit 0
fi

if [ "$NEW_SHA" = "$BASE_SHA" ]; then
	echo
	echo "RESULT: this commit is the baseline, nothing to check."
	exit 0
fi

WORK="$(mktemp -d)"
echo
echo "== Downloading to $WORK"
for f in $FILES; do
	for side in base new; do
		sha="$BASE_SHA"
		[ "$side" = "new" ] && sha="$NEW_SHA"
		mkdir -p "$WORK/$side/$(dirname "$f")"
		if curl -sf -o "$WORK/$side/$f.tmp" "$RAW/$sha/$f"; then
			tr -d '\r' < "$WORK/$side/$f.tmp" > "$WORK/$side/$f"
			rm -f "$WORK/$side/$f.tmp"
		elif [ "$side" = "base" ]; then
			echo "ERROR: $f is not in the baseline commit"
			exit 2
		fi
	done
done

changed=0
flag() { echo "CHANGED: $*"; changed=$((changed + 1)); }

# Prints a block from the first line containing the signature to the closing brace at the same indent
extract() {
	[ -f "$1" ] || return 0
	awk -v sig="$2" '
		!found && index($0, sig) { found = 1; match($0, /^[\t ]*/); indent = substr($0, 1, RLENGTH) }
		found {
			print
			line = $0
			sub(/[\t ]+$/, "", line)
			if (line == indent "}" || line == indent "};")
				exit
		}
	' "$1"
}

# Diffs one block between the baseline and the checked commit
check_block() {
	local file="$1" sig="$2"
	extract "$WORK/base/$file" "$sig" > "$WORK/block_base.txt"
	extract "$WORK/new/$file" "$sig" > "$WORK/block_new.txt"
	if [ ! -s "$WORK/block_new.txt" ]; then
		flag "$sig is gone from $file (renamed, moved or removed)"
		return
	fi
	if ! diff -q "$WORK/block_base.txt" "$WORK/block_new.txt" > /dev/null; then
		flag "$sig in $file"
		diff -u --label "baseline" --label "new" "$WORK/block_base.txt" "$WORK/block_new.txt" | sed 's/^/    /'
	fi
}

# Diffs the matching line plus the next few lines, for code that is not a braced block
check_lines() {
	local file="$1" text="$2" count="$3"
	grep -F -A"$count" "$text" "$WORK/base/$file" > "$WORK/lines_base.txt"
	grep -F -A"$count" "$text" "$WORK/new/$file" > "$WORK/lines_new.txt" 2> /dev/null
	if [ ! -s "$WORK/lines_new.txt" ]; then
		flag "$text is gone from $file (renamed, moved or removed)"
		return
	fi
	if ! diff -q "$WORK/lines_base.txt" "$WORK/lines_new.txt" > /dev/null; then
		flag "$text in $file"
		diff -u --label "baseline" --label "new" "$WORK/lines_base.txt" "$WORK/lines_new.txt" | sed 's/^/    /'
	fi
}

# Checks that a member our modded class uses is still declared and still accessible
check_member() {
	local file="$1" pattern="$2"
	grep -Eq "$pattern" "$WORK/new/$file" 2> /dev/null || flag "no match for /$pattern/ in $file (compile error or private now)"
}

echo
echo "== Whole files"
for f in $FILES; do
	if [ ! -f "$WORK/new/$f" ]; then
		flag "$f is not in the checked commit"
	elif diff -q "$WORK/base/$f" "$WORK/new/$f" > /dev/null; then
		echo "same:    $f"
	else
		echo "differs: $f ($(diff "$WORK/base/$f" "$WORK/new/$f" | grep -c '^[<>]') changed lines)"
	fi
done

echo
echo "== Members the mod uses"
before=$changed
check_member "$ENVIRONMENT" 'protected PlayerBase[[:space:]]+m_Player;'
check_member "$ENVIRONMENT" 'protected float[[:space:]]+m_HeatComfort;'
check_member "$ENVIRONMENT" 'protected float[[:space:]]+m_TargetHeatComfort;'
check_member "$ENVIRONMENT" 'protected ref SimpleMovingAverage<float>[[:space:]]+m_AverageHeatComfortBuffer;'
check_member "$ENVIRONMENT" 'protected int[[:space:]]+m_HeatComfortBehaviorCategory;'
check_member "$ENVIRONMENT" 'protected bool DetermineHeatcomfortBehavior\(\)'
check_member "$ENVIRONMENT" 'protected void SetHeatcomfortDirectly\(\)'
check_member "$ENVIRONMENT" 'protected void ProcessHeatComfort\(\)'
check_member "$ENVIRONMENT" 'protected void SetTargetHeatComfort\(float value\)'
check_member "$ENVIRONMENT" 'protected float EnvTempToCoef\(float pTemp\)'
check_member "$ENVIRONMENT" 'protected ref EnvironmentSnapshotData[[:space:]]+m_EnvironmentSnapshot;'
check_member "$ENVIRONMENT" 'protected float GetApplicableHeatbuffer\(\)'
check_member "$BOAT" 'float GetSpeedometerAbsolute\(\)'
check_member "$MOTORBIKE" 'float GetSpeedometerAbsolute\(\)'
check_member "$BOATSCRIPT" 'float GetHeatComfortOverride\(\)'
check_member "$MOTORBIKESCRIPT" 'float GetHeatComfortOverride\(\)'
check_member "$EASING" 'static float EaseInCubic'
[ "$changed" -eq "$before" ] && echo "all present"

echo
echo "== Code the mod replaces or depends on"
before=$changed
check_block "$ENVIRONMENT" "enum EEnvironmentHeatcomfortBehaviorCategory"
check_lines "$ENVIRONMENT" "if (DetermineHeatcomfortBehavior())" 3
check_block "$ENVIRONMENT" "protected bool DetermineHeatcomfortBehavior()"
check_block "$ENVIRONMENT" "protected void SetHeatcomfortDirectly()"
check_block "$ENVIRONMENT" "protected void ProcessHeatComfort()"
check_block "$ENVIRONMENT" "protected void SetTargetHeatComfort(float value)"
check_block "$ENVIRONMENT" "protected void ProcessHeatBuffer(EnvironmentSnapshotData data)"
check_block "$ENVIRONMENT" "protected float EnvTempToCoef(float pTemp)"
check_block "$ENVIRONMENT" "class EnvironmentSnapshotData"
check_block "$ENVIRONMENT" "protected float GetApplicableHeatbuffer()"
check_block "$BOAT" "float GetSpeedometerAbsolute()"
check_block "$MOTORBIKE" "float GetSpeedometerAbsolute()"
check_block "$BOATSCRIPT" "float GetHeatComfortOverride()"
check_block "$MOTORBIKESCRIPT" "float GetHeatComfortOverride()"
check_block "$EASING" "static float EaseInCubic"
[ "$changed" -eq "$before" ] && echo "all unchanged"

echo
echo "== Constants and heat comfort overrides"
grep -E 'ENVIRO_HEATCOMFORT_(BOAT|MOTORBIKE)_' "$WORK/base/$CONSTANTS" | tr -s ' \t' ' ' > "$WORK/constants_base.txt"
grep -E 'ENVIRO_HEATCOMFORT_(BOAT|MOTORBIKE)_' "$WORK/new/$CONSTANTS" 2> /dev/null | tr -s ' \t' ' ' > "$WORK/constants_new.txt"
if diff -q "$WORK/constants_base.txt" "$WORK/constants_new.txt" > /dev/null; then
	echo "boat and motorbike constants unchanged"
else
	flag "boat or motorbike heat comfort constants in $CONSTANTS"
	diff -u --label "baseline" --label "new" "$WORK/constants_base.txt" "$WORK/constants_new.txt" | sed 's/^/    /'
fi

# The baseline only ever sets the boat override to 0; a new assignment means boats can now lower the penalty
grep -n 'm_HeatComfortOverride' "$WORK/base/$BOATSCRIPT" | sed 's/^[0-9]*://' | tr -s ' \t' ' ' > "$WORK/boat_override_base.txt"
grep -n 'm_HeatComfortOverride' "$WORK/new/$BOATSCRIPT" 2> /dev/null | sed 's/^[0-9]*://' | tr -s ' \t' ' ' > "$WORK/boat_override_new.txt"
if diff -q "$WORK/boat_override_base.txt" "$WORK/boat_override_new.txt" > /dev/null; then
	echo "boat heat comfort override unchanged"
else
	flag "m_HeatComfortOverride use in $BOATSCRIPT"
	diff -u --label "baseline" --label "new" "$WORK/boat_override_base.txt" "$WORK/boat_override_new.txt" | sed 's/^/    /'
fi

echo
if [ "$changed" -eq 0 ]; then
	echo "RESULT: nothing the mod depends on changed. Record the new baseline with: bash check.sh --update-baseline $NEW_SHA"
	exit 0
fi

echo "RESULT: changes found, review needed. Files are in $WORK (base/ and new/)."
exit 1
