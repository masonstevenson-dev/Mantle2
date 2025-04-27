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

void UM2RecordSet::Initialize(FGuid NewSetId)
{
	// TODO(): Initialize should be called anytime this RecordSet gets deserialized.
	SetId = NewSetId;

	AddRecordFns.Empty();
	RemoveRecordFns.Empty();
	GetFieldFns.Empty();
	Archetype.Empty();
}

bool UM2RecordSet::MatchArchetype(TArray<UScriptStruct*>& MatchFields, TArray<UScriptStruct*>& ExcludeFields)
{
	for (UScriptStruct* FieldType : MatchFields)
	{
		if (!Archetype.Contains(FieldType))
		{
			return false;
		}
	}
	for (UScriptStruct* FieldType : ExcludeFields)
	{
		if (Archetype.Contains(FieldType))
		{
			return false;
		}
	}
	
	return (MatchFields.Num() + ExcludeFields.Num()) > 0;
}

FM2RecordHandle UM2RecordSet::AddRecord()
{
	RecordHandles.Add(FM2RecordHandle(SetId, FGuid::NewGuid()));
	RecordIndexMap.Add(RecordHandles.Last().RecordId, RecordHandles.Num() - 1);
	for (TFunction<void()> AddFunction : AddRecordFns)
	{
		AddFunction();
	}
	
	return RecordHandles.Last();
}

void UM2RecordSet::RemoveRecord(FM2RecordHandle& RecordHandle)
{
	if (RecordHandle == SingletonHandle)
	{
		return;
	}

	RemoveRecordInternal(RecordHandle);
}

void UM2RecordSet::ClearSingleton()
{
	RemoveRecordInternal(SingletonHandle);
	SingletonHandle.Clear();
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

void UM2RecordSet::RemoveRecordInternal(FM2RecordHandle& RecordHandle)
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
