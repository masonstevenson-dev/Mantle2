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

#include "Containers/Array.h"
#include "Containers/UnrealString.h"
#include "Foundation/M2Registry.h"
#include "Logging/LogVerbosity.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"
#include "Misc/AutomationTest.h"
#include "Testing/M2TestRegistry.h"
#include "Testing/M2TestTables.h"
#include "Testing/Macros/AnankeTestMacros.h"

#if WITH_EDITOR

class TestSuite
{
public:
	TestSuite(FAutomationTestBase* NewTestFramework): TestFramework(NewTestFramework)
	{
		// This constructor is run before each test.
		M2_LOG(LogM2Test, Log, TEXT("Setting up test suite."));

		M2_LOG(LogM2Test, Log, TEXT("Creating test world."));
		TestWorld = TStrongObjectPtr<UWorld>(UWorld::CreateWorld(EWorldType::Game, false));
		UWorld* World = TestWorld.Get();
		FWorldContext& WorldContext = GEngine->CreateNewWorldContext(EWorldType::Game);
		WorldContext.SetCurrentWorld(World);
		FURL URL;
		World->InitializeActorsForPlay(URL);
		World->BeginPlay();
		M2_LOG(LogM2Test, Log, TEXT("Test world initialized."));

		M2_LOG(LogM2Test, Log, TEXT("Creating test registry."));
		Registry = TStrongObjectPtr(NewObject<UM2TestRegistry>());
		M2_LOG(LogM2Test, Log, TEXT("Test registry created."));
	}

	~TestSuite()
	{
		// This destructor is run after each test.
		if (TestWorld.IsValid())
		{
			M2_LOG(LogM2Test, Log, TEXT("Destroying test world."));
			UWorld* WorldPtr = TestWorld.Get();
			GEngine->DestroyWorldContext(WorldPtr);
			WorldPtr->DestroyWorld(true);

			Registry.Reset();
			TestWorld.Reset();
			
			WorldPtr->MarkAsGarbage();
			CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);
			M2_LOG(LogM2Test, Log, TEXT("Test world destroyed."));	
		}
		else
		{
			Registry.Reset();
			TestWorld.Reset();
		}
	}

	void InitRegistry()
	{
		Registry->ConstructRecordSets();

		TestRH_1 = Registry->AddRecord<UM2TestSet_Door>();
		TestRH_2 = Registry->AddRecord<UM2TestSet_Door>();
		TestRH_3 = Registry->AddRecord<UM2TestSet_Door>();
		TestRH_Invalid = FM2RecordHandle();

		UM2TestSet_Door* DoorSet = Registry->GetRecordSet<UM2TestSet_Door>();

		DoorSet->Door[0].bIsOpen = false;
		DoorSet->Door[1].bIsOpen = true;
		DoorSet->Door[2].bIsOpen = false;

		DoorSet->Avatar[0].WorldPosition = FVector(1.0, 1.0, 1.0);
		DoorSet->Avatar[1].WorldPosition = FVector(2.0, 2.0, 2.0);
		DoorSet->Avatar[2].WorldPosition = FVector(3.0, 3.0, 3.0);

		TestRH_4 = Registry->AddRecord<UM2TestSet_Wall>();
		TestRH_5 = Registry->AddRecord<UM2TestSet_Wall>();
		TestRH_6 = Registry->AddRecord<UM2TestSet_Wall>();

		UM2TestSet_Wall* WallSet = Registry->GetRecordSet<UM2TestSet_Wall>();
		WallSet->Avatar[0].WorldPosition = FVector(4.0, 4.0, 4.0);
		WallSet->Avatar[1].WorldPosition = FVector(5.0, 5.0, 5.0);
		WallSet->Avatar[2].WorldPosition = FVector(6.0, 6.0, 6.0);

		WallSet->StaticEnvironment[0].Opacity = 0.1f;
		WallSet->StaticEnvironment[1].Opacity = 0.2f;
		WallSet->StaticEnvironment[2].Opacity = 0.3f;
	}

	void Test_SmokeTest()
	{
		ANANKE_TEST_TRUE(TestFramework, Registry.IsValid());
	}

	void Test_TestSetsExcludedFromStandardRegistry()
	{
		UM2Registry* NonTestRegistry = NewObject<UM2Registry>();
		NonTestRegistry->ConstructRecordSets();

		ANANKE_TEST_FALSE(TestFramework, NonTestRegistry->SetsByType.Contains(UM2TestSet_Player::StaticClass()));
		ANANKE_TEST_FALSE(TestFramework, NonTestRegistry->SetsByType.Contains(UM2TestSet_Door::StaticClass()));
		ANANKE_TEST_FALSE(TestFramework, NonTestRegistry->SetsByType.Contains(UM2TestSet_Excluded::StaticClass()));

		ANANKE_TEST_TRUE(TestFramework, NonTestRegistry->SetsById.Num() == NonTestRegistry->SetsByType.Num());
	}

	void Test_ConstructRecordSets()
	{
		Registry->ConstructRecordSets();

		ANANKE_TEST_TRUE(TestFramework, Registry->SetsByType.Contains(UM2TestSet_Player::StaticClass()));
		ANANKE_TEST_TRUE(TestFramework, Registry->SetsByType.Contains(UM2TestSet_Door::StaticClass()));
		ANANKE_TEST_FALSE(TestFramework, Registry->SetsByType.Contains(UM2TestSet_Excluded::StaticClass()));

		ANANKE_TEST_TRUE(TestFramework, Registry->SetsById.Num() == Registry->SetsByType.Num());

		for (auto SetIterator = Registry->SetsById.CreateConstIterator(); SetIterator; ++SetIterator)
		{
			UM2RecordSet* RecordSet = SetIterator.Value();
			ANANKE_TEST_NOT_NULL(TestFramework, RecordSet);
			ANANKE_TEST_TRUE(TestFramework, Registry->SetsByType.Contains(RecordSet->GetClass()));
		}
	}

	void Test_AddRecord()
	{
		InitRegistry();

		UM2TestSet_Door* DoorSet = Registry->GetRecordSet<UM2TestSet_Door>();
		ANANKE_TEST_NOT_NULL(TestFramework, DoorSet);

		ANANKE_TEST_EQUAL(TestFramework, DoorSet->GetHandles().Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Door.Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar.Num(), 3);

		FM2RecordHandle NewRecordHandle = Registry->AddRecord<UM2TestSet_Door>();
		Registry->GetField<FM2TestField_Door>(NewRecordHandle)->bIsOpen = true;
		Registry->GetField<FM2TestField_Avatar>(NewRecordHandle)->WorldPosition = FVector(42.0, 42.0, 42.0);

		ANANKE_TEST_TRUE(TestFramework, NewRecordHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, NewRecordHandle.RecordId.IsValid());
		if (ANANKE_TEST_TRUE(TestFramework, Registry->SetsById.Contains(NewRecordHandle.SetId)))
		{
			ANANKE_TEST_TRUE(TestFramework, Registry->SetsById.FindChecked(NewRecordHandle.SetId).Get() == DoorSet);
		}
		ANANKE_TEST_TRUE(TestFramework, NewRecordHandle.SetId == DoorSet->SetId);
		ANANKE_TEST_TRUE(TestFramework, DoorSet->RecordIndexMap.Contains(NewRecordHandle.RecordId));
		ANANKE_TEST_TRUE(TestFramework, Registry->HasRecord(NewRecordHandle));
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->GetHandles().Num(), 4);
		
		TArrayView<FM2TestField_Door> DoorFields = DoorSet->GetFieldArray<FM2TestField_Door>();
		TArrayView<FM2TestField_Avatar> AvatarFields = DoorSet->GetFieldArray<FM2TestField_Avatar>();
		
		if (!ANANKE_TEST_EQUAL(TestFramework, DoorFields.Num(), 4))
		{
			return;
		}
		if (!ANANKE_TEST_EQUAL(TestFramework, AvatarFields.Num(), 4))
		{
			return;
		}

		ANANKE_TEST_TRUE(TestFramework, DoorFields[0].bIsOpen == false);
		ANANKE_TEST_TRUE(TestFramework, DoorFields[1].bIsOpen == true);
		ANANKE_TEST_TRUE(TestFramework, DoorFields[2].bIsOpen == false);
		ANANKE_TEST_TRUE(TestFramework, DoorFields[3].bIsOpen == true);

		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[0].WorldPosition, FVector(1.0, 1.0, 1.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[1].WorldPosition, FVector(2.0, 2.0, 2.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[2].WorldPosition, FVector(3.0, 3.0, 3.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[3].WorldPosition, FVector(42.0, 42.0, 42.0));
	}

	void Test_RemoveRecord()
	{
		InitRegistry();

		UM2TestSet_Door* DoorSet = Registry->GetRecordSet<UM2TestSet_Door>();
		ANANKE_TEST_NOT_NULL(TestFramework, DoorSet);

		ANANKE_TEST_EQUAL(TestFramework, DoorSet->GetHandles().Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Door.Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar.Num(), 3);

		Registry->RemoveRecord(DoorSet->GetHandles()[1]);

		TArrayView<FM2TestField_Door> DoorFields = DoorSet->GetFieldArray<FM2TestField_Door>();
		TArrayView<FM2TestField_Avatar> AvatarFields = DoorSet->GetFieldArray<FM2TestField_Avatar>();
		
		if (!ANANKE_TEST_EQUAL(TestFramework, DoorFields.Num(), 2))
		{
			return;
		}
		if (!ANANKE_TEST_EQUAL(TestFramework, AvatarFields.Num(), 2))
		{
			return;
		}

		ANANKE_TEST_TRUE(TestFramework, DoorFields[0].bIsOpen == false);
		ANANKE_TEST_TRUE(TestFramework, DoorFields[1].bIsOpen == false);

		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[0].WorldPosition, FVector(1.0, 1.0, 1.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[1].WorldPosition, FVector(3.0, 3.0, 3.0));

		FM2RecordHandle NewRecordHandle = Registry->AddRecord<UM2TestSet_Door>();
		Registry->GetField<FM2TestField_Door>(NewRecordHandle)->bIsOpen = true;
		Registry->GetField<FM2TestField_Avatar>(NewRecordHandle)->WorldPosition = FVector(42.0, 42.0, 42.0);

		DoorFields = DoorSet->GetFieldArray<FM2TestField_Door>();
		AvatarFields = DoorSet->GetFieldArray<FM2TestField_Avatar>();
		
		if (!ANANKE_TEST_TRUE(TestFramework, DoorFields.Num() == 3))
		{
			return;
		}
		if (!ANANKE_TEST_TRUE(TestFramework, AvatarFields.Num() == 3))
		{
			return;
		}

		ANANKE_TEST_EQUAL(TestFramework, DoorFields[0].bIsOpen, false);
		ANANKE_TEST_EQUAL(TestFramework, DoorFields[1].bIsOpen, false);
		ANANKE_TEST_EQUAL(TestFramework, DoorFields[2].bIsOpen, true);

		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[0].WorldPosition, FVector(1.0, 1.0, 1.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[1].WorldPosition, FVector(3.0, 3.0, 3.0));
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields[2].WorldPosition, FVector(42.0, 42.0, 42.0));

		for (FM2RecordHandle RecordHandle : DoorSet->GetHandles())
		{
			Registry->RemoveRecord(RecordHandle);
		}

		DoorFields = DoorSet->GetFieldArray<FM2TestField_Door>();
		AvatarFields = DoorSet->GetFieldArray<FM2TestField_Avatar>();

		ANANKE_TEST_EQUAL(TestFramework, DoorFields.Num(), 0);
		ANANKE_TEST_EQUAL(TestFramework, AvatarFields.Num(), 0);
	}

	void Test_GetField()
	{
		InitRegistry();

		FM2TestField_Avatar* Field1 = Registry->GetField<FM2TestField_Avatar>(TestRH_1);
		FM2TestField_Avatar* Field2 = Registry->GetField<FM2TestField_Avatar>(TestRH_2);
		FM2TestField_Avatar* Field3 = Registry->GetField<FM2TestField_Avatar>(TestRH_3);
		FM2TestField_Avatar* InvalidField = Registry->GetField<FM2TestField_Avatar>(TestRH_Invalid);

		ANANKE_TEST_NOT_NULL(TestFramework, Field1);
		ANANKE_TEST_NOT_NULL(TestFramework, Field2);
		ANANKE_TEST_NOT_NULL(TestFramework, Field3);
		ANANKE_TEST_TRUE(TestFramework, InvalidField == nullptr);

		Field1->WorldPosition += FVector(1.0, 1.0, 1.0);
		Field2->WorldPosition += FVector(1.0, 1.0, 1.0);
		Field3->WorldPosition += FVector(1.0, 1.0, 1.0);

		UM2TestSet_Door* DoorSet = Registry->GetRecordSet<UM2TestSet_Door>();
		ANANKE_TEST_NOT_NULL(TestFramework, DoorSet);

		ANANKE_TEST_EQUAL(TestFramework, DoorSet->GetHandles().Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Door.Num(), 3);
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar.Num(), 3);

		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar[0].WorldPosition, FVector(2.0, 2.0, 2.0));
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar[1].WorldPosition, FVector(3.0, 3.0, 3.0));
		ANANKE_TEST_EQUAL(TestFramework, DoorSet->Avatar[2].WorldPosition, FVector(4.0, 4.0, 4.0));
	}

	void Test_GetSingletonField()
	{
		InitRegistry();

		UM2TestSet_Player* PlayerSet = Registry->GetRecordSet<UM2TestSet_Player>();
		ANANKE_TEST_NOT_NULL(TestFramework, PlayerSet);

		// The singleton field is automatically created during the INITIALIZE_FIELD_WITH_SINGLETON call.
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);

		// Calling GetAvatar repeatedly should not change anything
		FM2TestField_Avatar* AvatarField = PlayerSet->GetAvatar();
		ANANKE_TEST_NOT_NULL(TestFramework, AvatarField);
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);
		AvatarField = nullptr;
		AvatarField = PlayerSet->GetAvatar();
		ANANKE_TEST_NOT_NULL(TestFramework, AvatarField);
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);

		// Getting the field by handle should not change anything.
		AvatarField = nullptr;
		AvatarField = Registry->GetField<FM2TestField_Avatar>(PlayerSet->SingletonHandle);
		ANANKE_TEST_NOT_NULL(TestFramework, AvatarField);
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);

		// Getting the field by array should not change anything.
		TArrayView<FM2TestField_Avatar> AvatarArray = PlayerSet->GetFieldArray<FM2TestField_Avatar>();
		ANANKE_TEST_EQUAL(TestFramework, AvatarArray.Num(), 1);
	}

	void Test_ClearSingletonField()
	{
		InitRegistry();

		UM2TestSet_Player* PlayerSet = Registry->GetRecordSet<UM2TestSet_Player>();
		ANANKE_TEST_NOT_NULL(TestFramework, PlayerSet);

		// The singleton field is automatically created during the INITIALIZE_FIELD_WITH_SINGLETON call.
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);
		
		// Singleton records cannot be removed by handle.
		PlayerSet->RemoveRecord(PlayerSet->SingletonHandle);
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);

		// Calling ClearSingleton should flush the singleton data.
		PlayerSet->ClearSingleton();
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid() == false);
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid() == false);
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 0);

		// Calling Get{some field name}() regenerates the singleton.
		PlayerSet->GetAvatar();
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.SetId.IsValid());
		ANANKE_TEST_TRUE(TestFramework, PlayerSet->SingletonHandle.RecordId.IsValid());
		ANANKE_TEST_EQUAL(TestFramework, PlayerSet->Avatar.Num(), 1);
	}

	void Test_ProcessArchetype()
	{
		InitRegistry();

		UM2TestSet_Player* PlayerSet = Registry->GetRecordSet<UM2TestSet_Player>();
		ANANKE_TEST_NOT_NULL(TestFramework, PlayerSet);

		PlayerSet->GetAvatar()->WorldPosition = FVector(42.0, 42.0, 42.0);

		TArray<UScriptStruct*> RequiredStructs = TArray<UScriptStruct*>({FM2TestField_Avatar::StaticStruct()});
		TArray<UScriptStruct*> ExcludedStructs = TArray<UScriptStruct*>({FM2TestField_StaticEnvironment::StaticStruct()});
		TArray<UM2RecordSet*> Result = Registry->GetAll(RequiredStructs, ExcludedStructs);

		ANANKE_TEST_EQUAL(TestFramework, Result.Num(), 2);
		
		TSet<FVector> ExpectedPositions = TSet<FVector>({
			FVector(1.0, 1.0, 1.0),
			FVector(2.0, 2.0, 2.0),
			FVector(3.0, 3.0, 3.0),
			FVector(42.0, 42.0, 42.0),
		});
		int32 ExpectedFieldsProcessed = ExpectedPositions.Num();
		int32 FieldsProcessed = 0;

		// Note that RecordSet order is not guaranteed.
		for (UM2RecordSet* RecordSet : Result)
		{
			ANANKE_TEST_NOT_NULL(TestFramework, RecordSet);

			for (FM2TestField_Avatar& AvatarField : RecordSet->GetFieldArray<FM2TestField_Avatar>())
			{
				ANANKE_TEST_TRUE(TestFramework, ExpectedPositions.Contains(AvatarField.WorldPosition));
				ExpectedPositions.Remove(AvatarField.WorldPosition);
				FieldsProcessed++;
			}
		}

		ANANKE_TEST_EQUAL(TestFramework, FieldsProcessed, ExpectedFieldsProcessed);
	}

	void Test_ProcessArchetypeWithTags()
	{
		InitRegistry();

		UM2TestSet_Player* PlayerSet = Registry->GetRecordSet<UM2TestSet_Player>();
		ANANKE_TEST_NOT_NULL(TestFramework, PlayerSet);

		PlayerSet->GetAvatar()->WorldPosition = FVector(42.0, 42.0, 42.0);

		TArray<UScriptStruct*> RequiredStructs = TArray<UScriptStruct*>({FM2TestField_Avatar::StaticStruct()});
		TArray<UScriptStruct*> ExcludedStructs = TArray<UScriptStruct*>({FMTestTag_StaticEnvironment::StaticStruct()});
		TArray<UM2RecordSet*> Result = Registry->GetAll(RequiredStructs, ExcludedStructs);

		ANANKE_TEST_EQUAL(TestFramework, Result.Num(), 2);
		
		TSet<FVector> ExpectedPositions = TSet<FVector>({
			FVector(1.0, 1.0, 1.0),
			FVector(2.0, 2.0, 2.0),
			FVector(3.0, 3.0, 3.0),
			FVector(42.0, 42.0, 42.0),
		});
		int32 ExpectedFieldsProcessed = ExpectedPositions.Num();
		int32 FieldsProcessed = 0;

		// Note that RecordSet order is not guaranteed.
		for (UM2RecordSet* RecordSet : Result)
		{
			ANANKE_TEST_NOT_NULL(TestFramework, RecordSet);

			for (FM2TestField_Avatar& AvatarField : RecordSet->GetFieldArray<FM2TestField_Avatar>())
			{
				ANANKE_TEST_TRUE(TestFramework, ExpectedPositions.Contains(AvatarField.WorldPosition));
				ExpectedPositions.Remove(AvatarField.WorldPosition);
				FieldsProcessed++;
			}
		}

		ANANKE_TEST_EQUAL(TestFramework, FieldsProcessed, ExpectedFieldsProcessed);
	}

	void Test_GetShared()
	{
		// TODO()
	}

	// IMPORTANT! Be sure to register your fn inside your AutomationTest class below!

private:
	FAutomationTestBase* TestFramework;
	
	// Test objects
	TStrongObjectPtr<UWorld> TestWorld;
	TStrongObjectPtr<UM2TestRegistry> Registry;

	FM2RecordHandle TestRH_1;
	FM2RecordHandle TestRH_2;
	FM2RecordHandle TestRH_3;
	FM2RecordHandle TestRH_4;
	FM2RecordHandle TestRH_5;
	FM2RecordHandle TestRH_6;
	FM2RecordHandle TestRH_Invalid;
};

#define REGISTER_TEST_SUITE_FN(TargetTestName) Tests.Add(TEXT(#TargetTestName), &TestSuite::TargetTestName)

class FRegistryTests: public FAutomationTestBase
{
public:
	typedef void (TestSuite::*TestFunction)();
	
	FRegistryTests(const FString& TestName): FAutomationTestBase(TestName, false)
	{
		REGISTER_TEST_SUITE_FN(Test_SmokeTest);
		REGISTER_TEST_SUITE_FN(Test_TestSetsExcludedFromStandardRegistry);
		REGISTER_TEST_SUITE_FN(Test_ConstructRecordSets);
		REGISTER_TEST_SUITE_FN(Test_AddRecord);
		REGISTER_TEST_SUITE_FN(Test_RemoveRecord);
		REGISTER_TEST_SUITE_FN(Test_GetField);
		REGISTER_TEST_SUITE_FN(Test_GetSingletonField);
		REGISTER_TEST_SUITE_FN(Test_ClearSingletonField);
		REGISTER_TEST_SUITE_FN(Test_ProcessArchetype);
		REGISTER_TEST_SUITE_FN(Test_ProcessArchetypeWithTags);
		REGISTER_TEST_SUITE_FN(Test_GetShared);
	}
	
	virtual uint32 GetTestFlags() const override
	{
		return EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter;
	}
	virtual bool IsStressTest() const { return false; }
	virtual uint32 GetRequiredDeviceNum() const override { return 1; }

protected:
	virtual FString GetBeautifiedTestName() const override
	{
		return "Mantle2.Runtime.RegistryTests";
	}
	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override
	{
		TArray<FString> TargetTestNames;
		Tests.GetKeys(TargetTestNames);
		for (const FString& TargetTestName : TargetTestNames)
		{
			OutBeautifiedNames.Add(TargetTestName);
			OutTestCommands.Add(TargetTestName);
		}
	}
	virtual bool RunTest(const FString& Parameters) override
	{
		TestFunction* CurrentTest = Tests.Find(Parameters);
		if (!CurrentTest || !*CurrentTest)
		{
			M2_LOG(LogM2Test, Error, TEXT("Cannot find test: %s"), *Parameters);
			return false;
		}

		TestSuite Suite(this);
		(Suite.**CurrentTest)(); // Run the current test from the test suite.

		return true;
	}

	TMap<FString, TestFunction> Tests;
};

namespace
{
	FRegistryTests FRegistryTestsInstance(TEXT("FRegistryTests"));
}

#endif //WITH_EDITOR