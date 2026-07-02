// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveProp/ACoalHandVisualActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "UObject/ConstructorHelpers.h"
#include "Net/UnrealNetwork.h"

// Sets default values
AACoalHandVisualActor::AACoalHandVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;
	// 무기와 동일하게 복제되는 액터: HolderCharacter/소켓이 클라에 복제되어 스스로 부착한다.
	bReplicates = true;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	SetRootComponent(Mesh);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(
		TEXT("/Engine/BasicShapes/Sphere.Sphere")
	);

	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
	}

	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void AACoalHandVisualActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(AACoalHandVisualActor, HolderCharacter);
	DOREPLIFETIME(AACoalHandVisualActor, AttachSocketName);
}

void AACoalHandVisualActor::BeginPlay()
{
	Super::BeginPlay();

	if (CoalMaterial)
	{
		Mesh->SetMaterial(0, CoalMaterial);
	}

	SetActorScale3D(FVector(VisualScale));
}

void AACoalHandVisualActor::AttachToCharacter(ACharacter* InCharacter, FName InSocketName)
{
	HolderCharacter = InCharacter;
	AttachSocketName = InSocketName;

	// 서버에서 즉시 부착(클라는 OnRep_HolderCharacter 에서 동일하게 부착).
	ApplyAttachment();
}

void AACoalHandVisualActor::OnRep_HolderCharacter()
{
	ApplyAttachment();
}

void AACoalHandVisualActor::ApplyAttachment()
{
	if (!HolderCharacter) return;

	if (USkeletalMeshComponent* OwnerMesh = HolderCharacter->GetMesh())
	{
		AttachToComponent(OwnerMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale,
			AttachSocketName);

		SetActorScale3D(FVector(VisualScale));
	}
}
