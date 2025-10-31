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
    isJumping = false;


    // If the current segment is a jump
    if (SDTUtils::HasJumpFlag(segmentStart))
    {

        APlayerController* PC = Cast<APlayerController>(GetOwner());
        if (!PC)
            return;

        ASoftDesignTrainingMainCharacter* PawnChar = Cast<ASoftDesignTrainingMainCharacter>(PC->GetPawn());
        if (!PawnChar)
            return;

        if (isWaitingJump) {
            jumpDelay += DeltaTime;
            FRotator JumpRotation = (segmentEnd.Location - segmentStart.Location).GetSafeNormal().Rotation();
            float RotationDiff = FMath::Abs(FMath::UnwindDegrees(
                PawnChar->GetActorRotation().Yaw - JumpRotation.Yaw));


            if (jumpDelay < waitingTime && RotationDiff > 10.f) {
                FRotator interpRotation = FMath::RInterpTo(PawnChar->GetActorRotation(), (segmentEnd.Location - segmentStart.Location).GetSafeNormal().Rotation(), DeltaTime, 5.f);
                PawnChar->SetActorRotation(interpRotation);
                return;

            } else {
                jumpDelay = 0.0f;
                isWaitingJump = false;
                isJumping = true;
                PawnChar->SetActorRotation(JumpRotation);
            }
        }


        else if (PawnChar->m_JumpCurve)
        {
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
            jumpProgress = FMath::Abs(remainingJumpTime / jumpTime);
        }
    }
    else
    {
        FVector lineStart(segmentStart.Location);
        FVector lineEnd(segmentEnd.Location);
       
        const FVector currentLocation = NavMovementInterface->GetFeetLocation();

     
        FVector moveDirection = (lineEnd - currentLocation).GetSafeNormal();
        const FVector PathEnd = Path->GetEndLocation();
        const FVector::FReal DistToEndSq = FVector::DistSquared(currentLocation, PathEnd);
        float distanceToDecelerate = 14.0f;
        const bool shouldDecelerate = DistToEndSq < FMath::Square(distanceToDecelerate);


        const bool isCloseToNextSegment = (currentLocation - lineEnd).Size() < 100;
        const bool isNotLastSeg = MoveSegmentEndIndex != Path->GetPathPoints().Num() - 1;
        const bool secondVerif = isNotLastSeg && (FMath::Acos(FVector::DotProduct((points[MoveSegmentEndIndex + 1].Location - lineEnd).GetSafeNormal(), moveDirection)) + 1 ) / 2 < 0.5;

        if (shouldDecelerate || secondVerif)
        {
            const FVector::FReal  SpeedPct = FMath::Clamp(FMath::Sqrt(DistToEndSq) / CachedBrakingDistance, 0., 1.);
            moveDirection *= SpeedPct;
        }

        PostProcessMove.ExecuteIfBound(this, moveDirection);
        NavMovementInterface->RequestPathMove(moveDirection);
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
    isJumping = false;
    jumpProgress = 0;
    const FNavPathPoint& segmentStart = points[MoveSegmentStartIndex];
    const FNavPathPoint& segmentEnd = points[MoveSegmentStartIndex + 1];


    if (SDTUtils::HasJumpFlag(segmentStart) && FNavMeshNodeFlags(segmentStart.Flags).IsNavLink())
    {
        isWaitingJump = true;
        APlayerController* PC = Cast<APlayerController>(GetOwner());
        ASoftDesignTrainingMainCharacter* PawnChar = Cast<ASoftDesignTrainingMainCharacter>(PC->GetPawn());

       /* const FVector JumpDirection = (segmentEnd.Location - segmentStart.Location).GetSafeNormal();
        FRotator DesiredRotation = JumpDirection.Rotation();
        PawnChar->SetActorRotation(DesiredRotation);*/


        const FRichCurve& CurveData = PawnChar->m_JumpCurve->FloatCurve;
        const FRichCurveKey& FirstKey = CurveData.GetFirstKey();
        remainingJumpTime = FirstKey.Time;
        zValue = PawnChar->GetActorLocation().Z;
        jumpProgress = 0;

    }
    else
    {
        if (MoveSegmentEndIndex != Path->GetPathPoints().Num() - 1)
        {
            TArray<FVector> sampleShortcutPoints;

            sampleShortcutPoints.Empty();
            sampleShortcutPoints.Reserve(5);
            FVector originBB = FVector::ZeroVector;
            float radius = NavMovementInterface->GetNavAgentPropertiesRef().AgentRadius;
            FVector extentsBB(0, 0, radius);
            FVector destination;
            FVector2D destination2D;
            FVector displacementSamples = (points[MoveSegmentEndIndex].Location - segmentStart.Location) / 5;
            for (int i = 0; i < 5; i++)
            {
                FVector sample = segmentStart.Location + ((i + 1) * displacementSamples);
                sampleShortcutPoints.Push(sample);
            }
            for (int i = 5 - 1; i >= 0; i--)
            {
                FVector start = NavMovementInterface->GetFeetLocation();
                start.Z = originBB.Z;
                FVector end = sampleShortcutPoints[i];
                end.Z = originBB.Z;
                FVector sideVectorLeft = FVector::CrossProduct((end - start), FVector::UpVector);
                sideVectorLeft.Normalize();
                sideVectorLeft *= extentsBB.Size();
                FVector sideVectorRight = -sideVectorLeft;

                FHitResult HitOut;
                if (!SDTUtils::Raycast(GetWorld(),
                    start + sideVectorLeft,
                    end + sideVectorLeft
                )
                    &&
                    !SDTUtils::Raycast(GetWorld(),
                        start + sideVectorRight,
                        end + sideVectorRight
                    )
                    )
                {
                    destination = sampleShortcutPoints[i];
                    destination2D = FVector2D(sampleShortcutPoints[i]);
                    if (destination.Equals(Path->GetPathPoints()[MoveSegmentEndIndex + 1]))
                        MoveSegmentEndIndex++;
                    break;
                }
            }
        }
    }
}

