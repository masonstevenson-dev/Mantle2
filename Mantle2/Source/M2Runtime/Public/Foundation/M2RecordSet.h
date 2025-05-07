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

class TestSuite;

#define I_M2_INITIALIZE_FIELD(TypeOrAlias, FieldName)																					\
	AddRecordFns.Add([this]() { return FieldName.AddDefaulted(); });																	\
	RemoveRecordFns.Add([this](int32 RecordIndex) { FieldName.RemoveAtSwap(RecordIndex); });											\
	GetFieldFns.Add(TypeOrAlias::StaticStruct(), [this](){ return FAnankeUntypedArrayView(FieldName.GetData(), FieldName.Num()); });	\
	Archetype.Add(TypeOrAlias::StaticStruct());

// Note: We include Check##FieldType as a simple way of forcing a compilation error if:
//			1) The field "FieldName" was declared with a different type other than FieldType.
//			2) Any other field was initialized with the same field type. If you want to initialize multiple fields with
//			   the same type, use M2_INITIALIZE_FIELD_WITH_ALIAS instead.
#define M2_INITIALIZE_FIELD(FieldType, FieldName)		\
	FieldType* Check##FieldType = FieldName.GetData();	\
	I_M2_INITIALIZE_FIELD(FieldType, FieldName)

#define M2_INITIALIZE_FIELD_WITH_ALIAS(FieldType, TypeAlias, FieldName)	\
	FieldType* Check##FieldType##TypeAlias = FieldName.GetData();		\
	I_M2_INITIALIZE_FIELD(TypeAlias, FieldName)

#define M2_INITIALIZE_TAG(TagType) \
	Archetype.Add(TagType::StaticStruct());

// Note: M2_DECLARE_FIELD should be placed in a public: section.
#define M2_DECLARE_FIELD(FieldType, FieldName)					\
protected:														\
	UPROPERTY()													\
	TArray<FieldType> FieldName;								\
public:


UCLASS()
class M2RUNTIME_API UM2RecordSet : public UObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FGuid NewSetId);
	virtual void FinishInitialize();

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

		TArrayView<ViewType> FieldArray = GetFieldArray<ViewType>();
		if (!FieldArray.GetData())
		{
			return nullptr;
		}
		
		return &FieldArray[RecordIndexMap.FindChecked(Handle.RecordId)];
	}

	template <typename ViewType, typename TypeAlias>
	ViewType* GetField(const FM2RecordHandle& Handle)
	{
		if (!HasRecord(Handle))
		{
			return nullptr;
		}

		TArrayView<ViewType> FieldArray = GetFieldArray<ViewType, TypeAlias>();
		if (!FieldArray.GetData())
		{
			return nullptr;
		}
		
		return &FieldArray[RecordIndexMap.FindChecked(Handle.RecordId)];
	}

	template <typename ViewType>
	ViewType* GetSingletonField()
	{
		RefreshSingleton();
		return GetField<ViewType>(SingletonHandle);
	}

	template <typename ViewType, typename TypeAlias>
	ViewType* GetSingletonField()
	{
		RefreshSingleton();
		return GetField<ViewType, TypeAlias>(SingletonHandle);
	}

	FM2RecordHandle GetSingletonHandle()
	{
		RefreshSingleton();
		return SingletonHandle;
	}
	
	template <typename ViewType>
	TArrayView<ViewType> GetFieldArray()
	{
		return GetFieldInternal(ViewType::StaticStruct()).template GetArrayView<ViewType>();
	}

	template <typename ViewType, typename TypeAlias>
	TArrayView<ViewType> GetFieldArray()
	{
		return GetFieldInternal(TypeAlias::StaticStruct()).template GetArrayView<ViewType>();
	}
	
	bool MatchArchetype(TArray<UScriptStruct*>& Match, TArray<UScriptStruct*>& Exclude);

	int32 Num()
	{
		return RecordIndexMap.Num();
	}
	
	bool HasRecord(const FM2RecordHandle& Handle)
	{
		return Handle.SetId.IsValid() && Handle.SetId == SetId && Handle.RecordId.IsValid() && RecordIndexMap.Contains(Handle.RecordId);
	}

	template <typename ViewType>
	bool HasField()
	{
		return GetFieldFns.Contains(ViewType::StaticStruct());
	}
	bool HasField(UScriptStruct* FieldType)
	{
		return GetFieldFns.Contains(FieldType);
	}

	FM2RecordHandle AddRecord();
	void RemoveRecord(FM2RecordHandle& RecordHandle);

	// Note: Calling Get{some field name}() will automatically recreate the singleton.
	void ClearSingleton();

protected:
	friend TestSuite;
	
	FAnankeUntypedArrayView GetFieldInternal(UScriptStruct* FieldType);
	void RemoveRecordInternal(FM2RecordHandle& RecordHandle);

	void RefreshSingleton();

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

	UPROPERTY()
	bool bInitWithSingleton = false;
};
