#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "TheNightfallSiegeInstance.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPortalLocationSelectionDoesNotConsumeCandidatesTest,
    "TheNightfallSiege.Portal.LocationSelection.DoesNotConsumeCandidates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPortalLocationSelectionDoesNotConsumeCandidatesTest::RunTest(const FString& Parameters)
{
    UTheNightfallSiegeInstance* GameInstance = NewObject<UTheNightfallSiegeInstance>();
    if (!TestNotNull(TEXT("Game instance should be created"), GameInstance))
    {
        return false;
    }

    GameInstance->StartRaid();
    constexpr int32 ExpectedLocationCount = 6;
    TestEqual(
        TEXT("StartRaid should configure all portal locations"),
        GameInstance->DungeonPortalLocations.Num(),
        ExpectedLocationCount);

    for (int32 SelectionIndex = 0; SelectionIndex < 100; ++SelectionIndex)
    {
        FVector SelectedLocation = FVector::ZeroVector;
        if (!TestTrue(
                FString::Printf(TEXT("Selection %d should succeed"), SelectionIndex + 1),
                GameInstance->SelectNextDungeonPortalLocation(SelectedLocation)))
        {
            return false;
        }

        TestTrue(
            FString::Printf(TEXT("Selection %d should use a configured location"), SelectionIndex + 1),
            GameInstance->DungeonPortalLocations.Contains(SelectedLocation));
    }

    TestEqual(
        TEXT("Repeated selection should not consume portal locations"),
        GameInstance->DungeonPortalLocations.Num(),
        ExpectedLocationCount);
    return true;
}

#endif
