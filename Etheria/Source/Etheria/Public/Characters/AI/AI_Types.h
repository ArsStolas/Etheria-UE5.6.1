/**
* Etheria's End Project, 2025
 * Created by: Mato
 * Class: AITypes - Shared Enums & Structs for AI System
 */

#pragma once

#include "CoreMinimal.h"
#include "AI_Types.generated.h"

/* ───────────── Hostility ───────────── */

UENUM(BlueprintType)
enum class EAIHostilityType : uint8
{
	Passive		UMETA(DisplayName = "Passive"),
	Neutral		UMETA(DisplayName = "Neutral"),
	Aggressive	UMETA(DisplayName = "Aggressive")
};

/* ───────────── Rank (Aggressive only) ───────────── */

UENUM(BlueprintType)
enum class EAIRank : uint8
{
	Basic	UMETA(DisplayName = "Basic"),
	Elite	UMETA(DisplayName = "Elite"),
	Boss	UMETA(DisplayName = "Boss")
};

/* ───────────── AI State ───────────── */

UENUM(BlueprintType)
enum class EAIState : uint8
{
	Idle		UMETA(DisplayName = "Idle"),
	Patrolling	UMETA(DisplayName = "Patrolling"),
	Chasing		UMETA(DisplayName = "Chasing"),
	Attacking	UMETA(DisplayName = "Attacking"),
	Returning	UMETA(DisplayName = "Returning"),
	Interacting	UMETA(DisplayName = "Interacting"),
	Dead		UMETA(DisplayName = "Dead")
};

/* ───────────── Patrol Mode ───────────── */

UENUM(BlueprintType)
enum class EPatrolMode : uint8
{
	Stationary	UMETA(DisplayName = "Stationary"),
	Zone		UMETA(DisplayName = "Zone (Random in radius)"),
	Path		UMETA(DisplayName = "Path (Waypoints)")
};

/* ───────────── Patrol Path Loop Mode ───────────── */

UENUM(BlueprintType)
enum class EPatrolLoopMode : uint8
{
	Loop		UMETA(DisplayName = "Loop"),
	PingPong	UMETA(DisplayName = "Ping-Pong"),
	Once		UMETA(DisplayName = "Once")
};
