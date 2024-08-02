#pragma once

#include "Assert/CQTestConvert.h"
#include "Misc/AutomationTest.h"

template<typename T>
struct IMatcher
{
	virtual ~IMatcher() = default;

	virtual bool Matches(const T& Value) const = 0;

	virtual FString Describe() const = 0;
};

namespace Is
{
	template<typename T>
	requires std::is_pointer_v<T>
	struct Null final : IMatcher<T>
	{
		virtual bool Matches(const T& Value) const override
		{
			return Value != nullptr;
		}

		virtual FString Describe() const override
		{
			return TEXT("be nullptr");
		}
	};

	template<typename T, typename M>
	// TODO: Add requires
	struct NotMatcher final : IMatcher<T>
	{
		M Matcher;

		virtual bool Matches(const T& Value) const override
		{
			return !Matcher.Matches(Value);
		}

		virtual FString Describe() const override
		{
			return FString::Printf(TEXT("not %s"), *Matcher.Describe());
		}
	};

	template<typename T>
	requires std::same_as<T, bool>
	struct True final : IMatcher<T>
	{
		virtual bool Matches(const T& Value) const override
		{
			return Value;
		}

		virtual FString Describe() const override
		{
			return TEXT("be true");
		}
	};

	template<typename T>
	requires std::same_as<T, bool>
	struct False final : IMatcher<T>
	{
		virtual bool Matches(const T& Value) const override
		{
			return !Value;
		}

		virtual FString Describe() const override
		{
			return TEXT("be false");
		}
	};

	namespace Not
	{
		template<typename T>
		using Null = NotMatcher<T, Null<T>>;

		template<typename T>
		using True = False<T>;

		template<typename T>
		using False = True<T>;
	}
}

#define ASSERT_THAT(Value, Matcher) \
	do \
	{ \
		Matcher<decltype(Value)> _Matcher; \
		if (!ensureAlwaysMsgf(_Matcher.Matches(Value), TEXT("%s must %s"), *CQTestConvert::ToString(Value), *_Matcher.Describe())) \
		{ \
			return; \
		} \
	} while (false)

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

public:
	// TODO: Can we do this without delegates, just using method pointers?
	TMap<FString, FSimpleDelegate> TestMethods;
};

struct FUESTMethodRegistrar
{
	FUESTMethodRegistrar(FString Name, FUESTTestBase& Test, FSimpleDelegate Delegate)
	{
		Test.TestMethods.Add(MoveTemp(Name), MoveTemp(Delegate));
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

#define UEST_CONCAT_SEQ_1(seq, fold_op, elem_op) elem_op(BOOST_PP_SEQ_HEAD(seq))
#define UEST_CONCAT_SEQ_N(seq, fold_op, elem_op) BOOST_PP_SEQ_FOLD_LEFT(fold_op, elem_op(BOOST_PP_SEQ_HEAD(seq)), BOOST_PP_SEQ_TAIL(seq))
#define UEST_CONCAT_SEQ(seq, fold_op, elem_op) BOOST_PP_IF(BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(seq), 1), UEST_CONCAT_SEQ_N, UEST_CONCAT_SEQ_1)(seq, fold_op, elem_op)

#define UEST_PRETTY_NAME_ELEM_OP(x) BOOST_PP_STRINGIZE(x)
#define UEST_PRETTY_NAME_FOLD_OP(s, state, x) state "." UEST_PRETTY_NAME_ELEM_OP(x)
#define UEST_PRETTY_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_PRETTY_NAME_FOLD_OP, UEST_PRETTY_NAME_ELEM_OP)

#define UEST_CLASS_NAME_ELEM_OP(x) x
#define UEST_CLASS_NAME_FOLD_OP(s, state, x) BOOST_PP_CAT(state, BOOST_PP_CAT(_, x))
#define UEST_CLASS_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_CLASS_NAME_FOLD_OP, UEST_CLASS_NAME_ELEM_OP)

#define TEST_CLASS_WITH_BASE_IMPL(BaseClass, ClassName, PrettyName) \
	struct BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)); \
	struct BOOST_PP_CAT(F, ClassName) : public BaseClass \
	{ \
		typedef BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)) ThisClass; \
		typedef BaseClass Super; \
		BOOST_PP_CAT(F, ClassName)() \
		    : Super(TEXT(UE_MODULE_NAME "." PrettyName)) \
		{ \
		} \
		/* This using is needed so Rider understands that we are a runnable test */ \
		using Super::RunTest; \
		virtual FString GetBeautifiedTestName() const override \
		{ \
			return TEXT(UE_MODULE_NAME "." PrettyName); \
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

#define TEST_METHOD(MethodName) FUESTMethodRegistrar reg##MethodName{TEXT(#MethodName), *this, FSimpleDelegate::CreateRaw(this, &ThisClass::MethodName)}; \
		void MethodName()
