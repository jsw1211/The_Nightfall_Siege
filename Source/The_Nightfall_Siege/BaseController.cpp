// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseController.h"
#include "BaseCharacter.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

void ABaseController::SetupInputComponent()
{
    Super::SetupInputComponent();

	InputComponent->BindAction("RightClick", IE_Pressed, this, &ABaseController::MoveToMouse);
}

void ABaseController::OnRightClick()
{
    FHitResult Hit;
    if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
    {
        RotateCharacterToCursor();

        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
    }
}

void ABaseController::MoveToMouse()
{
    ABaseCharacter* MyCharacter = Cast<ABaseCharacter>(GetPawn());
    if (MyCharacter && MyCharacter->bIsUsingSkill)
    {
        return; // ⭐ 스킬 중이면 이동 막기
    }

    FHitResult Hit;
    GetHitResultUnderCursor(ECC_Visibility, false, Hit);

    if (Hit.bBlockingHit)
    {
        UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
    }
}

void ABaseController::RotateCharacterToCursor()
{
    FHitResult Hit;
    if (!GetHitResultUnderCursor(ECC_Visibility, false, Hit))
        return;

    APawn* BasePawn = GetPawn();
    if (!BasePawn) return;

    FVector Direction = Hit.Location - BasePawn->GetActorLocation();
    Direction.Z = 0.f;

    FRotator TargetRotation = Direction.Rotation();

    BasePawn->SetActorRotation(TargetRotation);
}

void ABaseController::BeginPlay()
{
    Super::BeginPlay();

    bShowMouseCursor = true;
    DefaultMouseCursor = EMouseCursor::Default;

    SetIgnoreLookInput(true); // 🔥 마우스 회전 입력 차단
}