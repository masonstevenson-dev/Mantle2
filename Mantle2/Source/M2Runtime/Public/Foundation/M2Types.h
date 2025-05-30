﻿// Copyright © Mason Stevenson
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
struct M2RUNTIME_API FM2RecordHandle
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

	// Returns true if this handle is non-null. This does not mean the handle points to a valid record.
	bool IsSet() const { return SetId.IsValid() && RecordId.IsValid(); }

	// Returns true if this handle is non-null and points to a valid record.
	bool IsValid(UM2Registry* Registry) const;

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
enum class EM2EffectState: uint8
{
	Scheduled,
	Tick,
	Cancel,
	Finished,
	Delete
};

UENUM()
enum class EM2EffectTriggerResponse
{
	Continue,
	Done,
	Cancel
};

USTRUCT()
struct M2RUNTIME_API FM2EffectContext
{
	GENERATED_BODY()

public:
	UPROPERTY(Transient)
	TObjectPtr<UWorld> World;

	UPROPERTY(Transient)
	TObjectPtr<UM2Registry> Registry;

	// frame time, in seconds.
	UPROPERTY(Transient)
	float DeltaTime = 0.0f;
};

USTRUCT(BlueprintType)
struct M2RUNTIME_API FM2EffectMetadata
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

	FM2EffectMetadata& WithTimeLimit(float InTimeLimit)
	{
		if (State == EM2EffectState::Scheduled)
		{
			MaxDuration = InTimeLimit < 0.0 ? kUnlimitedDuration : InTimeLimit;
		}
		
		return *this;
	}

	FM2EffectMetadata& WithTriggerLimit(int32 InTriggers)
	{
		if (State == EM2EffectState::Scheduled)
		{
			TriggerLimit = InTriggers < 0 ? kUnlimitedTriggers : InTriggers;
		}
		
		return *this;
	}

	FM2EffectMetadata& WithInstigator(const FM2RecordHandle& RecordHandle)
	{
		if (State == EM2EffectState::Scheduled)
		{
			Instigator = RecordHandle;
		}
		
		return *this;
	}

	FM2EffectMetadata& WithTarget(const FM2RecordHandle& RecordHandle)
	{
		if (State == EM2EffectState::Scheduled)
		{
			Target = RecordHandle;
		}
		
		return *this;
	}

	FM2EffectMetadata& WithInstanceData(const FM2RecordHandle& RecordHandle)
	{
		if (State == EM2EffectState::Scheduled)
		{
			InstanceDataHandle = RecordHandle;
		}
		
		return *this;
	}

	bool HasRemainingTriggers() const
	{
		return TriggerLimit == FM2EffectMetadata::kUnlimitedTriggers || TriggerCount < TriggerLimit;
	}

	bool HasRemainingDuration() const
	{
		return MaxDuration == FM2EffectMetadata::kUnlimitedDuration || TotalElapsedTime <= MaxDuration;
	}

	void CancelEffect()
	{
		State = EM2EffectState::Cancel;
	}

	bool HasEverTicked() const
	{
		return TriggerCount > 0;
	}

	FM2RecordHandle GetInstigator() const
	{
		return Instigator;
	}
	
	FM2RecordHandle GetTarget() const
	{
		return Target;
	}

	FM2RecordHandle GetInstanceData() const
	{
		return InstanceDataHandle;
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
			State = EM2EffectState::Finished;
		}
	}
	
	UPROPERTY()
	TSubclassOf<UM2Effect> Effect;

	UPROPERTY()
	EM2EffectState State = EM2EffectState::Scheduled;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float TriggerRateSec = 0.0f;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	float MaxDuration = 0.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	int32 TriggerLimit = 0;

	UPROPERTY()
	float TotalElapsedTime = 0.0f;

	UPROPERTY()
	float TriggerElapsedTime = 0.0f;

	UPROPERTY()
	int32 TriggerCount = 0;

	UPROPERTY()
	FM2RecordHandle Instigator = FM2RecordHandle();

	UPROPERTY()
	FM2RecordHandle Target = FM2RecordHandle();
	
	UPROPERTY()
	FM2RecordHandle InstanceDataHandle = FM2RecordHandle();
};