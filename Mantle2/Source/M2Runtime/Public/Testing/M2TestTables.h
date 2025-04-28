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
#include "Foundation/M2RecordSet.h"

#include "M2TestTables.generated.h"

class TestSuite;

USTRUCT()
struct FM2TestField_Avatar
{
	GENERATED_BODY()

public:
	UPROPERTY()
	FVector WorldPosition = FVector::ZeroVector;
};

USTRUCT()
struct FM2TestField_Door
{
	GENERATED_BODY()

public:
	UPROPERTY()
	bool bIsOpen = false;
};

USTRUCT()
struct FM2TestField_StaticEnvironment
{
	GENERATED_BODY()

public:
	UPROPERTY()
	float Opacity = 0.0f;
};

USTRUCT()
struct FMTestTag_StaticEnvironment { GENERATED_BODY() };

UCLASS()
class UM2TestRecordSet : public UM2RecordSet { GENERATED_BODY() };

UCLASS()
class UM2TestSet_Player : public UM2TestRecordSet
{
	GENERATED_BODY()

public:
	friend TestSuite;
	
	virtual void Initialize(FGuid NewSetId) override;

	M2_DECLARE_FIELD_WITH_SINGLETON(FM2TestField_Avatar, Avatar);
};

UCLASS()
class UM2TestSet_Door : public UM2TestRecordSet
{
	GENERATED_BODY()

public:
	friend TestSuite;
	
	virtual void Initialize(FGuid NewSetId) override;

	M2_DECLARE_FIELD(FM2TestField_Avatar, Avatar);
	M2_DECLARE_FIELD(FM2TestField_Door, Door);
};

UCLASS()
class UM2TestSet_Wall : public UM2TestRecordSet
{
	GENERATED_BODY()

public:
	friend TestSuite;
	
	virtual void Initialize(FGuid NewSetId) override;

	M2_DECLARE_FIELD(FM2TestField_Avatar, Avatar);
	M2_DECLARE_FIELD(FM2TestField_StaticEnvironment, StaticEnvironment);
};

UCLASS()
class UM2TestSet_Excluded : public UM2TestRecordSet
{
	GENERATED_BODY()

public:
	virtual void Initialize(FGuid NewSetId) override;

	M2_DECLARE_FIELD(FM2TestField_Avatar, Avatar);
};
