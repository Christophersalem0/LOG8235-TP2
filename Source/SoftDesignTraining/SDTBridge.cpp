// Fill out your copyright notice in the Description page of Project Settings.


#include "SDTBridge.h"
#include "SDTBoat.h"
#include "SDTBoatAIController.h"
#include "Components/TextRenderComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/OverlapResult.h"

// Sets default values
ASDTBridge::ASDTBridge()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}

// Called when the game starts or when spawned
void ASDTBridge::BeginPlay()
{
	Super::BeginPlay();

	m_State = EBridgeState::BRIDGE_UP;
	m_IsMoving = false;
	m_BridgeOpeningAlpha = 1.0f;
	m_BridgeOpenningSpeed = 0.5f;
}

// Called every frame
void ASDTBridge::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	VerifyIncomingBoats();
	UE_LOG(LogTemp, Log, TEXT("m_CanActivate: %d"), (m_CanActivate));
	if (m_CanActivate) {
		FString textTag = Tags[0].ToString() + "_Text";
		TArray<UActorComponent*> Components = GetComponentsByTag(UTextRenderComponent::StaticClass(), *textTag);
		if (Components.Num() > 0) {
			if (UTextRenderComponent* TextRender = Cast<UTextRenderComponent>(Components[0]))
			{
				TextRender->SetText(FText::FromString("Hold Space To Activate"));
			}
		}

		FString partTag = Tags[0].ToString() + "_Part";
		FName partName(*partTag);

		TArray<UActorComponent*> parts = GetComponentsByTag(UStaticMeshComponent::StaticClass(), partName);

		UMaterialInterface* NewMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));

		if (NewMaterial)
		{
			for (UActorComponent* part : parts)
			{
				UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(part);
				if (MeshComp)
				{
					MeshComp->SetMaterial(0, NewMaterial);
					UE_LOG(LogTemp, Log, TEXT("Changed material on: %s"), *MeshComp->GetName());
				}
			}
		}
	}
	else {
		UE_LOG(LogTemp, Log, TEXT("DISABLE"));
		FString textTag = Tags[0].ToString() + "_Text";
		UE_LOG(LogTemp, Log, TEXT("Tag: %s"), *textTag);
		TArray<UActorComponent*> Components = GetComponentsByTag(UTextRenderComponent::StaticClass(), *textTag);
		if (Components.Num() > 0) {
			if (UTextRenderComponent* TextRender = Cast<UTextRenderComponent>(Components[0]))
			{
				TextRender->SetText(FText::FromString("Wait! A boat is passing! "));
			}
		}

		FString partTag = Tags[0].ToString() + "_Part";
		FName partName(*partTag);

		TArray<UActorComponent*> parts = GetComponentsByTag(UStaticMeshComponent::StaticClass(), partName);

		UMaterialInterface* NewMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Mats/Red.Red"));

		if (NewMaterial)
		{
			for (UActorComponent* part : parts)
			{
				UStaticMeshComponent* MeshComp = Cast<UStaticMeshComponent>(part);
				if (MeshComp)
				{
					MeshComp->SetMaterial(0, NewMaterial);
					UE_LOG(LogTemp, Log, TEXT("Changed material on: %s"), *MeshComp->GetName());
				}
			}
		}
	}

	if (!m_IsMoving)
	{
		return;
	}

	if (m_State == EBridgeState::BRIDGE_GOING_UP)
	{
		m_BridgeOpeningAlpha += DeltaTime * m_BridgeOpenningSpeed;
		if (m_BridgeOpeningAlpha > 1.f)
		{
			m_BridgeOpeningAlpha = 1.f;
			m_IsMoving = false;
			m_State = EBridgeState::BRIDGE_UP;
		}
	}
	else if (m_State == EBridgeState::BRIDGE_GOING_DOWN)
	{
		m_BridgeOpeningAlpha -= DeltaTime * m_BridgeOpenningSpeed;
		if (m_BridgeOpeningAlpha < 0.f)
		{
			m_BridgeOpeningAlpha = 0.f;
			m_IsMoving = false;
			m_State = EBridgeState::BRIDGE_DOWN;
		}
	}
}

bool ASDTBridge::Activate()
{
	if (!m_CanActivate) { 
		return false; 
	}
	if (m_State == EBridgeState::BRIDGE_UP)
	{
		m_State = EBridgeState::BRIDGE_GOING_DOWN;
	}
	else if (m_State == EBridgeState::BRIDGE_DOWN)
	{
		m_State = EBridgeState::BRIDGE_GOING_UP;
	}

	m_IsMoving = true;
	return true;
}

void ASDTBridge::Deactivate()
{
	m_IsMoving = false;
}

void ASDTBridge::VerifyIncomingBoats()
{
	TArray<FOverlapResult> boathits;
	GetWorld()->OverlapMultiByChannel(boathits, GetActorLocation(), GetActorRotation().Quaternion(), ECollisionChannel::ECC_WorldDynamic, FCollisionShape::MakeSphere(1000.f));

	FString tag("Bridge_0");
	TArray<AActor*> foundActors;
	UGameplayStatics::GetAllActorsWithTag(this, *tag, foundActors);
	BoatState stateToCheck = foundActors[0]->GetName() == GetName() ? BoatState::GO_TO_OPERATOR : BoatState::GO_TO_DESPAWN;
	m_CanActivate = true;
	// Draw the hits
	for (FOverlapResult& hitResult : boathits)
	{
		AActor* hitActor = hitResult.GetActor();
		ASDTBoat* boat = Cast<ASDTBoat>(hitActor);
		if (boat)
		{
			ASDTBoatAIController* boatController = Cast<ASDTBoatAIController>(boat->GetController());
			//UE_LOG(LogTemp, Log, TEXT("GOTOPERATOR: %d"), boatController->GetBoatState() == BoatState::GO_TO_OPERATOR);
			UE_LOG(LogTemp, Log, TEXT("distance: %f"), (GetActorLocation().Y - boat->GetActorLocation().Y));
			if (GetActorLocation().Y - boat->GetActorLocation().Y < 400 && boatController->GetBoatState() == stateToCheck && !boatController->GetReachedTarget()) {
				m_CanActivate = false;
			}
		}
	}
}


EBridgeState ASDTBridge::GetState() const
{
	return m_State;
}