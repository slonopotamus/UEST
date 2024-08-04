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
	if (TestMethods.Num())
	{
		TestMethods.GenerateKeyArray(OutBeautifiedNames);
		TestMethods.GenerateKeyArray(OutTestCommands);
	}
	else
	{
		OutBeautifiedNames.Add(GetBeautifiedTestName());
		OutTestCommands.AddDefaulted(1);
	}
}

bool FUESTTestBase::RunTest(const FString& Parameters)
{
	if (TestMethods.Num())
	{
		if (const auto* Method = TestMethods.Find(Parameters); ensure(Method))
		{
			Method->Execute();
		}
	}

	// TODO: Should we mark test class without test methods as failed?

	return true;
}

IMPLEMENT_MODULE(FDefaultModuleImpl, UEST)
