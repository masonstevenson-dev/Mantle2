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

#include "EffectSystem/M2Effect.h"
#include "EffectSystem/M2EffectInstance.h"
#include "EffectSystem/M2EffectManager.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"

void UM2EffectManager::Initialize(UM2Registry* Registry)
{
	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		UClass* TargetClass = *ClassIterator;
			
		if (!TargetClass || !TargetClass->IsChildOf(UM2Effect::StaticClass()))
		{
			continue;
		}
		if (TargetClass->HasAnyFlags(RF_Transient) || TargetClass->HasAnyClassFlags(CLASS_Abstract))
		{
			continue;
		}
		if (TargetClass->GetName().StartsWith(TEXT("SKEL_")) || TargetClass->GetName().StartsWith(TEXT("REINST_")))
		{
			// Note: The warning is here because the RF_Transient check should theoretically catch these. If for
			//       some reason it doesn't, this check is fairly brittle, so there should be a notification.
			M2_LOG(LogM2, Warning, TEXT("Target class is an engine temporary class: %s."), *TargetClass->GetName());
			continue;
		}

		M2_LOG(LogM2, Log, TEXT("Registering effect type %s."), *TargetClass->GetName());

		// Constructs a shared object for this effect type.
		if (!Registry->GetShared<UM2Effect>(TargetClass))
		{
			M2_LOG(LogM2, Error, TEXT("Failed to construct shared object of type %s."), *TargetClass->GetName());
		}
	}
}

void UM2EffectManager::PerformOperation(FM2OperationContext& Ctx)
{
	if (!Ctx.Registry.IsValid())
	{
		M2_LOG(LogM2, Error, TEXT("Registry is missing."))
		return;
	}

	auto* EffectInstances = Ctx.Registry->GetRecordSet<UM2EffectInstance>();
	if (!EffectInstances)
	{
		M2_LOG(LogM2, Error, TEXT("M2EffectInstance RecordSet is missing."))
		return;
	}

	TArrayView<FM2RecordHandle> RecordHandles = EffectInstances->GetHandles();
	TArrayView<FM2EffectMetadata> MetadataArray = EffectInstances->GetFieldArray<FM2EffectMetadata>();

	if (RecordHandles.Num() != MetadataArray.Num())
	{
		M2_LOG(LogM2, Error, TEXT("Unable to perform operation: mismatch between record count and field count."))
		return;
	}

	PendingDeletions.Empty();
	
	for (int32 RecordIndex = 0; RecordIndex < RecordHandles.Num(); ++RecordIndex)
	{
		FM2RecordHandle& RecordHandle = RecordHandles[RecordIndex];
		FM2EffectMetadata& EffectMetadata = MetadataArray[RecordIndex];

		UM2Effect* TargetEffect = Ctx.Registry->GetShared<UM2Effect>(EffectMetadata.Effect);

		if (!TargetEffect)
		{
			// TODO(): increment stat counter.
			EffectMetadata.State = EM2EffectState::Delete;
			PendingDeletions.Add(RecordHandle);
			continue;
		}
		
		FM2EffectContext EffectContext;
		EffectContext.World = Ctx.World.Get();
		EffectContext.Registry = Ctx.Registry.Get();

		if (EffectMetadata.State == EM2EffectState::Scheduled)
		{
			if (!EffectMetadata.HasRemainingTriggers() || !EffectMetadata.HasRemainingDuration())
			{
				EffectMetadata.State = EM2EffectState::Finished;
				continue;
			}

			// No need for any pre-tick processing, since this is the first tick.
			EM2EffectTriggerResponse Response = TargetEffect->TickEffect(EffectContext, EffectMetadata);
			if (Response == EM2EffectTriggerResponse::Continue)
			{
				EffectMetadata.PostTick();
			}
			else if (Response == EM2EffectTriggerResponse::Done)
			{
				TargetEffect->OnFinishEffect(EffectContext, EffectMetadata);
				EffectMetadata.State = EM2EffectState::Finished;
			}
			else
			{
				EffectMetadata.State = EM2EffectState::Cancel;
			}
		}
		else if (EffectMetadata.State == EM2EffectState::Tick)
		{
			EffectContext.DeltaTime = Ctx.DeltaTime;
			EffectMetadata.TotalElapsedTime += EffectContext.DeltaTime;
			EffectMetadata.TriggerElapsedTime += EffectContext.DeltaTime;
			
			if (!EffectMetadata.HasRemainingTriggers() || !EffectMetadata.HasRemainingDuration())
			{
				EffectMetadata.State = EM2EffectState::Finished;
				continue;
			}
			if (!EffectMetadata.IsReadyForTick())
			{
				continue;
			}

			EffectMetadata.PreTick();
			
			EM2EffectTriggerResponse Response = TargetEffect->TickEffect(EffectContext, EffectMetadata);
			if (Response == EM2EffectTriggerResponse::Continue)
			{
				EffectMetadata.PostTick();
			}
			else if (Response == EM2EffectTriggerResponse::Done)
			{
				EffectMetadata.State = EM2EffectState::Finished;
			}
			else
			{
				EffectMetadata.State = EM2EffectState::Cancel;
			}
		}
		else if (EffectMetadata.State == EM2EffectState::Cancel)
		{
			TargetEffect->OnCancelEffect(EffectContext, EffectMetadata);
			EffectMetadata.State = EM2EffectState::Delete;
		}
		else if (EffectMetadata.State == EM2EffectState::Finished)
		{
			TargetEffect->OnFinishEffect(EffectContext, EffectMetadata);
			EffectMetadata.State = EM2EffectState::Delete;
		}
		else if (EffectMetadata.State == EM2EffectState::Delete)
		{
			TargetEffect->OnDeleteEffect(EffectContext, EffectMetadata);
			PendingDeletions.Add(RecordHandle);
		}
		else
		{
			M2_LOG(LogM2, Error, TEXT("Unsupported effect state: %s. Marking effect for deletion."), *UEnum::GetValueAsString(EffectMetadata.State));
			EffectMetadata.State = EM2EffectState::Delete;
			break;
		}
	}

	for (FM2RecordHandle& EffectHandle : PendingDeletions)
	{
		Ctx.Registry->RemoveRecord(EffectHandle);
	}
	PendingDeletions.Empty();
}
