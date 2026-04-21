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

#include "Foundation/M2RecordSet.h"

#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"

void UM2RecordSet::PreInitialize(FGuid NewSetId)
{
	// TODO(): Initialize should be called anytime this RecordSet gets deserialized.
	SetId = NewSetId;

	AddRecordFns.Empty();
	RemoveRecordFns.Empty();
	GetFieldFns.Empty();
	Archetype.Empty();
}

void UM2RecordSet::Initialize()
{
	// Fatal because forgetting to call M2_INITIALIZE_FIELD is likely to crash the editor anyway.
	M2_LOG(LogM2, Fatal, TEXT("Initialize function not overriden for %s! Please override Initialize and use the M2_INITIALIZE_FIELD macro to initialize your fields."), *GetClass()->GetName());
}

bool UM2RecordSet::MatchArchetype(TArray<UScriptStruct*>& Match, TArray<UScriptStruct*>& Exclude)
{
	for (UScriptStruct* FieldType : Match)
	{
		if (!Archetype.Contains(FieldType))
		{
			return false;
		}
	}
	for (UScriptStruct* FieldType : Exclude)
	{
		if (Archetype.Contains(FieldType))
		{
			return false;
		}
	}
	
	return (Match.Num() + Exclude.Num()) > 0;
}

FM2RecordHandle UM2RecordSet::AddRecordInternal(int32& OutRecordIndex)
{
	RecordHandles.Add(FM2RecordHandle(SetId, FGuid::NewGuid()));
	OutRecordIndex = RecordHandles.Num() - 1;
	RecordIndexMap.Add(RecordHandles.Last().RecordId, OutRecordIndex);
	for (TFunction<void()> AddFunction : AddRecordFns)
	{
		AddFunction();
	}
	
	return RecordHandles.Last();
}

FM2RecordHandle UM2RecordSet::AddRecord()
{
	int32 RecordIndex;
	return AddRecordInternal(RecordIndex);
}

FM2RecordHandle UM2RecordSet::AddAndInitializeRecord(const FGameplayTag& InitID)
{
	// By default, this fn doesn't do anything special. Override it to add your own initialization logic.
	FM2RecordHandle RH = AddRecord();
	return RH;
}

void UM2RecordSet::RemoveRecord(const FM2RecordHandle& RecordHandle)
{
	if (!RecordHandle.SetId.IsValid() || RecordHandle.SetId != SetId)
	{
		return;
	}
	
	if (!RecordIndexMap.Contains(RecordHandle.RecordId))
	{
		return;
	}

	int32 RecordIndex = *RecordIndexMap.Find(RecordHandle.RecordId);
	if (RecordIndex >= RecordIndexMap.Num())
	{
		// TODO(): Log error
		return;
	}

	for (TFunction<void(int32)> RemoveFn : RemoveRecordFns)
	{
		RemoveFn(RecordIndex);
	}

	RecordIndexMap.Remove(RecordHandle.RecordId);

	FGuid LastId = RecordHandles.Last().RecordId;
	RecordHandles.RemoveAtSwap(RecordIndex);
	
	if (RecordHandles.IsValidIndex(RecordIndex))
	{
		// a swap should have happened. Sanity check here to make sure the correct record was swapped.
		check(RecordHandles[RecordIndex].RecordId == LastId);
		RecordIndexMap[LastId] = RecordIndex;	
	}
}

FAnankeUntypedArrayView UM2RecordSet::GetFieldInternal(UScriptStruct* ComponentType)
{
	if (!GetFieldFns.Contains(ComponentType))
	{
		return FAnankeUntypedArrayView();
	}

	TFunction<FAnankeUntypedArrayView()> GetFieldFn = GetFieldFns.FindChecked(ComponentType);
	return GetFieldFn();
}