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
#include "Fields/VPTBase.h"
#include "Foundation/M2Operation.h"

#include "ViewpointTraceOperation.generated.h"

namespace M2
{
	struct VPTOptions
	{
		float ScanRange = 200.0f;
		double ScanRateSec = 0.01;
		ECollisionChannel TraceChannel = ECC_Visibility;
		bool bDrawDebugGeometry = false;
	};
	
	struct VPTDebugSphereData
	{
		float Radius = 5.0f;
		int32 Segments = 16;
		bool bPersistentLines = false;
		float LifeTime = -1.0; 
		uint8 DepthPriority = 0; 
		float Thickness = 0.0;

		FColor DefaultColor = FColor::Black;
		FColor NoHitsColor = FColor::Red;
		FColor UnknownColor = FColor::Yellow;
		FColor HitColor = FColor::Green;
	};
}

UCLASS()
class M2RUNTIME_API UViewpointTraceOperation : public UM2Operation
{
	GENERATED_BODY()

public:
	virtual void Initialize(UM2Registry* Registry) override;
	virtual void PerformOperation(FM2OperationContext& Ctx) override;
	
protected:
	virtual TArrayView<FLD_VPTBase> GetVPTData(UM2RecordSet* ResultSet);
	
	virtual void ProcessTraceResults(
		FM2OperationContext& Ctx,
		FM2RecordHandle& SourceHandle,
		FLD_VPTBase& SourceVPT,
		FM2RecordHandle& Hit,
		TArray<FM2RecordHandle>& Overlaps
	) {}
	
	void PerformLineTrace(
		FM2OperationContext& Ctx,
		AActor& ViewpointActor,
		FM2RecordHandle& OutHit,
		TArray<FM2RecordHandle>& OutOverlaps
	);

	// init these in Operation::Initialize()
	TArray<UScriptStruct*> IncludeFields;
	TArray<UScriptStruct*> ExcludeFields;
	M2::VPTOptions TraceOptions;
	M2::VPTDebugSphereData DebugData;
};
