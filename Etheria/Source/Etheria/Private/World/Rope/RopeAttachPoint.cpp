// Fill out your copyright notice in the Description page of Project Settings.


#include "World/Rope/RopeAttachPoint.h"

#include "Components/SphereComponent.h"

ARopeAttachPoint::ARopeAttachPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	DetectionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("DetectionSphere"));
	DetectionSphere->SetupAttachment(Root);
	DetectionSphere->SetSphereRadius(DetectionRadius);

	DetectionSphere->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
