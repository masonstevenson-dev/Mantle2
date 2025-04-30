// Copyright © Mason Stevenson
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted (subject to the limitations in the disclaimer
// below) provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived from
//    this software without specific prior written permission.
//
// NO EXPRESS OR IMPLIED LICENSES TO ANY PARTY'S PATENT RIGHTS ARE GRANTED BY
// THIS LICENSE. THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND
// CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT
// NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
// PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
// OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
// OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
// ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "M2Types.generated.h"

class TestSuite;
class UM2EffectManager;
class UM2Effect;
class UM2RecordSet;
class UM2Registry;
class UM2Script;

USTRUCT()
struct FM2RecordHandle
{
	GENERATED_BODY()

public:
	FM2RecordHandle() = default;
	FM2RecordHandle(FGuid NewSetId, FGuid NewRecordId) : SetId(NewSetId), RecordId(NewRecordId) {}

	bool operator ==(const FM2RecordHandle& Other) const
	{
		return SetId == Other.SetId && RecordId == Other.RecordId;
	}

	void Clear()
	{
		SetId = FGuid();
		RecordId = FGuid();
	}

	bool IsValid(UM2Registry* Registry);

private:
	friend UM2Registry;
	friend UM2RecordSet;
	friend TestSuite;
	
	UPROPERTY()
	FGuid SetId = FGuid();
	
	UPROPERTY()
	FGuid RecordId = FGuid();
};

UENUM()
enum class EM2EffectState
{
	Scheduled,
	Tick,
	Cancel,
	Delete
};

USTRUCT()
struct FM2EffectContext
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TObjectPtr<UWorld> World;

	// frame time, in seconds.
	UPROPERTY()
	float DeltaTime = 0.0f;
};

USTRUCT()
struct FM2EffectMetadata
{
	GENERATED_BODY()

	// constants
public:
	static constexpr int kUnlimitedTriggers = -1;
	static constexpr double kUnlimitedDuration = -1.0;

public:
	static FM2EffectMetadata MakeOneTimeEffect(TSubclassOf<UM2Effect> InEffect)
	{
		FM2EffectMetadata NewMetadata;

		NewMetadata.Effect = InEffect;
		NewMetadata.TriggerRateSec = 1.0;
		NewMetadata.MaxDuration = 1.0;
		NewMetadata.TriggerLimit = 1;

		return NewMetadata;
	}
	
	static FM2EffectMetadata MakeRecurringEffect(TSubclassOf<UM2Effect> InEffect, float InTriggerRateSec)
	{
		FM2EffectMetadata NewMetadata;

		NewMetadata.Effect = InEffect;
		NewMetadata.TriggerRateSec = InTriggerRateSec;
		NewMetadata.MaxDuration = kUnlimitedDuration;
		NewMetadata.TriggerLimit = kUnlimitedTriggers;

		return NewMetadata;
	}

	FM2EffectMetadata& WithInstanceData(FM2RecordHandle& RecordHandle)
	{
		InstanceDataHandle = RecordHandle;
		return *this;
	}

	FM2EffectMetadata& WithTimeLimit(float InTimeLimit)
	{
		MaxDuration = InTimeLimit < 0.0 ? kUnlimitedDuration : InTimeLimit;
		return *this;
	}

	FM2EffectMetadata& WithTriggerLimit(int32 InTriggers)
	{
		TriggerLimit = InTriggers < 0 ? kUnlimitedTriggers : InTriggers;
		return *this;
	}

	bool HasRemainingTriggers()
	{
		return TriggerLimit == FM2EffectMetadata::kUnlimitedTriggers || TriggerCount < TriggerLimit;
	}

	bool HasRemainingDuration()
	{
		return MaxDuration == FM2EffectMetadata::kUnlimitedDuration || TotalElapsedTime <= MaxDuration;
	}

	void CancelEffect()
	{
		State = EM2EffectState::Cancel;
	}

	bool HasEverTicked()
	{
		return TriggerCount > 0;
	}

protected:
	friend UM2EffectManager;

	bool IsReadyForTick()
	{
		return TriggerElapsedTime >= TriggerRateSec;
	}
	
	void PreTick()
	{
		TriggerElapsedTime = 0.0f;
	}

	void PostTick()
	{
		TriggerCount++;

		if (HasRemainingTriggers())
		{
			State = EM2EffectState::Tick;
		}
		else
		{
			State = EM2EffectState::Delete;
		}
	}
	
	UPROPERTY()
	TSubclassOf<UM2Effect> Effect;

	UPROPERTY()
	EM2EffectState State = EM2EffectState::Scheduled;

	UPROPERTY()
	float TriggerRateSec = 0.0f;
	
	UPROPERTY()
	float MaxDuration = 0.0f;

	UPROPERTY()
	float TotalElapsedTime = 0.0f;

	UPROPERTY()
	float TriggerElapsedTime = 0.0f;

	UPROPERTY()
	uint32 TriggerCount = 0;

	UPROPERTY()
	uint32 TriggerLimit = 0;

	UPROPERTY()
	FM2RecordHandle InstanceDataHandle = FM2RecordHandle();
};