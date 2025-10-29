// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTPlayerNavigationFilter.h"
#include "SDTNavArea_Jump.h"
#include "SoftDesignTraining.h"
#include "NavAreas/NavArea_Default.h"

#include "SDTUtils.h"

USDTPlayerNavigationFilter::USDTPlayerNavigationFilter(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer)
{
    TSubclassOf<UNavArea_Default> ZoneDefault = TSubclassOf<UNavArea_Default>(UNavArea_Default::StaticClass());
    TSubclassOf<USDTNavArea_Jump> ZoneJump = TSubclassOf<USDTNavArea_Jump>(USDTNavArea_Jump::StaticClass());

    AddTravelCostOverride(ZoneJump, 1.0f);
    AddTravelCostOverride(ZoneDefault, 1.0f);

}
