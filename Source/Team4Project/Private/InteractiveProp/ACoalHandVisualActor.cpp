// Fill out your copyright notice in the Description page of Project Settings.


#include "InteractiveProp/ACoalHandVisualActor.h"

// Sets default values
AACoalHandVisualActor::AACoalHandVisualActor()
{
	PrimaryActorTick.bCanEverTick = false;

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

void AACoalHandVisualActor::BeginPlay()
{
	Super::BeginPlay();
	
	if (CoalMaterial)
	{
		Mesh->SetMaterial(0, CoalMaterial);
	}
}

void AACoalHandVisualActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	
	SetActorScale3D(FVector(VisualScale));
}