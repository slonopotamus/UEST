#include "UEST.h"

#include "Modules/ModuleManager.h"

FUESTTestBase::FUESTTestBase(const FString& InName)
    : FAutomationTestBase(InName, false)
{
}

uint32 FUESTTestBase::GetRequiredDeviceNum() const
{
	return 1;
}

uint32 FUESTTestBase::GetTestFlags() const
{
	return EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter;
}

void FUESTTestBase::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const
{
	OutBeautifiedNames.Add(GetBeautifiedTestName());
	OutTestCommands.Add(FString());
}

bool FUESTTestBase::RunTest(const FString& Parameters)
{
	DoTest(Parameters);
	return true;
}

IMPLEMENT_MODULE(FDefaultModuleImpl, UEST)
