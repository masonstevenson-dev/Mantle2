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
class TestSuite;

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
	 * 
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
	 * Gets a handle for the singleton record associated with a particular record type, or creates one and then returns
	 * the newly created handle if no singleton currently exists.
	 * 
	 * @tparam RecordType - The type of record to add.
	 * @return - Returns a valid RecordHandle.
	 */
	template <typename RecordType>
	FM2RecordHandle GetSingletonHandle()
	{
		static_assert(std::is_base_of_v<UM2RecordSet, RecordType>);
		TObjectPtr<UM2RecordSet>* Result = SetsByType.Find(RecordType::StaticClass());
		return Result ? Result->Get()->GetSingletonHandle() : FM2RecordHandle();
	}

	/**
	 * Removes a record from the registry, if it exists.
	 * 
	 * @param RecordHandle - The handle for the record that should be removed.
	 */
	void RemoveRecord(FM2RecordHandle& RecordHandle);

	/**
	 *	Fetches a field for an individual record.
	 * 
	 * @tparam FieldType - The type of the target field.
	 * @param Handle - The RecordHandle, which is a unique id for a target record.
	 * @return Returns a pointer to the matching field if it exists, otherwise nullptr.
	 */
	template <typename FieldType>
	FieldType* GetField(const FM2RecordHandle& Handle)
	{
		TObjectPtr<UM2RecordSet>* Result = SetsById.Find(Handle.SetId);
		return Result ? Result->Get()->GetField<FieldType>(Handle) : nullptr;
	}

	/**
	 * Fetches an aliased field for an individual record.
	 * 
	 * @tparam FieldType - The type of field to fetch.
	 * @tparam TypeAlias - The alias for the field to fetch.
	 * @param Handle - The RecordHandle, which is a unique id for a target record.
	 * @return Returns a pointer to the matching field if it exists, otherwise nullptr.
	 */
	template <typename FieldType, typename TypeAlias>
	FieldType* GetField(const FM2RecordHandle& Handle)
	{
		TObjectPtr<UM2RecordSet>* Result = SetsById.Find(Handle.SetId);
		return Result ? Result->Get()->GetField<FieldType, TypeAlias>(Handle) : nullptr;
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
	 * @param Match - A list of field/tag types that every RecordSet in the result must contain.
	 * @param Exclude - A list of field/tag types that must not be present in any of RecordSets in the result.
	 *
	 * @return Returns an array containing any RecordSets found that match the target composition.
	 */
	TArray<UM2RecordSet*> GetAll(TArray<UScriptStruct*>& Match, TArray<UScriptStruct*>& Exclude);

	
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
	friend TestSuite;
	
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
