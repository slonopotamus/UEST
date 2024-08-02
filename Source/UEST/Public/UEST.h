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

// TODO: Get rid of Boost
#include <boost/preprocessor/cat.hpp>
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/seq/elem.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/stringize.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

#define UEST_CONCAT_SEQ_1(seq, op) BOOST_PP_SEQ_HEAD(seq)
#define UEST_CONCAT_SEQ_N(seq, op) BOOST_PP_SEQ_FOLD_LEFT(op, BOOST_PP_SEQ_HEAD(seq), BOOST_PP_SEQ_TAIL(seq))
#define UEST_CONCAT_SEQ(seq, op) BOOST_PP_IF(BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(seq), 1), UEST_CONCAT_SEQ_N, UEST_CONCAT_SEQ_1)(seq, op)

#define UEST_PRETTY_NAME_OP(s, state, x) BOOST_PP_CAT(state, BOOST_PP_CAT(., x))
#define UEST_PRETTY_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_PRETTY_NAME_OP)

#define UEST_CLASS_NAME_OP(s, state, x) BOOST_PP_CAT(state, BOOST_PP_CAT(_, x))
#define UEST_CLASS_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_CLASS_NAME_OP)

#define TEST_CLASS_WITH_BASE_IMPL(BaseClass, ClassName, PrettyName) \
	struct BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)); \
	struct BOOST_PP_CAT(F, ClassName) : public BaseClass \
	{ \
		typedef BaseClass Super; \
		BOOST_PP_CAT(F, ClassName)() \
		    : Super(UE_MODULE_NAME "." BOOST_PP_STRINGIZE(PrettyName)) \
		{ \
		} \
		/* This using is needed so Rider understands that we are a runnable test */ \
		using Super::RunTest; \
		virtual FString GetBeautifiedTestName() const override \
		{ \
			return UE_MODULE_NAME "." BOOST_PP_STRINGIZE(PrettyName); \
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
	static TUESTInstantiator<BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl))> BOOST_PP_CAT(ClassName, Instantiator); \
	struct BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)) : public BOOST_PP_CAT(F, ClassName)

#define TEST_CLASS_WITH_BASE(BaseClass, ...) TEST_CLASS_WITH_BASE_IMPL(BaseClass, UEST_CLASS_NAME(__VA_ARGS__), UEST_PRETTY_NAME(__VA_ARGS__))

#define TEST_WITH_BASE(BaseClass, ...) \
	TEST_CLASS_WITH_BASE(BaseClass, __VA_ARGS__) \
	{ \
		virtual void DoTest(const FString& Parameters) override; \
	}; \
	void BOOST_PP_CAT(BOOST_PP_CAT(F, UEST_CLASS_NAME(__VA_ARGS__)), Impl)::DoTest(const FString& Parameters)

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
#define TEST(...) TEST_WITH_BASE(FUESTTestBase, __VA_ARGS__)

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
#define TEST_CLASS(...) TEST_CLASS_WITH_BASE(FUESTTestBase, __VA_ARGS__)

// TODO: This is just a stub, we need to register method in test class
#define TEST_METHOD(MethodName) void MethodName()
