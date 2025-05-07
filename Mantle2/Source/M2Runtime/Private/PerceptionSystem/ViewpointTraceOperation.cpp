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
#include "PerceptionSystem/ViewpointTraceOperation.h"

#include "Components/M2AvatarComponent.h"
#include "Fields/Avatar.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"

void UViewpointTraceOperation::Initialize(UM2Registry* Registry)
{
	Super::Initialize(Registry);

	IncludeFields.Empty();
	ExcludeFields.Empty();
	IncludeFields.Add(FLD_AvatarActor::StaticStruct());
}

void UViewpointTraceOperation::PerformOperation(FM2OperationContext& Ctx)
{
	if (!Ctx.Registry.IsValid() || !Ctx.World.IsValid())
	{
		return;
	}

	UM2Registry* Registry = Ctx.Registry.Get();

	for (UM2RecordSet* ResultSet : Registry->GetAll(IncludeFields, ExcludeFields))
	{
		if (!ResultSet)
		{
			M2_LOG(LogM2, Error, TEXT("Invalid record set."));
			continue;
		}

		TArrayView<FM2RecordHandle> RecordHandles = ResultSet->GetHandles();
		TArrayView<FLD_AvatarActor> AvatarFields = ResultSet->GetFieldArray<FLD_AvatarActor>();
		TArrayView<FLD_VPTBase> VPTFields = GetVPTData(ResultSet);

		for (int32 RecordIndex = 0; RecordIndex < RecordHandles.Num(); ++RecordIndex)
		{
			FM2RecordHandle& SourceHandle = RecordHandles[RecordIndex];
			FLD_AvatarActor& SourceAvatar = AvatarFields[RecordIndex];
			FLD_VPTBase& SourceVPT = VPTFields[RecordIndex];

			if (!SourceAvatar.AvatarActor.IsValid())
			{
				M2_LOG(LogM2, Error, TEXT("SourceAvatar.AvatarActor is invalid."));
				continue;
			}
			if (!SourceVPT.bEnabled)
			{
				SourceVPT.DeltaTime = -1.0f;
				SourceVPT.LastHit = FM2RecordHandle();
				continue;
			}

			if (SourceVPT.DeltaTime < 0.0f || SourceVPT.DeltaTime >= TraceOptions.ScanRateSec)
			{
				SourceVPT.DeltaTime = 0.0f;
			}
			else
			{
				SourceVPT.DeltaTime += TraceOptions.ScanRateSec;
				continue;
			}

			AActor* ViewpointActor = SourceAvatar.AvatarActor.Get();
			FM2RecordHandle Hit;
			TArray<FM2RecordHandle> Overlaps;
			
			PerformLineTrace(Ctx, *ViewpointActor, Hit, Overlaps);
			ProcessTraceResults(Ctx, SourceHandle, SourceVPT, Hit, Overlaps);
			SourceVPT.LastHit = Hit;
		}
	}
}


TArrayView<FLD_VPTBase> UViewpointTraceOperation::GetVPTData(UM2RecordSet* ResultSet)
{
	return TArrayView<FLD_VPTBase>();
}

void UViewpointTraceOperation::PerformLineTrace(
	FM2OperationContext& Ctx,
	AActor& ViewpointActor,
	FM2RecordHandle& OutHit,
	TArray<FM2RecordHandle>& OutOverlaps)
{
	FVector TraceStartPoint;
	FRotator TraceStartRotator;
	ViewpointActor.GetActorEyesViewPoint(TraceStartPoint, TraceStartRotator);
	FVector TraceDirection = TraceStartRotator.Vector();
	FVector TraceEndPoint = TraceStartPoint + (TraceDirection * TraceOptions.ScanRange);

	FCollisionQueryParams QueryParams;
	QueryParams.TraceTag = SCENE_QUERY_STAT(DustVPT);
	QueryParams.bTraceComplex = false;

	QueryParams.AddIgnoredActor(&ViewpointActor);

	TArray<FHitResult> TraceResults;
	Ctx.World->LineTraceMultiByChannel(TraceResults, TraceStartPoint, TraceEndPoint, TraceOptions.TraceChannel, QueryParams);

	FVector DebugCenter;
	FColor DebugColor;
	
	if (TraceResults.Num() > 0)
	{
		DebugCenter = TraceResults[TraceResults.Num() - 1].Location;
		DebugColor = FColor::Yellow;
	}
	else
	{
		DebugCenter = TraceEndPoint;
		DebugColor = FColor::Red;
	}

	for (FHitResult TraceResult : TraceResults)
	{
		AActor* ResultActor = TraceResult.GetActor();
		if (!ResultActor)
		{
			continue;
		}

		auto* AvatarComponent = Cast<UM2AvatarComponent>(ResultActor->GetComponentByClass(UM2AvatarComponent::StaticClass()));
		
		if (AvatarComponent && Ctx.Registry->HasRecord(AvatarComponent->GetRecordHandle()))
		{
			FM2RecordHandle TargetId = AvatarComponent->GetRecordHandle();
			
			if (TraceResult.bBlockingHit)
			{
				DebugColor = FColor::Green;
				OutHit = TargetId;
			}
			else
			{
				OutOverlaps.Add(TargetId);
			}
		}	
	}

	if (TraceOptions.bDrawDebugGeometry)
	{
		DrawDebugSphere(
			Ctx.World.Get(),
			DebugCenter,
			DebugData.Radius,
			DebugData.Segments,
			DebugColor,
			DebugData.bPersistentLines,
			DebugData.LifeTime,
			DebugData.DepthPriority,
			DebugData.Thickness
		);
	}
}

