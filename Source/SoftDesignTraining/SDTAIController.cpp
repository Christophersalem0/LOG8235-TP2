// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTAIController.h"
#include "SoftDesignTraining.h"
#include "SDTBridge.h"
#include "SDTPathFollowingComponent.h"
#include "SoftDesignTrainingMainCharacter.h"
#include "DrawDebugHelpers.h"
#include "Kismet/KismetMathLibrary.h"

#include "SDTUtils.h"
#include "EngineUtils.h"

#include "NavigationSystem.h"

ASDTAIController::ASDTAIController(const FObjectInitializer& ObjectInitializer)
    : Super(ObjectInitializer.SetDefaultSubobjectClass<USDTPathFollowingComponent>(TEXT("PathFollowingComponent")))
{
           
}

void ASDTAIController::GoToBestTarget(float deltaTime)
{
    //Move to target depending on current behavior
    APawn* pawn = GetPawn();
    m_ReachedTarget = false;


    UPathFollowingComponent* path = GetPathFollowingComponent();
    if (path == nullptr)
    {
        return;
    }

    switch (m_PedestrianState)
    {
        case PedestrianState::SPAWNED:
        {
            FString tag("WaitPoint_Bridge_");
            AActor* actor = FindActorWithTag(tag);
            
            if (actor != nullptr)
            {
                // TODO : Agents wants to move towards actor
                MoveToActor(actor);
                m_PedestrianState = PedestrianState::GO_TO_BRIDGE;
            }
            break;
        }
        case PedestrianState::GO_TO_BRIDGE:
        {
            // Nothing to do
            break;
        }
        case PedestrianState::WAIT_AT_BRIDGE:
        {
            // Check if bridge is down
            FString tag("Bridge_");
            AActor* actor = FindActorWithTag(tag);
            ASDTBridge* bridge = Cast<ASDTBridge>(actor);
            
            // Once the bridge is down, we go through
            if (bridge != nullptr && bridge->GetState() == EBridgeState::BRIDGE_DOWN)
            {
                m_PedestrianState = PedestrianState::GO_TO_DESPAWN;
            }

            break;
        }
        case PedestrianState::GO_TO_DESPAWN:
        {
            FString tag("Despawn_");
            AActor* actor = FindActorWithTag(tag);
            
            if (actor != nullptr)
            {
                // TODO : Agents wants to move towards actor
                MoveToActor(actor);
            }
            
            break;
        }
        case PedestrianState::DESPAWN:
        {
            UnPossess();
            Destroy();

            pawn->Destroy();
            break;
        }
    }
}

void ASDTAIController::OnMoveCompleted(FAIRequestID RequestID, const FPathFollowingResult& Result)
{
    Super::OnMoveCompleted(RequestID, Result);

    m_ReachedTarget = true;

    // Safeguard
    if (Result.Code == EPathFollowingResult::Aborted)
    {
        return;
    }

    switch (m_PedestrianState)
    {
        case PedestrianState::GO_TO_BRIDGE:
        {
            m_PedestrianState = PedestrianState::WAIT_AT_BRIDGE;
            break;
        }
        case PedestrianState::GO_TO_DESPAWN:
        {
            m_PedestrianState = PedestrianState::DESPAWN;
            break;
        }
    }
}

void ASDTAIController::ShowNavigationPath()
{    
    // Show current navigation path DrawDebugLine and DrawDebugSphere
    // Use the UPathFollowingComponent of the AIController to get the path
    // This function is called while m_ReachedTarget is false 
    // Check void ASDTBaseAIController::Tick for how it works.
    UPathFollowingComponent* PathComp = GetPathFollowingComponent();
    if (!PathComp)
        return;

    FNavPathSharedPtr NavPath = PathComp->GetPath();
    if (!NavPath.IsValid())
        return;

    const TArray<FNavPathPoint>& Points = NavPath->GetPathPoints();
    if (Points.Num() == 0)
        return;

    const float SphereRadius = 15.0f;
    const int32 SphereSegments = 16;
    const float LineThickness = 8.0f;

    for (int32 i = 0; i < Points.Num() - 1; ++i)
    {
        DrawDebugLine(GetWorld(), Points[i].Location, Points[i + 1].Location, FColor::Red, false, 0.0f, 0, LineThickness);
        DrawDebugSphere(GetWorld(), Points[i].Location, SphereRadius, SphereSegments, FColor::Blue, false, 0.0f);
    }

    DrawDebugSphere(GetWorld(), Points.Last().Location, SphereRadius, SphereSegments, FColor::Blue, false, 0.0f);
}


void ASDTAIController::AIStateInterrupted()
{
    StopMovement();

    m_ReachedTarget = false;
}