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

#include "Testing/M2TestTables.h"

void UM2TestSet_Player::Initialize(FGuid NewSetId)
{
	Super::Initialize(NewSetId);

	M2_INITIALIZE_FIELD_WITH_SINGLETON(FM2TestField_Avatar, Avatar);
}

void UM2TestSet_Door::Initialize(FGuid NewSetId)
{
	Super::Initialize(NewSetId);

	M2_INITIALIZE_FIELD(FM2TestField_Avatar, Avatar);
	M2_INITIALIZE_FIELD(FM2TestField_Door, Door);
}

void UM2TestSet_Wall::Initialize(FGuid NewSetId)
{
	Super::Initialize(NewSetId);

	M2_INITIALIZE_FIELD(FM2TestField_Avatar, Avatar);
	M2_INITIALIZE_FIELD(FM2TestField_StaticEnvironment, StaticEnvironment);
	M2_INITIALIZE_TAG(FMTestTag_StaticEnvironment);
}

void UM2TestSet_Excluded::Initialize(FGuid NewSetId)
{
	Super::Initialize(NewSetId);

	M2_INITIALIZE_FIELD(FM2TestField_Avatar, Avatar);
}
