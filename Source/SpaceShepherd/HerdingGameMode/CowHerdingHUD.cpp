// CowHerdingHUD.cpp
#include "CowHerdingHUD.h"
#include "CowHerdingGameMode.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Kismet/GameplayStatics.h"
#include "CanvasItem.h"

ACowHerdingHUD::ACowHerdingHUD()
{
    // Load default font
    static ConstructorHelpers::FObjectFinder<UFont> HUDFontObject(TEXT("/Engine/EngineFonts/RobotoDistanceField"));
    if (HUDFontObject.Succeeded())
    {
        HUDFont = HUDFontObject.Object;
    }
    
    CurrentTime = 0.0f;
    CurrentCowCount = 0;
    bIsGameOver = false;
    FinalScore = 0;
}

void ACowHerdingHUD::BeginPlay()
{
    Super::BeginPlay();
    
    // Get game mode reference
    GameModeRef = Cast<ACowHerdingGameMode>(UGameplayStatics::GetGameMode(GetWorld()));
    
    if (GameModeRef)
    {
        // Bind to game mode events
        GameModeRef->OnTimeUpdated.AddDynamic(this, &ACowHerdingHUD::OnTimeUpdated);
        GameModeRef->OnCowCountChanged.AddDynamic(this, &ACowHerdingHUD::OnCowCountChanged);
        GameModeRef->OnGameEnded.AddDynamic(this, &ACowHerdingHUD::OnGameEnded);
        
        // Get initial values
        CurrentTime = GameModeRef->GetRemainingTime();
        CurrentCowCount = GameModeRef->GetCurrentCowCount();
    }
}

void ACowHerdingHUD::DrawHUD()
{
    Super::DrawHUD();
    
    if (!Canvas || !HUDFont)
    {
        return;
    }
    
    // Draw game HUD elements
    DrawTimer();
    DrawCowCount();
    
    if (bIsGameOver)
    {
        DrawGameOver();
    }
}

void ACowHerdingHUD::OnTimeUpdated(float RemainingTime)
{
    CurrentTime = RemainingTime;
}

void ACowHerdingHUD::OnCowCountChanged(int32 CowCount)
{
    CurrentCowCount = CowCount;
}

void ACowHerdingHUD::OnGameEnded(int32 Score)
{
    bIsGameOver = true;
    FinalScore = Score;
}

void ACowHerdingHUD::DrawTimer()
{
    // Format time as MM:SS
    int32 Minutes = FMath::FloorToInt(CurrentTime / 60.0f);
    int32 Seconds = FMath::FloorToInt(CurrentTime) % 60;
    FString TimeString = FString::Printf(TEXT("Time: %02d:%02d"), Minutes, Seconds);
    
    // Draw at top center
    float X = Canvas->SizeX * 0.5f;
    float Y = Canvas->SizeY * 0.05f;
    
    // Change color when time is running out
    FLinearColor DrawColor = TimerColor;
    if (CurrentTime < 30.0f)
    {
        DrawColor = FLinearColor::Yellow;
    }
    if (CurrentTime < 10.0f)
    {
        DrawColor = FLinearColor::Red;
        // Add pulsing effect for last 10 seconds
        float PulseAlpha = (FMath::Sin(GetWorld()->GetTimeSeconds() * 5.0f) + 1.0f) * 0.5f;
        DrawColor.A = FMath::Lerp(0.5f, 1.0f, PulseAlpha);
    }
    
    DrawText(TimeString, DrawColor, X, Y, 2.0f);
}

void ACowHerdingHUD::DrawCowCount()
{
    FString CowString = FString::Printf(TEXT("Cows in Pen: %d"), CurrentCowCount);
    
    // Draw at top left
    float X = Canvas->SizeX * 0.1f;
    float Y = Canvas->SizeY * 0.1f;
    
    DrawText(CowString, CowCountColor, X, Y, 1.5f);
}

void ACowHerdingHUD::DrawGameOver()
{
    // Draw game over text
    FString GameOverString = TEXT("GAME OVER!");
    float X = Canvas->SizeX * 0.5f;
    float Y = Canvas->SizeY * 0.4f;
    DrawText(GameOverString, GameOverColor, X, Y, 3.0f);
    
    // Draw final score
    FString ScoreString = FString::Printf(TEXT("Final Score: %d Cows"), FinalScore);
    Y = Canvas->SizeY * 0.5f;
    DrawText(ScoreString, CowCountColor, X, Y, 2.0f);
    
    // Draw restart instruction
    FString RestartString = TEXT("Press R to Restart");
    Y = Canvas->SizeY * 0.6f;
    DrawText(RestartString, TimerColor, X, Y, 1.5f);
}

void ACowHerdingHUD::DrawText(const FString& Text, FLinearColor Color, float X, float Y, float Scale)
{
    if (!HUDFont)
    {
        return;
    }
    
    // Calculate text size for centering
    float TextWidth, TextHeight;
    GetTextSize(Text, TextWidth, TextHeight, HUDFont, Scale * HUDScale);
    
    // Center the text at the given position
    float DrawX = X - (TextWidth * 0.5f);
    float DrawY = Y - (TextHeight * 0.5f);
    
    // Create canvas text item
    FCanvasTextItem TextItem(FVector2D(DrawX, DrawY), FText::FromString(Text), HUDFont, Color);
    TextItem.Scale = FVector2D(Scale * HUDScale, Scale * HUDScale);
    TextItem.bCentreX = false;
    TextItem.bCentreY = false;
    TextItem.bOutlined = true;
    TextItem.OutlineColor = FLinearColor::Black;
    
    // Draw the text
    Canvas->DrawItem(TextItem);
}