#pragma once

#include "CoreMinimal.h"
#include "DestructionMeshType.generated.h"

UENUM(BlueprintType)
enum class EDestructionMeshType : uint8
{
    StaticMesh UMETA(DisplayName = "Static Mesh"),
    GeometryCollection UMETA(DisplayName = "Geometry Collection")
};