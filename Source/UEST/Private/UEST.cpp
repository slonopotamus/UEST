#include "UEST.h"

#include "Modules/ModuleManager.h"

FUESTTestBase::FUESTTestBase(const FString& InName, bool bIsComplex)
    : FAutomationTestBase(InName, bIsComplex)
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

static FString ExtractTestMethod(const FString& InTestName)
{
	auto Result = InTestName;
	const auto Pos = Result.Find(TEXT(" "));

	if (Pos != INDEX_NONE)
	{
		Result.RightChopInline(Pos + 1);
	}

	return Result;
}

FString FUESTTestBase::GetTestSourceFileName(const FString& InTestName) const
{
	if (TestMethods.Num())
	{
		const auto& TestMethod = ExtractTestMethod(InTestName);
		if (const auto* TestInfo = TestMethods.Find(TestMethod); ensure(TestInfo))
		{
			return TestInfo->FileName;
		}
	}

	return Super::GetTestSourceFileName(InTestName);
}

int32 FUESTTestBase::GetTestSourceFileLine(const FString& InTestName) const
{
	if (TestMethods.Num())
	{
		const auto& TestMethod = ExtractTestMethod(InTestName);
		if (const auto* TestInfo = TestMethods.Find(TestMethod); ensure(TestInfo))
		{
			return TestInfo->FileLine;
		}
	}

	return Super::GetTestSourceFileLine(InTestName);
}

bool FUESTTestBase::RunTest(const FString& InTestName)
{
	if (TestMethods.Num())
	{
		if (const auto* TestInfo = TestMethods.Find(InTestName); ensure(TestInfo))
		{
			// TODO: Add things like BEFORE_EACH() {...}, AFTER_EACH() {...}?
			TestInfo->Delegate.Execute();
		}
	}

	// TODO: Should we mark test class without test methods as failed?

	return true;
}

IMPLEMENT_MODULE(FDefaultModuleImpl, UEST)
