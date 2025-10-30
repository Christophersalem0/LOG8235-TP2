// Fill out your copyright notice in the Description page of Project Settings.

#include "SDTPathFollowingComponent.h"
#include "SoftDesignTraining.h"
#include "SDTUtils.h"
#include "SDTAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "NavigationSystem.h"
#include "NavLinkCustomInterface.h"
#include "SoftDesignTrainingMainCharacter.h"

#include "DrawDebugHelpers.h"

USDTPathFollowingComponent::USDTPathFollowingComponent(const FObjectInitializer& ObjectInitializer)
{

}

/**
* This function is called every frame while the AI is following a path.
* MoveSegmentStartIndex and MoveSegmentEndIndex specify where we are on the path point array.
*/
void USDTPathFollowingComponent::FollowPathSegment(float DeltaTime)
{
    const TArray<FNavPathPoint>& points = Path->GetPathPoints();
    const FNavPathPoint& segmentStart = points[MoveSegmentStartIndex];
    const FNavPathPoint& segmentEnd = points[MoveSegmentEndIndex];
    //UE_LOG(LogTemp, Warning, TEXT("Path has %d points"), points.Num());


    // If the current segment is a jump
    if (SDTUtils::HasJumpFlag(segmentStart))
    {
        APlayerController* PC = Cast<APlayerController>(GetOwner());
        if (!PC)
            return;

        ASoftDesignTrainingMainCharacter* PawnChar = Cast<ASoftDesignTrainingMainCharacter>(PC->GetPawn());
        if (!PawnChar)
            return;


        if (PawnChar->m_JumpCurve)
        {
            UE_LOG(LogTemp, Warning, TEXT("Entered jump"));
            const FRichCurve& CurveData = PawnChar->m_JumpCurve->FloatCurve;

            const FRichCurveKey& FirstKey = CurveData.GetFirstKey();
            const FRichCurveKey& LastKey = CurveData.GetLastKey();
            const float jumpTime = LastKey.Time - FirstKey.Time;

            remainingJumpTime += DeltaTime;
            remainingJumpTime = FMath::Clamp(remainingJumpTime, FirstKey.Time, LastKey.Time);

            
            float ZCurveValue = PawnChar->m_JumpCurve->GetFloatValue(remainingJumpTime);
            float ZOffset = ZCurveValue * 1000.f; 

            const FVector displacement = ((segmentEnd.Location - segmentStart.Location) / jumpTime) * DeltaTime;

            const FVector currentLocation = PawnChar->GetActorLocation();
            PawnChar->SetActorLocation(FVector(currentLocation.X + displacement.X, currentLocation.Y + displacement.Y, ZOffset + zValue), true);
         
        }
    }
    else
    {
        // Update navigation along path (move along)
        Super::FollowPathSegment(DeltaTime);
        
    }
}

/**
* This function is called every time the AI has reached a new point on the path.
* If you need to do something at a given point in the path, this is the place.
*/
void USDTPathFollowingComponent::SetMoveSegment(int32 segmentStartIndex)
{
    Super::SetMoveSegment(segmentStartIndex);

    const TArray<FNavPathPoint>& points = Path->GetPathPoints();

    const FNavPathPoint& segmentStart = points[MoveSegmentStartIndex];
    const FNavPathPoint& segmentEnd = points[MoveSegmentStartIndex + 1];


    if (SDTUtils::HasJumpFlag(segmentStart) && FNavMeshNodeFlags(segmentStart.Flags).IsNavLink())
    {
        APlayerController* PC = Cast<APlayerController>(GetOwner());
        ASoftDesignTrainingMainCharacter* PawnChar = Cast<ASoftDesignTrainingMainCharacter>(PC->GetPawn());

        const FVector JumpDirection = (segmentEnd.Location - segmentStart.Location).GetSafeNormal();
        FRotator DesiredRotation = JumpDirection.Rotation();
        PawnChar->SetActorRotation(DesiredRotation);


        // set le temps du jump
        const FRichCurve& CurveData = PawnChar->m_JumpCurve->FloatCurve;
        const FRichCurveKey& FirstKey = CurveData.GetFirstKey();
        remainingJumpTime = FirstKey.Time;
        zValue = PawnChar->GetActorLocation().Z;

    }
    else
    {
        
        
    }
}

