#pragma once

#if WITH_UNREAL_DEVELOPER_TOOLS
#include "Assert/CQTestConvert.h"
#endif

#include "Misc/AutomationTest.h"

namespace UEST
{
	template<typename T>
	static FString ToString(const T& Value)
	{
#if WITH_UNREAL_DEVELOPER_TOOLS
		return CQTestConvert::ToString(Value);
#else
		return TEXT("UNKNOWN");
#endif
	}

	template<typename M, typename... P>
	concept Matcher = requires(M const m, P... p) {
		{ M{p...} };
		// TODO: Can we require that there exists such template? template<typename T> bool M::Matches(const<T>&)
		//{ m.Matches(t) } -> std::same_as<bool>;
		{ m.Describe() } -> std::same_as<FString>;
	};

	namespace Matchers
	{
		struct Null
		{
			template<typename T>
			    requires std::is_pointer_v<T> || std::is_null_pointer_v<T>
			bool Matches(const T& Value) const
			{
				return Value == nullptr;
			}

			FString Describe() const
			{
				return TEXT("be nullptr");
			}
		};

		struct True
		{
			template<typename T>
			    requires requires(const T t) { { static_cast<bool>(t) }; }
			bool Matches(const T& Value) const
			{
				return Value;
			}

			FString Describe() const
			{
				return TEXT("be true");
			}
		};

		struct False
		{
			template<typename T>
			    requires requires(const T t) { { static_cast<bool>(t) }; }
			bool Matches(const T& Value) const
			{
				return !Value;
			}

			FString Describe() const
			{
				return TEXT("be false");
			}
		};

		template<typename P>
		struct EqualTo final
		{
			const P Expected;

			explicit EqualTo(P Expected)
			    : Expected(MoveTemp(Expected))
			{
			}

			template<typename T>
			    requires requires(const T t, const P p) {
				    {
					    t == p
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value == Expected;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be equal to %s"), *ToString(Expected));
			}
		};

		template<typename P>
		struct LessThan final
		{
			const P Expected;

			explicit LessThan(P Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			template<typename T>
			    requires requires(const T t, const P p) {
				    {
					    t < p
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value < Expected;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be less than %s"), *ToString(Expected));
			}
		};

		template<typename P>
		struct LessThanOrEqualTo final
		{
			const P Expected;

			explicit LessThanOrEqualTo(P Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			template<typename T>
			    requires requires(const T t, const P p) {
				    {
					    t > p
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value <= Expected;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be less than or equal to %s"), *ToString(Expected));
			}
		};

		template<typename P>
		struct GreaterThan final
		{
			const P Expected;

			explicit GreaterThan(P Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			template<typename T>
			    requires requires(const T t, const P p) {
				    {
					    t > p
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value > Expected;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be greater than %s"), *ToString(Expected));
			}
		};

		template<typename P>
		struct GreaterThanOrEqualTo final
		{
			const P Expected;

			explicit GreaterThanOrEqualTo(P Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			template<typename T>
			    requires requires(const T t, const P p) {
				    {
					    t >= p
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value >= Expected;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be greater than or equal to %s"), *ToString(Expected));
			}
		};

		template<typename Lower, typename Upper = Lower>
		struct InRange final
		{
			const Lower From;
			const Upper To;

			explicit InRange(Lower From, Upper To)
			    : From{MoveTemp(From)}
			    , To{MoveTemp(To)}
			{
			}

			template<typename T>
			    requires requires(const T t, const Lower lower, const Upper upper) {
				    {
					    t >= lower
				    };
				    {
					    t <= upper
				    };
			    }
			bool Matches(const T& Value) const
			{
				if (!ensureAlwaysMsgf(From <= To, TEXT("Invalid range, %s is greater than %s"), *CQTestConvert::ToString(From), *CQTestConvert::ToString(To)))
				{
					return false;
				}

				return Value >= From && Value <= To;
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be in range from %s to %s"), *ToString(From), *ToString(To));
			}
		};

		template<typename M, typename... P>
		    requires Matcher<M, P...>
		struct Not final
		{
			M Nested;

			explicit Not(P... Args)
			    : Nested{MoveTemp(Args)...}
			{
			}

			explicit Not(M&& Nested)
			    : Nested{Nested}
			{
			}

			template<typename T>
			bool Matches(const T& Value) const
			{
				return !Nested.template Matches<T>(Value);
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("not %s"), *Nested.Describe());
			}
		};
	} // namespace Matchers
} // namespace UEST

namespace Is
{
	constexpr auto Null = UEST::Matchers::Null{};
	constexpr auto True = UEST::Matchers::True{};
	constexpr auto False = UEST::Matchers::False{};

	template<typename T>
	using EqualTo = UEST::Matchers::EqualTo<T>;

	template<typename T>
	using LessThan = UEST::Matchers::LessThan<T>;

	template<typename T>
	using LessThanOrEqualTo = UEST::Matchers::LessThanOrEqualTo<T>;

	template<typename T>
	using AtMost = LessThanOrEqualTo<T>;

	template<typename T>
	using GreaterThan = UEST::Matchers::GreaterThan<T>;

	template<typename T>
	using GreaterThanOrEqualTo = UEST::Matchers::GreaterThanOrEqualTo<T>;

	template<typename T>
	using AtLeast = GreaterThanOrEqualTo<T>;

	const auto Zero = EqualTo<int64>(0);
	const auto Positive = GreaterThan<int64>(0);
	const auto Negative = LessThan<int64>(0);

	template<typename T>
	using InRange = UEST::Matchers::InRange<T>;

	namespace Not
	{
		const auto Null = UEST::Matchers::Not<UEST::Matchers::Null>{};
		constexpr auto False = Is::True;
		constexpr auto True = Is::False;

		template<typename T>
		using EqualTo = UEST::Matchers::Not<UEST::Matchers::EqualTo<T>, T>;

		template<typename T>
		using LessThan = UEST::Matchers::GreaterThanOrEqualTo<T>;

		template<typename T>
		using LessThanOrEqualTo = UEST::Matchers::GreaterThan<T>;

		template<typename T>
		using GreaterThan = UEST::Matchers::LessThanOrEqualTo<T>;

		template<typename T>
		using GreaterThanOrEqualTo = UEST::Matchers::LessThan<T>;

		const auto Zero = EqualTo<int64>(0);

		const auto Positive = GreaterThan<int64>(0);

		const auto Negative = LessThan<int64>(0);

		template<typename T>
		using InRange = UEST::Matchers::Not<UEST::Matchers::InRange<T>, T, T>;
	} // namespace Not
} // namespace Is

// TODO: Provide ASSERT_THAT(Value) variant that tests that Value is true
#define ASSERT_THAT(Value, Matcher) \
	do \
	{ \
		const auto& _M = Matcher; \
		if (!ensureAlwaysMsgf(_M.Matches<decltype(Value)>(Value), TEXT("%s: %s must %s"), TEXT(#Value), *CQTestConvert::ToString(Value), *_M.Describe())) \
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
	struct BOOST_PP_CAT(F, ClassName) \
	    : public BaseClass \
	{ \
		typedef BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)) ThisClass; \
		typedef BaseClass Super; \
		BOOST_PP_CAT(F, ClassName) \
		() \
		    : Super(TEXT(PrettyName)) \
		{ \
		} \
		/* This using is needed so Rider understands that we are a runnable test */ \
		using Super::RunTest; \
		virtual FString GetBeautifiedTestName() const override \
		{ \
			return TEXT(PrettyName); \
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
	static const TUESTInstantiator<BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl))> BOOST_PP_CAT(ClassName, Instantiator); \
	struct BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)) \
	    : public BOOST_PP_CAT(F, ClassName)

#define TEST_CLASS_WITH_BASE(BaseClass, ...) TEST_CLASS_WITH_BASE_IMPL(BaseClass, UEST_CLASS_NAME(__VA_ARGS__), UEST_PRETTY_NAME(__VA_ARGS__))

#define TEST_WITH_BASE(BaseClass, ...) \
	TEST_CLASS_WITH_BASE(BaseClass, __VA_ARGS__) \
	{ \
		virtual bool RunTest(const FString& Parameters) override \
		{ \
			DoTest(Parameters); \
			return true; \
		} \
		void DoTest(const FString& Parameters); \
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

#define TEST_METHOD(MethodName) \
	FUESTMethodRegistrar reg##MethodName{TEXT(#MethodName), *this, FSimpleDelegate::CreateRaw(this, &ThisClass::MethodName)}; \
	void MethodName()
