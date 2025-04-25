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
#include "M2RecordSet.h"

#include "M2Registry.generated.h"

class UM2Engine;

UCLASS()
class M2RUNTIME_API UM2Registry : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Checks if a RecordHandle points to a valid record.
	 * 
	 * @param RecordHandle - The handle to check.
	 * @return - True if the handle is valid.
	 */
	bool HasRecord(const FM2RecordHandle& RecordHandle);

	/**
	 * Adds a record of the target type to the registry.
	 * @tparam RecordType - The type of record to add.
	 * @return - Returns a valid RecordHandle if the record was added.
	 */
	template <typename RecordType>
	FM2RecordHandle AddRecord()
	{
		static_assert(std::is_base_of_v<UM2RecordSet, RecordType>);
		TObjectPtr<UM2RecordSet>* Result = SetsByType.Find(RecordType::StaticClass());
		return Result ? Result->Get()->AddRecord() : FM2RecordHandle();
	}

	/**
	 * Removes a record from the registry, if it exists.
	 * 
	 * @param RecordHandle - The handle for the record that should be removed.
	 */
	void RemoveRecord(FM2RecordHandle& RecordHandle);

	/**
	 *	Gets a compacted array of fields of a particular type for each record of a particular type.
	 * 
	 * @tparam RecordType - The type of record to fetch.
	 * @tparam FieldType - The field type to fetch.
	 * @return An ArrayView<FieldType> containing the matching record fields, or an empty ArrayView if no match was found.
	 */
	template <typename RecordType, typename FieldType>
	TArrayView<FieldType> GetField()
	{
		static_assert(std::is_base_of_v<UM2RecordSet, RecordType>);
		TObjectPtr<UM2RecordSet>* Result = SetsByType.Find(RecordType::StaticClass());
		return Result ? Result->Get()->GetField<FieldType>() : TArrayView<FieldType>();
	}

	/**
	 *	Fetches a field for an individual record.
	 * 
	 * @tparam RecordType - The RecordSet type of the target record.
	 * @tparam FieldType - The type of the target field.
	 * @param Handle - The RecordHandle, which is a unique id for a target record.
	 * @return Returns a pointer to the matching field if it exists, otherwise nullptr.
	 */
	template <typename RecordType, typename FieldType>
	FieldType* GetField(FM2RecordHandle& Handle)
	{
		static_assert(std::is_base_of_v<UM2RecordSet, RecordType>);
		TObjectPtr<UM2RecordSet>* Result = SetsByType.Find(RecordType::StaticClass());
		return Result ? Result->Get()->GetField<FieldType>(Handle) : nullptr;
	}
	
	/**
	 *	Fetches a RecordSet of the target type, if one exists.
	 * 
	 * @tparam RecordType - The type of RecordSet to fetch.
	 * @return Returns a pointer to the matching RecordSet if it exists, otherwise nullptr.
	 */
	template <typename RecordType>
	RecordType* GetRecordSet()
	{
		static_assert(std::is_base_of_v<UM2RecordSet, RecordType>);
		TObjectPtr<UM2RecordSet>* Result = SetsByType.Find(RecordType::StaticClass());
		return Result ? Cast<RecordType>(Result->Get()) : nullptr;
	}
	
	/**
	 * Fetches a list of RecordSets matching the target types, if they exist.
	 * 
	 * @param RecordTypes - A list of RecordSet types to search for.
	 * @return Returns an array containing the matching RecordSet types, if they exist.
	 */
	TArray<UM2RecordSet*> GetAll(TArray<TSubclassOf<UM2RecordSet>>& RecordTypes);

	/**
	 * Fetches a list of RecordSets matching a particular composition.
	 *
	 * @param MatchColumns - A list of column types that every RecordSet in the result must contain.
	 * @param ExcludeColumns - A list of column types that must not be present in any of RecordSets in the result.
	 *
	 * @return Returns an array containing any RecordSets found that match the target composition.
	 */
	TArray<UM2RecordSet*> GetAll(TArray<UScriptStruct*>& MatchColumns, TArray<UScriptStruct*>& ExcludeColumns);

	
	/**
	 * Gets a shared object of the target type and casts it to a base type. Useful if you know the base type at compile
	 * time but not the target type.
	 * 
	 * @tparam BaseType - The type to cast to.
	 * @param TargetType - The type to get.
	 * @param bCacheEnabled - If true, the shared object will be cached for later reuse. Otherwise, any existing shared object will be released.
	 * @return Returns the shared object.
	 */
	template <typename BaseType>
	BaseType* GetShared(UClass* TargetType, bool bCacheEnabled = true)
	{
		if (!TargetType)
		{
			return nullptr;
		}
		
		BaseType* TheObject = nullptr;
		bool bNewlyConstructed = false;
		
		if (!SharedObjects.Contains(TargetType) || !IsValid(SharedObjects.FindChecked(TargetType)))
		{
			TheObject = NewObject<BaseType>(this, TargetType);
			bNewlyConstructed = true;
		}
		else
		{
			TheObject = Cast<BaseType>(SharedObjects.FindChecked(TargetType).Get());
		}

		if (!TheObject)
		{
			return nullptr;
		}

		if (!bCacheEnabled)
		{
			SharedObjects.Remove(TargetType);
		}
		else if (bNewlyConstructed)
		{
			SharedObjects.Add(TargetType, TheObject);
		}

		return TheObject;
	}

	/**
	 *	Same as GetShared<Base>(Target), but assumes you know the target type at compile time.
	 * 
	 * @tparam TargetType - The type of shared object to get.
	 * @param bCacheEnabled - If true, the shared object will be cached for later reuse. Otherwise, any existing shared object will be released.
	 * @return Returns the shared object.
	 */
	template <typename TargetType>
	TargetType* GetShared(bool bCacheEnabled = true)
	{
		return GetShared<TargetType>(TargetType::StaticClass(), bCacheEnabled);
	}

protected:
	friend UM2Engine;
	
	// By default, the DB will be initialized with 1 copy of each class extending UM2RecordSet found in your codebase.
	// to exclude a RecordSet from being constructed, override this fn.
	virtual TArray<TSubclassOf<UM2RecordSet>> GetExcludedSets() { return TArray<TSubclassOf<UM2RecordSet>>(); }
	
	virtual bool ExcludeTestSets() { return true; }

	// TODO()
	// Call this once when the DB is created and then again when the DB is deserialized. The deserialization part is
	// important because of the following:
	//
	//   1) User loads level for the first time
	//   2) DB gets created with X RecordSets
	//   3) User saves game (db is serialized)
	//   4) Update is pushed to game containing a new RecordSet type and new avatar actors in the world that register records in that RecordSet
	//   5) User loads the game
	//   6) These new entities try to register with the DB, but the DB doesn't know about the new RecordSet.
	//   
	void ConstructRecordSets();

	UPROPERTY()
	TMap<FGuid, TObjectPtr<UM2RecordSet>> SetsById;

	UPROPERTY()
	TMap<TSubclassOf<UM2RecordSet>, TObjectPtr<UM2RecordSet>> SetsByType;

	UPROPERTY()
	TMap<UClass*, TObjectPtr<UObject>> SharedObjects;

private:
	bool IsClassExcluded(UClass* TargetClass);
};
