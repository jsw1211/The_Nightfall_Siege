// Fill out your copyright notice in the Description page of Project Settings.


#include "BaseController.h"
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

    APawn* Pawn = GetPawn();
    if (!Pawn) return;

    FVector Direction = Hit.Location - Pawn->GetActorLocation();
    Direction.Z = 0.f;

    FRotator TargetRotation = Direction.Rotation();

    Pawn->SetActorRotation(TargetRotation);
}
