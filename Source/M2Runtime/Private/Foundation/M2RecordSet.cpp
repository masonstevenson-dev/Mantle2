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
