// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "SDTPlayerNavFilter.generated.h"

/**
 * 
 */
UCLASS()
class SOFTDESIGNTRAINING_API USDTPlayerNavFilter : public UNavigationQueryFilter
{
	GENERATED_BODY()

public:
	USDTPlayerNavFilter(const FObjectInitializer& ObjectInitializer);
};
