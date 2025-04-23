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

#pragma once

// Log Categories
M2RUNTIME_API DECLARE_LOG_CATEGORY_EXTERN(LogM2, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogM2Test, Log, All);

// Statgroups
DECLARE_STATS_GROUP(TEXT("M2"), STATGROUP_M2, STATCAT_M2);
DECLARE_STATS_GROUP(TEXT("M2"), STATGROUP_M2Errors, STATCAT_M2);

// Counters
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Entity Count"), STAT_M2_EntityCount, STATGROUP_M2, M2RUNTIME_API);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Temporary Entities Created"), STAT_M2_TempararyEntitiesAdded, STATGROUP_M2, M2RUNTIME_API);
DECLARE_DWORD_ACCUMULATOR_STAT_EXTERN(TEXT("Temporary Entities Removed"), STAT_M2_TempararyEntitiesRemoved, STATGROUP_M2, M2RUNTIME_API);