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
    UE_LOG(LogTemp, Warning, TEXT("Path has %d points"), points.Num());

    for (int32 i = 0; i < points.Num(); i++)
    {
        const FVector& Location = points[i].Location;
        UE_LOG(LogTemp, Warning, TEXT("Point %d: X=%f Y=%f Z=%f"), i, Location.X, Location.Y, Location.Z);
    }
    // If the current segment is a jump
    if (SDTUtils::HasJumpFlag(segmentStart))
    {
    }
    else
    {
        // Update navigation along path (move along)
        FVector lineStart(segmentStart.Location);
        FVector lineEnd(segmentEnd.Location);
        FVector lineDirection = lineEnd - lineStart;
        lineDirection.Normalize();
        const FVector currentLocation = NavMovementInterface->GetFeetLocation();
        FVector projectedPosition = FMath::ClosestPointOnLine(lineStart, lineEnd, currentLocation);
        FVector destination = lineEnd;
        if ((currentLocation - lineEnd).Size() > 1.f) {
            destination = projectedPosition + ((lineEnd - projectedPosition) / 2);
        }

        UE_LOG(LogTemp, Warning, TEXT("Destination: X=%f Y=%f Z=%f"), destination.X, destination.Y, destination.Z);
        FVector MoveVelocity = (destination - currentLocation) / DeltaTime;
        NavMovementInterface->RequestDirectMove(MoveVelocity, false);
        
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
        // Handle starting jump
        

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
            FVector extentsBB(0,0,radius);
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

