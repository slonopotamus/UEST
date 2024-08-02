#pragma once

// These asserts are just a stub

#define AssertThatMsgf(Expression, Message, ...) \
	do \
	{ \
		if (!ensureAlwaysMsgf((Expression), TEXT(Message), ##__VA_ARGS__)) \
		{ \
			return; \
		} \
	} while (false)

#define ASSERT_THAT(Expression) AssertThatMsgf(Expression, "expected expression to be true")

class UEST_API FUESTTestBase : public FAutomationTestBase
{
protected:
	FUESTTestBase(const FString& InName);

	virtual uint32 GetRequiredDeviceNum() const override;

	virtual uint32 GetTestFlags() const override;

	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override;

	virtual bool RunTest(const FString& Parameters) override;

	virtual void DoTest(const FString& Parameters)
	{
	}
};

template<typename TClass>
struct TUESTInstantiator
{
#if WITH_AUTOMATION_WORKER
	TUESTInstantiator()
	{
		Instance = MakeUnique<TClass>();
	}
	TUniquePtr<TClass> Instance;
#endif
};

#define TEST_CLASS_WITH_BASE(ClassName, BaseClass, PrettyName) \
	struct F##ClassName##Impl; \
	struct F##ClassName : public BaseClass \
	{ \
		typedef BaseClass Super; \
		F##ClassName() \
		    : Super(TEXT(#PrettyName)) \
		{ \
		} \
		/* This using is needed so Rider understands that we are a runnable test */ \
		using Super::RunTest; \
		virtual FString GetBeautifiedTestName() const override \
		{ \
			return TEXT(#PrettyName); \
		} \
		virtual FString GetTestSourceFileName() const override \
		{ \
			return TEXT(__FILE__); \
		} \
		virtual int32 GetTestSourceFileLine() const override \
		{ \
			return __LINE__; \
		} \
	}; \
	static TUESTInstantiator<F##ClassName##Impl> ClassName##Instantiator; \
	struct F##ClassName##Impl : public F##ClassName

#define TEST_WITH_BASE(TestName, BaseClass, PrettyName) \
	TEST_CLASS_WITH_BASE(TestName, BaseClass, PrettyName) \
	{ \
		virtual void DoTest(const FString& Parameters) override; \
	}; \
	void F##TestName##Impl::DoTest(const FString& Parameters)

/**
 * Simple macro for a test.
 * Usage:
 *
 * TEST(MyFancyTest)
 * {
 *     // Test body goes here
 *     ASSERT_THAT(...);
 * }
 */
// TODO: Add proper support for variadics
#define TEST(...) TEST_WITH_BASE(__VA_ARGS__, FUESTTestBase, __VA_ARGS__)

/**
 * Declares a test class
 * Usage:
 *
 * TEST_CLASS(MyFancyTestClass)
 * {
 *     TEST_METHOD(Method1)
 *     {
 *         // Test body goes here
 *         ASSERT_THAT(...);
 *     }
 *
 *     TEST_METHOD(Method2)
 *     {
 *         // Test body goes here
 *         ASSERT_THAT(...);
 *     }
 *
 *     // You can put helper fields or methods here
 * }
 */
// TODO: Add proper support for variadics
#define TEST_CLASS(...) TEST_CLASS_WITH_BASE(__VA_ARGS__, FUESTTestBase, __VA_ARGS__)

// TODO: Add proper support for variadics
// TODO: This is just a stub, we need to register method in test class
#define TEST_METHOD(MethodName) void MethodName()
