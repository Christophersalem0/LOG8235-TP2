#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "SDTPlayerNavigationFilter.generated.h"

/**
 *
 */
UCLASS()
class SOFTDESIGNTRAINING_API USDTPlayerNavigationFilter : public UNavigationQueryFilter
{
	GENERATED_BODY()

public:
	USDTPlayerNavigationFilter(const FObjectInitializer& ObjectInitializer);
};
