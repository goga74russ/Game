#include "FNInventory.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFNInventoryAssignmentTest, "FadedNav.Inventory.Assignment",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFNInventoryAssignmentTest::RunTest(const FString& Parameters)
{
	FFNInventorySnapshot State;
	State.OwnedSkills = { 0, 1, 2, 3 };
	State.Panel = { 0, 1, 2 };
	TArray<int32> Result = { 9 };
	FString Error;
	TestFalse(TEXT("Cannot edit away from a treba"), FNInventory::PlanAssignment(State, 3, 0, Result, Error));
	TestEqual(TEXT("Rejected transaction leaves output untouched"), Result[0], 9);
	State.bCanEdit = true;
	TestTrue(TEXT("Replace with owned gem"), FNInventory::PlanAssignment(State, 3, 0, Result, Error));
	TestTrue(TEXT("Only the requested slot changes"), Result == TArray<int32>({ 3, 1, 2 }));
	TestTrue(TEXT("Moving equipped gem swaps slots"), FNInventory::PlanAssignment(State, 0, 2, Result, Error));
	TestTrue(TEXT("No duplicate or lost displaced gem"), Result == TArray<int32>({ 2, 1, 0 }));
	TestTrue(TEXT("Same-slot assignment is idempotent"), FNInventory::PlanAssignment(State, 1, 1, Result, Error));
	TestTrue(TEXT("Same-slot panel unchanged"), Result == State.Panel);
	TestFalse(TEXT("Cannot equip an undiscovered gem"), FNInventory::PlanAssignment(State, 4, 0, Result, Error));
	TestFalse(TEXT("Cannot use ultimate slot in Yav"), FNInventory::PlanAssignment(State, 3, 3, Result, Error));
	TestFalse(TEXT("Reject negative slot"), FNInventory::PlanAssignment(State, 3, -1, Result, Error));
	TestFalse(TEXT("Reject unknown skill id"), FNInventory::PlanAssignment(State, 99, 0, Result, Error));
	TestTrue(TEXT("Model supports clearing a slot"), FNInventory::PlanAssignment(State, -1, 0, Result, Error));
	TestTrue(TEXT("Clearing does not destroy owned gems"), Result == TArray<int32>({ -1, 1, 2 }) && State.OwnedSkills.Num() == 4);
	State.Panel = { 0, 0, 2 };
	TestFalse(TEXT("Reject corrupt duplicate panel"), FNInventory::PlanAssignment(State, 3, 0, Result, Error));
	State.Panel = { 0, 4, 2 };
	TestFalse(TEXT("Reject panel with unowned gem"), FNInventory::PlanAssignment(State, 3, 0, Result, Error));
	State.Panel = { 0, 1 };
	TestFalse(TEXT("Reject truncated panel"), FNInventory::PlanAssignment(State, 3, 0, Result, Error));
	return true;
}
#endif
