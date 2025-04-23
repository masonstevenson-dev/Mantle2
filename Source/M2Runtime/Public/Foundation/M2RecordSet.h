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
#include "M2Types.h"
#include "Containers/AnankeUntypedArrayView.h"

#include "M2RecordSet.generated.h"

#define M2_INITIALIZE_FIELD(FieldType, FieldName)																					\
	AddRecordFns.Add([this]() { return FieldName.AddDefaulted(); });																\
	RemoveRecordFns.Add([this](int32 RecordIndex) { FieldName.RemoveAtSwap(RecordIndex); });										\
	GetFieldFns.Add(FieldType::StaticStruct(), [this](){ return FAnankeUntypedArrayView(FieldName.GetData(), FieldName.Num()); });	\
	Archetype.Add(FieldType::StaticStruct());

#define M2_INITIALIZE_FIELD_WITH_SINGLETON(FieldType, FieldName)	\
	M2_INITIALIZE_FIELD(FieldType, FieldName);						\
	Get##FieldName();

// Note: M2_DECLARE_FIELD should be placed in a public: section.
#define M2_DECLARE_FIELD(FieldType, FieldName)					\
protected:														\
	UPROPERTY()													\
	TArray<FieldType> FieldName;								\
public:

// The same as DECLARE_FIELD, but also adds an accessor for singleton fields.
#define M2_DECLARE_FIELD_WITH_SINGLETON(FieldType, FieldName)	\
	M2_DECLARE_FIELD(FieldType, FieldName);						\
	UFUNCTION()													\
	FieldType* Get##FieldName()									\
	{															\
		if (!HasRecord(SingletonHandle))						\
		{														\
			SingletonHandle = AddRecord();						\
		}														\
		return GetField<FieldType>(SingletonHandle);			\
	}															

UCLASS()
class M2RUNTIME_API UM2RecordSet : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FGuid NewSetId);

	TArrayView<FM2RecordHandle> GetHandles()
	{
		return TArrayView<FM2RecordHandle>(RecordHandles);
	}

	template <typename ViewType>
	ViewType* GetField(const FM2RecordHandle& Handle)
	{
		if (!HasRecord(Handle))
		{
			return nullptr;
		}

		return &GetFieldArray<ViewType>()[RecordIndexMap.FindChecked(Handle.RecordId)];
	}
	
	template <typename ViewType>
	TArrayView<ViewType> GetFieldArray()
	{
		return GetFieldInternal(ViewType::StaticStruct()).template GetArrayView<ViewType>();
	}
	
	bool MatchArchetype(TArray<UScriptStruct*>& MatchFields, TArray<UScriptStruct*>& ExcludeFields);

	int32 Num() { return RecordIndexMap.Num(); }
	bool HasRecord(const FM2RecordHandle& Handle)
	{
		return Handle.SetId.IsValid() && Handle.SetId == SetId && Handle.RecordId.IsValid() && RecordIndexMap.Contains(Handle.RecordId);
	}

	FM2RecordHandle AddRecord();
	void RemoveRecord(FM2RecordHandle& RecordHandle);

protected:
	FAnankeUntypedArrayView GetFieldInternal(UScriptStruct* FieldType);

	UPROPERTY()
	FGuid SetId = FGuid();
	
	UPROPERTY()
	TArray<FM2RecordHandle> RecordHandles;
	
	UPROPERTY()
	TMap<FGuid, int32> RecordIndexMap;
	
	TArray<TFunction<void()>> AddRecordFns;
	TArray<TFunction<void(int32)>> RemoveRecordFns;
	TMap<UScriptStruct*, TFunction<FAnankeUntypedArrayView()>> GetFieldFns;
	TSet<UScriptStruct*> Archetype;
	
	// Hidden to force authors into using the macro accessor (i.e. GetMyField(), GetMyOtherField(), etc.)
	// This simplifies refactors: just replace all your M2_DECLARE_SINGLETON_FIELD calls with M2_DECLARE_FIELD and your
	// code will conveniently break in every place where you called GetMyField(), GetMyOtherField(), etc.
	UPROPERTY()
	FM2RecordHandle SingletonHandle;
};
