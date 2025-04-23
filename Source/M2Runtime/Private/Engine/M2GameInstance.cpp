﻿// Copyright © Mason Stevenson
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

#include "Engine/M2GameInstance.h"

#include "Foundation/M2Engine.h"
#include "Logging/M2LoggingDefs.h"
#include "Logging/M2LoggingMacros.h"
#include "Macros/AnankeCoreLoggingMacros.h"

void UM2GameInstance::Init()
{
	Super::Init(); // The UM2Engine subsystem will be initialized here.

	auto* M2Engine = GetSubsystem<UM2Engine>(this);
	check(M2Engine);
	
	ConfigureM2Engine(*M2Engine);
	M2Engine->FinishConfiguration();
	FWorldDelegates::OnWorldBeginTearDown.AddUObject(this, &ThisClass::OnWorldTearDown);
	
	bIsInitialized = true;

	if (CurrentWorld.IsValid())
	{
		OnWorldChanged(nullptr, CurrentWorld.Get());
	}
}

void UM2GameInstance::Shutdown()
{
	Super::Shutdown();

	FWorldDelegates::OnWorldBeginTearDown.RemoveAll(this);
	if (CurrentWorld.IsValid())
	{
		CurrentWorld->OnWorldBeginPlay.RemoveAll(this);
	}
	bIsInitialized = false;
}

void UM2GameInstance::OnWorldChanged(UWorld* OldWorld, UWorld* NewWorld)
{
	if (CurrentWorld == NewWorld)
	{
		return;
	}
	
	if (!NewWorld || !NewWorld->IsGameWorld())
	{
		CurrentWorld = nullptr;
		return;
	}

	CurrentWorld = NewWorld;

	if (CurrentWorld->HasBegunPlay())
	{
		M2_LOG_OBJECT(this, LogM2, Warning, TEXT("Current world has already begun play."));
		OnWorldBeginPlay();
	}
	else
	{
		CurrentWorld->OnWorldBeginPlay.AddUObject(this, &ThisClass::OnWorldBeginPlay);
	}
	
	M2_LOG_OBJECT(this, LogM2, Log, TEXT("World changed to %s"), *CurrentWorld->GetName());
}

void UM2GameInstance::OnWorldBeginPlay()
{
	if (!bIsInitialized)
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("World has begun play but M2Engine has not been initialized."))
		return;
	}
	if (!CurrentWorld.IsValid())
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("Expected valid world."));
		return;
	}

	UWorld* StartedWorld = CurrentWorld.Get();

	auto* M2Engine = GetSubsystem<UM2Engine>(this);
	check(M2Engine);

	if (M2Engine->IsStarted())
	{
		M2_LOG_OBJECT(this, LogM2, Error, TEXT("Expected M2Engine to be stopped."));
		M2Engine->Stop();
	}

	M2Engine->Start(*StartedWorld);
}

void UM2GameInstance::OnWorldTearDown(UWorld* OldWorld)
{
	auto* M2Engine = UGameInstance::GetSubsystem<UM2Engine>(this);
	check(M2Engine);
	
	if (!M2Engine->IsStarted())
	{
		return;
	}

	M2_LOG_OBJECT(this, LogM2, Log, TEXT("World teardown has begun: Stopping M2."))
	M2Engine->Stop();
}