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
#include "M2Operation.h"
#include "Async/TaskGraphFwd.h"
#include "Async/TaskGraphInterfaces.h"
#include "Containers/Array.h"
#include "Engine/EngineBaseTypes.h"
#include "Engine/World.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "UObject/Class.h"
#include "UObject/Object.h"
#include "UObject/ObjectPtr.h"
#include "UObject/UObjectGlobals.h"

#include "M2Engine.generated.h"

// Lightweight container for a collection of operations. We may use this to specify operations that can run concurrently
// in the future.
USTRUCT()
struct M2RUNTIME_API FM2OperationGroup
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<TWeakObjectPtr<UM2Operation>> Operations;
};

USTRUCT()
struct M2RUNTIME_API FM2EngineLoopOptions
{
	GENERATED_BODY()

public:
	UPROPERTY()
	TArray<FM2OperationGroup> OperationGroups;
};

USTRUCT()
struct FM2EngineLoop : public FTickFunction
{
	GENERATED_BODY()

public:
	FM2EngineLoop();

	UPROPERTY()
	FM2EngineLoopOptions Options;

	UPROPERTY()
	FM2OperationContext OperationContext;

protected:
	virtual void ExecuteTick(
		float DeltaTime,
		ELevelTick TickType,
		ENamedThreads::Type CurrentThread,
		const FGraphEventRef& MyCompletionGraphEvent
	) override;
};

template<>
struct TStructOpsTypeTraits<FM2EngineLoop> : public TStructOpsTypeTraitsBase2<FM2EngineLoop>
{
	enum
	{
		WithCopy = false // It is unsafe to copy FTickFunctions
	};
};

UENUM()
enum class EM2EngineState
{
	Initialize,
	Started,
	Stopped
};

UCLASS()
class M2RUNTIME_API UM2Engine : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UM2Engine(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return true; }

	// Call this from your game instance to configure which operations run. If you inherit from M2GameInstance
	// this is all that is needed to get up and running.
	void ConfigureEngineLoop(const ETickingGroup TickGroup, const FM2EngineLoopOptions& Options);
	void FinishConfiguration();
	
	void Start(UWorld& World);
	void Stop();
	bool IsStarted() { return EngineState == EM2EngineState::Started; }

	TObjectPtr<UM2Registry> GetRegistry()
	{
		return Registry;
	}

	template<typename OperationType>
	TWeakObjectPtr<OperationType> NewOperation()
	{
		if (EngineState != EM2EngineState::Initialize)
		{
			return nullptr;
		}
		
		OperationType* Operation = NewObject<OperationType>(this);
		Operations.Add(Operation);
		return TWeakObjectPtr<OperationType>(Operation);
	}

protected:
	void ResetCounters();
	
	void ActivateEngineLoop(FM2EngineLoop& TickFunction, UWorld& World);
	void DeactivateEngineLoop(FM2EngineLoop& TickFunction);
	
	UPROPERTY()
	TObjectPtr<UM2Registry> Registry = nullptr;

	UPROPERTY()
	TArray<TObjectPtr<UM2Operation>> Operations;

	FM2EngineLoop PrePhysicsLoop;
	FM2EngineLoop StartPhysicsLoop;
	FM2EngineLoop DuringPhysicsLoop;
	FM2EngineLoop EndPhysicsLoop;
	FM2EngineLoop PostPhysicsLoop;
	FM2EngineLoop FrameEndLoop;

	EM2EngineState EngineState = EM2EngineState::Initialize;
};
