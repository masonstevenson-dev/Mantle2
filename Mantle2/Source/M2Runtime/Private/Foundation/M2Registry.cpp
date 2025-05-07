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

#include "Foundation/M2Registry.h"

#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"
#include "Testing/M2TestTables.h"

bool UM2Registry::HasRecord(const FM2RecordHandle& RecordHandle)
{
	return (
		RecordHandle.SetId.IsValid() &&
		SetsById.Contains(RecordHandle.SetId) &&
		SetsById.FindChecked(RecordHandle.SetId)->HasRecord(RecordHandle)
	);
}

void UM2Registry::RemoveRecord(FM2RecordHandle& RecordHandle)
{
	if (!RecordHandle.SetId.IsValid() || !RecordHandle.RecordId.IsValid())
	{
		return;
	}
	
	TObjectPtr<UM2RecordSet>* Result = SetsById.Find(RecordHandle.SetId);
	if (!Result)
	{
		// "stale" meaning at some point guid was generated for this set id, but now we can't find the matching recordset.
		M2_LOG(LogM2, Error, TEXT("RecordHandle has stale set id"));
		return;
	}

	Result->Get()->RemoveRecord(RecordHandle);
}

TArray<UM2RecordSet*> UM2Registry::GetAll(TArray<TSubclassOf<UM2RecordSet>>& RecordTypes)
{
	TArray<UM2RecordSet*> Result;

	for (TSubclassOf<UM2RecordSet> RecordType : RecordTypes)
	{
		if (!SetsByType.Contains(RecordType))
		{
			continue;
		}
		Result.Add(SetsByType.FindChecked(RecordType));
	}
	
	return Result;
}

TArray<UM2RecordSet*> UM2Registry::GetAll(TArray<UScriptStruct*>& Match, TArray<UScriptStruct*>& Exclude)
{
	TArray<UM2RecordSet*> Result;

	for (auto Iterator = SetsByType.CreateConstIterator(); Iterator; ++Iterator)
	{
		UM2RecordSet* TargetSet = Iterator.Value();
		
		if (TargetSet->MatchArchetype(Match, Exclude))
		{
			Result.Add(TargetSet);
		}
	}
	
	return Result;
}

void UM2Registry::ConstructRecordSets()
{
	TArray<FString> AllValidSets;
	TSet<FString> NewlyAddedSets;
	
	for (TObjectIterator<UClass> ClassIterator; ClassIterator; ++ClassIterator)
	{
		UClass* TargetClass = *ClassIterator;
			
		if (!TargetClass || !TargetClass->IsChildOf(UM2RecordSet::StaticClass()))
		{
			continue;
		}

		if (IsClassExcluded(TargetClass))
		{
			continue;
		}
		
		AllValidSets.Add(TargetClass->GetName());

		if (SetsByType.Contains(TargetClass))
		{
			continue;
		}

		NewlyAddedSets.Add(TargetClass->GetName());

		auto* NewRecordSet = NewObject<UM2RecordSet>(this, TargetClass);
		FGuid NewId = FGuid::NewGuid();
		NewRecordSet->Initialize(NewId);
		NewRecordSet->FinishInitialize();
		SetsById.Add(NewId, NewRecordSet);
		SetsByType.Add(TargetClass, NewRecordSet);
	}

	AllValidSets.Sort();

	M2_LOG_OBJECT(this, LogM2, Log, TEXT("DB initialized with Sets:"));
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("-------------------------------------------------------------------------------"));
	for (FString TypeName : AllValidSets)
	{
		M2_LOG_OBJECT(this, LogM2, Log, TEXT("%s%s)"), *TypeName, NewlyAddedSets.Contains(TypeName) ? TEXT(" (Newly Added)") : TEXT(""));
	}
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("-------------------------------------------------------------------------------"));
}

bool UM2Registry::IsClassExcluded(UClass* TargetClass)
{
	for (TSubclassOf<UM2RecordSet> ExcludeClass : GetExcludedSets())
	{
		if (TargetClass->IsChildOf(ExcludeClass.Get()))
		{
			return true;
		}
	}

	if (ExcludeTestSets() && TargetClass->IsChildOf(UM2TestRecordSet::StaticClass()))
	{
		return true;
	}

	return false;
}
