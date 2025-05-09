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

#include "Actor/M2Actor.h"

#include "Components/M2AvatarComponent.h"
#include "Fields/Avatar.h"
#include "Foundation/M2Engine.h"
#include "FunctionLibraries/M2EngineLibrary.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"

AM2Actor::AM2Actor(const FObjectInitializer& Initializer): Super(Initializer)
{
	StaticMeshComponent = CreateDefaultSubobject<UStaticMeshComponent>("Mesh");
	RootComponent = StaticMeshComponent;
}

void AM2Actor::PreInitializeComponents()
{
	Super::PreInitializeComponents();

	if (!bIsInitialized && !bDeferInitialization)
	{
		if (!InitializeMantleActor())
		{
			M2_LOG_OBJECT(this, LogM2, Warning, TEXT("Failed to initialize mantle actor during PreInitializeComponents. Will try again on BeginPlay."));
		}
	}
}

void AM2Actor::BeginPlay()
{
	Super::BeginPlay();

	if (!bIsInitialized && !bDeferInitialization)
	{
		if (!InitializeMantleActor())
		{
			M2_LOG_OBJECT(this, LogM2, Error, TEXT("Failed to initialize mantle actor on BeginPlay."));
		}
	}
}

bool AM2Actor::InitializeMantleActor()
{
	if (bIsInitialized)
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("This actor is already initialized."));
		return bIsInitialized;
	}

	Registry = UM2EngineLibrary::GetRegistry(GetGameInstance());

	if (!Registry)
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("Expected valid registry."));
		return bIsInitialized;
	}
	
	UWorld* World = GetWorld();
	if (!World || !World->IsGameWorld())
	{
		return bIsInitialized;
	}

	FM2RecordHandle RecordHandle = CreateRecord(*Registry);
	if (!RecordHandle.IsValid(Registry))
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("CreateRecord returned an invalid handle. Skipping initialization"));
		return bIsInitialized;
	}
	else if (auto* AvatarField = Registry->GetField<FLD_AvatarActor>(RecordHandle))
	{
		AvatarField->AvatarActor = this;
		AvatarField->AvatarClass = GetClass();
	}
	else
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("Expected record to have Avatar field. Skipping initialization"));
		Registry->RemoveRecord(RecordHandle);
		return bIsInitialized;
	}
	
	AvatarComponent = NewObject<UM2AvatarComponent>(this, TEXT("AvatarComponent"));
	AvatarComponent->RegisterComponent();
	AddInstanceComponent(AvatarComponent);

	AvatarComponent->InitializeAvatar(Registry, RecordHandle, bRemoveEntityOnDestruction);
			
	bIsInitialized = true;
	return bIsInitialized;
}

FM2RecordHandle AM2Actor::CreateRecord(UM2Registry& InRegistry)
{
	M2_LOG_OBJECT(this, LogM2, Error, TEXT("CreateRecord must be overriden by all classes derived from AM2Actor."));
	return FM2RecordHandle();
}
