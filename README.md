# Mantle2 ECS

Author: Mason Stevenson

[https://masonstevenson-dev.github.io/](https://masonstevenson-dev.github.io/)

<br>

Mantle2 is an entity system for Unreal Engine. The goal for this project is to:

1) Provide some of the organizational benefits common in ECS architecture (query-by-composition, explicit tick order, per-entity operation framework).
2) Pick the "low hanging fruit" for performance optimizations (aka, be cache friendly, but not like *that* cache friendly).
3) Have simple and maintainable internals (prioritize using standard UE framework over building bespoke solutions).
4) Avoid the parts of ECS design that are probably irrelevant for the majority of indie devs (looking at you, dynamic composition).

<br>

This project is still very much **WIP**. Use at your own risk.

<br>

## Supported Engine Versions

Mantle2 currently supports **UE 5.6.**

<br>

## Required Plugins

This plugin uses my [AnankeCore](https://github.com/masonstevenson-dev/AnankeCore) plugin. This plugin is required and must be installed in order for Mantle2 to compile.

<br>

## Installation

Install this plugin in your project's plugins folder. The folder structure should be: `YourProject/Plugins/Mantle2/Mantle2/Source`.

<br>

The plugin is enabled by default.

<br>

## Architecture

Mantle implements a fairly standard ECS architecture. For for those familiar with the usual ECS naming conventions, here is the mapping of terms:

* **Entities** => **Records**
  * A "record" is just an ID that points to some data.
* **Components** => **Fields**
  * That data each entity is associated with is a collection of fields. These are just standard USTRUCTs and are grouped together in a **RecordSet**, which is were all records with the same field composition are stored.
* **Systems** => **Operations**
  * An operation is where you define your logic for manipulating batches of entities. It is essentially just an OnTick() function, and mantle gives you an easy way to explicitly set the order of what ticks when so it is easy to keep track of the sequence of events each frame.

<br>

Some other important concepts:

*   The **Engine** (`UM2Engine`):
    *   A `UGameInstanceSubsystem` that holds everything related to Mantle, and also manages tick order of operations.

*   The **Registry** (`UM2Registry`): 
    *   The central database that holds all ECS data. It manages collections of Record Sets and provides an API to query and modify records.

*   **Record Sets** (`UM2RecordSet`): 
    *   Each RecordSet is a data container for a particular entity type. Under the hood, all the RecordSet is really doing is storing a bunch of `TArray<FYourFieldStruct>` for each field you have associated with it. Mantle simply provides a standardized way of accessing this field data.
    *   Note this follows the standard "Array of Structs" ECS paradigm, where each entity has a slot reserved for it in every field array. M2 doesn't use any special compaction techniques or dynamic field composition for entities. It's just standard vanilla UPROPERTY() marked TArrays.

<br>

## Usage

### 1. Creating a Game Instance

To use Mantle, you must manually configure which operations you wish to run from your game instance. Inherit from `UM2GameInstance` and override `ConfigureM2Engine()`. This will handle configuring engine loops and starting the `M2Engine`.

```cpp
#include "Engine/M2GameInstance.h"
// ...

UCLASS()
class UMyGameInstance : public UM2GameInstance
{
    GENERATED_BODY()

protected:
    virtual void ConfigureM2Engine(UM2Engine& Engine) override
    {
        Super::ConfigureM2Engine(Engine);

        // Configure engine loops and add operations
        FM2EngineLoopOptions Options;
        
        // Add operation groups and schedule your operations here
        // Engine.NewOperation<UMyOperation>();
        
        Engine.ConfigureEngineLoop(ETickingGroup::TG_PrePhysics, Options);
    }
};
```

<br>


### 2. Defining a Record Set

Fields are just standard vanilla USTRUCTs. Be sure to mark UPROPERTY() for serialization. The macros M2_DECLARE_FIELD and M2_INITIALIZE_FIELD basically just setup the `TArray<FYourFieldType>` and take care of a little behind the scenes boilerplate for you. They are required for each field you create.

```cpp
#include "Foundation/M2RecordSet.h"
// ...

USTRUCT()
struct FMyHealthField
{
    GENERATED_BODY()
        
public:
    void Init(float StartingHealth)
    {
        Health = StartingHealth;
    }
    
    UPROPERTY()
    float Damage = 0.0f;
    
    UPROPERTY()
    float Healing = 0.0f;
    
protected:
    friend UMyHealthOperation;
    
    UPROPERTY()
    float Health = 100.0f;
};

UCLASS()
class UMyRecordSet : public UM2RecordSet
{
    GENERATED_BODY()

    M2_DECLARE_FIELD(FMyHealthField, HealthFields);

public:
    virtual void Initialize(FGuid NewSetId) override
    {
        Super::Initialize(NewSetId);
        M2_INITIALIZE_FIELD(FMyHealthField, HealthFields);
    }
};
```

<br>

### 3. Creating an Operation

Operations run logic on your Record Sets. Create a new operation by inheriting from `UM2Operation` and implementing `PerformOperation`.

```cpp
#include "Foundation/M2Operation.h"
// ...

UCLASS()
class UMyHealthOperation : public UM2Operation
{
    GENERATED_BODY()

protected:
    virtual void PerformOperation(FM2OperationContext& Ctx) override
    {
        UM2Registry* Registry = Ctx.Registry.Get();
        
        TArray<UScriptStruct*> IncludeFields;
        TArray<UScriptStruct*> ExcludeFields;
        
        IncludeFields.Add(FMyHealthField::StaticStruct());
        
        // GetAll() will grab every RecordSet with the matching fields. Then below, you need to iterate through the field arrays
        // for each one.
        for (UM2RecordSet* ResultSet : Registry->GetAll(IncludeFields, ExcludeFields))
		{
            if (!ResultSet)
            {
                continue;
            }
            
            TArrayView<FM2RecordHandle> RecordHandles = ResultSet->GetHandles();
			TArrayView<FMyHealthField> HealthFields = RecordSet->GetFieldArray<FMyHealthField>();
            
            for (int32 RecordIndex = 0; RecordIndex < RecordHandles.Num(); ++RecordIndex)
			{
                FMyHealthField& HealthField = HealthFields[RecordIndex];
                
                HealthField.Health += (HealthField.Healing - HealthField.Damage);
                HealthField.Healing = 0.0f;
                HealthField.Damage = 0.0f;
            }
        }
    }
};
```

<br>

### 4. Creating a Record

To create a new record, simply grab the registry from the game instance and call `AddRecord()` specifying which RecordSet you want to use. From there, you can call `GetField()` to set individual field values.

```cpp
void SomeFunc()
{
    if (UM2Registry* Registry = UM2EngineLibrary::GetRegistry(GetGameInstance()))
    {
        FM2RecordHandle NewHandle = Registry->AddRecord<UMyRecordSet>();
        
        FMyHealthField* HealthField = Registry->GetField<FMyHealthField>(NewHandle);
        HealthField.Init(200.0f);
    }
}
```
