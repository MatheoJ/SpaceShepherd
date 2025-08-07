// SpaceShepherdCharacter.h
// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Logging/LogMacros.h"
#include "SpaceShepherdCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class UInputAction;
struct FInputActionValue;

DECLARE_LOG_CATEGORY_EXTERN(LogTemplateCharacter, Log, All);

/**
 *  A simple player-controllable third person character
 *  Implements a controllable orbiting camera and cow herding mechanics
 */
UCLASS(abstract)
class ASpaceShepherdCharacter : public ACharacter
{
    GENERATED_BODY()

    /** Camera boom positioning the camera behind the character */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    USpringArmComponent* CameraBoom;

    /** Follow camera */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
    UCameraComponent* FollowCamera;

    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnGravityChanged();
    
protected:

    // ========== Input Actions ==========
    
    /** Jump Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* JumpAction;

    /** Move Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* MoveAction;

    /** Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* LookAction;

    /** Mouse Look Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* MouseLookAction;
    
    /** Pickup/Drop Cow Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* PickupAction;
    
    /** Throw Cow Input Action (Hold to charge) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* ThrowAction;
    
    /** Attraction Mode Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* AttractionAction;
    
    /** Repulsion Mode Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* RepulsionAction;
    
    /** Neutral Mode Input Action */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Input")
    UInputAction* NeutralAction;

    // ========== Components ==========
    
    /** Shepherd component for herding mechanics */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Shepherd")
    class UPlayerShepherdComponent* ShepherdComponent;

    // ========== Input Handlers ==========
    
    /** Handler for attraction mode input */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnAttractionPressed();
    
    /** Handler for repulsion mode input */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnRepulsionPressed();
    
    /** Handler for neutral mode input */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnNeutralPressed();
    
    /** Handler for pickup/drop input */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnPickupPressed();
    
    /** Handler for throw charge start */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnThrowPressed();
    
    /** Handler for throw release */
    UFUNCTION(BlueprintCallable, Category = "Input")
    void OnThrowReleased();
    
public:

    /** Constructor */
    ASpaceShepherdCharacter(); 

protected:

    /** Initialize input action bindings */
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    /** Called for movement input */
    void Move(const FInputActionValue& Value);

    /** Called for looking input */
    void Look(const FInputActionValue& Value);

public:

    /** Handles move inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoMove(float Right, float Forward);

    /** Handles look inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoLook(float Yaw, float Pitch);

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpStart();

    /** Handles jump pressed inputs from either controls or UI interfaces */
    UFUNCTION(BlueprintCallable, Category="Input")
    virtual void DoJumpEnd();

public:

    /** Returns CameraBoom subobject **/
    FORCEINLINE class USpringArmComponent* GetCameraBoom() const { return CameraBoom; }

    /** Returns FollowCamera subobject **/
    FORCEINLINE class UCameraComponent* GetFollowCamera() const { return FollowCamera; }
    
    /** Returns ShepherdComponent subobject **/
    FORCEINLINE class UPlayerShepherdComponent* GetShepherdComponent() const { return ShepherdComponent; }
};