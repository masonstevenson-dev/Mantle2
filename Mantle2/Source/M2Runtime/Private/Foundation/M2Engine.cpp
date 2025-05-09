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

#include "Foundation/M2Engine.h"

#include "Foundation/M2Operation.h"
#include "Macros/AnankeCoreLoggingMacros.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"

FM2EngineLoop::FM2EngineLoop()
{
	bCanEverTick = true;
	bStartWithTickEnabled = false;
}

void FM2EngineLoop::ExecuteTick(
	float DeltaTime,
	ELevelTick TickType,
	ENamedThreads::Type CurrentThread,
	const FGraphEventRef& MyCompletionGraphEvent
)
{
	if (TickType == ELevelTick::LEVELTICK_ViewportsOnly || TickType == ELevelTick::LEVELTICK_PauseTick)
	{
		return;
	}

	OperationContext.DeltaTime = DeltaTime;

	for (FM2OperationGroup& OperationGroup : Options.OperationGroups)
	{
		// In the future, we may support multithreading by running operations in the same OperationGroup concurrently.
		// Something like:
		// if (bRunMultithreaded)
		// {
		//   for(Operation : Group.Operations) {
		//     Operation.RunConcurrent();
		//   }
		//   Wait();
		// }

		for (TWeakObjectPtr<UM2Operation> Operation : OperationGroup.Operations)
		{
			if (!Operation.IsValid())
			{
				M2_LOG(LogM2, Error, TEXT("Operation is invalid."));
			}
			Operation->Run(OperationContext);
		}
	}
}

UM2Engine::UM2Engine(const FObjectInitializer& ObjectInitializer)
{
	PrePhysicsLoop.TickGroup = ETickingGroup::TG_PrePhysics;
	StartPhysicsLoop.TickGroup = ETickingGroup::TG_StartPhysics;
	DuringPhysicsLoop.TickGroup = ETickingGroup::TG_DuringPhysics;
	EndPhysicsLoop.TickGroup = ETickingGroup::TG_EndPhysics;
	PostPhysicsLoop.TickGroup = ETickingGroup::TG_PostPhysics;
	FrameEndLoop.TickGroup = ETickingGroup::TG_LastDemotable;

	Registry = CreateDefaultSubobject<UM2Registry>(TEXT("M2Registry"));
}

void UM2Engine::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	M2_LOG(LogM2, Log, TEXT("Initializing M2Engine"));
	ResetCounters();

	if (!Registry)
	{
		M2_LOG(LogM2, Fatal, TEXT("Registry was not created."));
	}
	
	Registry->ConstructRecordSets();
}

void UM2Engine::Deinitialize()
{
	Super::Deinitialize();

	ResetCounters();
}

void UM2Engine::ConfigureEngineLoop(const ETickingGroup TickGroup, const FM2EngineLoopOptions& Options)
{
	M2_LOG(LogM2, Log, TEXT("Configuring engine loop for TickGroup: %s."), *UEnum::GetValueAsName(TickGroup).ToString());
	
	if (EngineState == EM2EngineState::Started)
	{
		M2_LOG(LogM2, Warning, TEXT("Unable to configure engine loop: Engine is already started."));
		return;
	}

	switch (TickGroup)
	{
	case TG_PrePhysics:
		PrePhysicsLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	case TG_StartPhysics:
		StartPhysicsLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	case TG_DuringPhysics:
		DuringPhysicsLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	case TG_EndPhysics:
		EndPhysicsLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	case TG_PostPhysics:
		PostPhysicsLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	case TG_LastDemotable:
		FrameEndLoop.Options.OperationGroups.Append(Options.OperationGroups);
		break;
	default:
		M2_LOG(LogM2, Error, TEXT("Unknown TickGroup: %s"), *UEnum::GetDisplayValueAsText(TickGroup).ToString());
		break;
	}
}

void UM2Engine::FinishConfiguration()
{
	for (TObjectPtr<UM2Operation> Operation : Operations)
	{
		Operation->Initialize(Registry);
		M2_LOG(LogM2, Log, TEXT("Operation %s initialized."), *Operation->GetClass()->GetName());
	}
	
	EngineState = EM2EngineState::Stopped;	
}

void UM2Engine::Start(UWorld& World)
{
	if (!World.IsGameWorld())
	{
		M2_LOG(LogM2, Error, TEXT("M2Engine does not support running outside of game worlds."));
		return;
	}

	M2_LOG_OBJECT(this, LogM2, Log, TEXT("Starting M2Engine."));
	ActivateEngineLoop(PrePhysicsLoop, World);
	ActivateEngineLoop(StartPhysicsLoop, World);
	ActivateEngineLoop(DuringPhysicsLoop, World);
	ActivateEngineLoop(EndPhysicsLoop, World);
	ActivateEngineLoop(PostPhysicsLoop, World);
	ActivateEngineLoop(FrameEndLoop, World);

	EngineState = EM2EngineState::Started;
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("M2Engine started."));
}

void UM2Engine::Stop()
{
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("Stopping M2Engine."));
	DeactivateEngineLoop(PrePhysicsLoop);
	DeactivateEngineLoop(StartPhysicsLoop);
	DeactivateEngineLoop(DuringPhysicsLoop);
	DeactivateEngineLoop(EndPhysicsLoop);
	DeactivateEngineLoop(PostPhysicsLoop);
	DeactivateEngineLoop(FrameEndLoop);

	EngineState = EM2EngineState::Stopped;
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("M2Engine stopped."));
}

void UM2Engine::ResetCounters()
{
	SET_DWORD_STAT(STAT_M2_TempararyEntitiesAdded, 0);
	SET_DWORD_STAT(STAT_M2_TempararyEntitiesRemoved, 0);
}

void UM2Engine::ActivateEngineLoop(FM2EngineLoop& TickFunction, UWorld& World)
{
	TickFunction.OperationContext.Registry = Registry.Get();
	TickFunction.OperationContext.World = &World;
	TickFunction.RegisterTickFunction(World.PersistentLevel);
	TickFunction.SetTickFunctionEnable(true);
}

void UM2Engine::DeactivateEngineLoop(FM2EngineLoop& TickFunction)
{
	TickFunction.SetTickFunctionEnable(false);
	TickFunction.OperationContext.Registry = nullptr;
	TickFunction.OperationContext.World = nullptr;
	TickFunction.UnRegisterTickFunction();
}