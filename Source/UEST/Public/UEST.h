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

	template<typename M, typename T, typename... P>
	concept Matcher = requires(M const m, const T& t, P... p) {
		{ M{p...} };
		{ m.Matches(t) } -> std::same_as<bool>;
		{ m.Describe() } -> std::same_as<FString>;
	};

	template<typename M, typename T, typename... P>
	    requires Matcher<M, T, P...>
	struct Passthrough
	{
		M Matcher;

		explicit Passthrough(P... Args)
		    : Matcher{MoveTemp(Args)...}
		{
		}

		template<typename>
		auto operator()() const
		{
			// TODO: Can we avoid copying here?
			return Matcher;
		}
	};

	namespace Matchers
	{
		static constexpr struct Null
		{
			template<typename T>
			    requires std::is_pointer_v<T> || std::is_null_pointer_v<T>
			struct Matcher final
			{
				bool Matches(const T& Value) const
				{
					return Value == nullptr;
				}

				FString Describe() const
				{
					return TEXT("be nullptr");
				}
			};

			template<typename T>
			auto operator()() const
			{
				return Matcher<T>{};
			}
		} Null;

		static constexpr struct True
		{
			template<typename T>
			// TODO: Maybe we instead want to accept anything with operator bool()?
			    requires std::same_as<T, bool>
			struct Matcher final
			{
				bool Matches(const T& Value) const
				{
					return Value;
				}

				FString Describe() const
				{
					return TEXT("be true");
				}
			};

			template<typename T>
			auto operator()() const
			{
				return Matcher<T>{};
			}
		} True;

		static constexpr struct False
		{
			template<typename T>
			// TODO: Maybe we instead want to accept anything with operator bool()?
			    requires std::same_as<T, bool>
			struct Matcher final
			{
				bool Matches(const T& Value) const
				{
					return !Value;
				}

				FString Describe() const
				{
					return TEXT("be false");
				}
			};

			template<typename T>
			auto operator()() const
			{
				return Matcher<T>{};
			}
		} False;

		template<typename T, typename P = T>
		// TODO: Add requires
		struct EqualTo final
		{
			const T Expected;

			explicit EqualTo(T Expected)
			    : Expected(MoveTemp(Expected))
			{
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

		template<typename T, typename P = T>
		// TODO: Add requires
		struct LessThan final
		{
			const T Expected;

			explicit LessThan(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
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

		template<typename T, typename P = T>
		// TODO: Add requires
		struct LessThanOrEqualTo final
		{
			const T Expected;

			explicit LessThanOrEqualTo(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
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

		template<typename T, typename P = T>
		// TODO: Add requires
		struct GreaterThan final
		{
			const T Expected;

			explicit GreaterThan(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
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

		template<typename T, typename P = T>
		// TODO: Add requires
		struct GreaterThanOrEqualTo final
		{
			const T Expected;

			explicit GreaterThanOrEqualTo(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
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

		template<typename T, typename P1 = T, typename P2 = T>
		// TODO: Add requires
		struct InRange final
		{
			const P1 From;
			const P2 To;

			explicit InRange(P1 From, P2 To)
			    : From{MoveTemp(From)}
			    , To{MoveTemp(To)}
			{
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

		template<template<typename...> typename M, typename T, typename... P>
		    requires Matcher<M<T, P...>, T, P...>
		struct Not
		{
			M<T, P...> Nested;

			explicit Not(P... Args)
			    : Nested{MoveTemp(Args)...}
			{
			}

			explicit Not(M<T, P...>&& Nested)
			    : Nested{Nested}
			{
			}

			bool Matches(const T& Value) const
			{
				return !Nested.Matches(Value);
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("not %s"), *Nested.Describe());
			}
		};

		using Bla = Not<EqualTo, int32, int32>;
	} // namespace Matchers
} // namespace UEST

namespace Is
{
	constexpr const auto& Null = UEST::Matchers::Null;
	constexpr const auto& True = UEST::Matchers::True;
	constexpr const auto& False = UEST::Matchers::False;

	template<typename T>
	using EqualTo = UEST::Passthrough<UEST::Matchers::EqualTo<T>, T, T>;

	template<typename T>
	using LessThan = UEST::Passthrough<UEST::Matchers::LessThan<T>, T, T>;

	template<typename T>
	using LessThanOrEqualTo = UEST::Passthrough<UEST::Matchers::LessThanOrEqualTo<T>, T>;

	template<typename T>
	using AtMost = LessThanOrEqualTo<T>;

	template<typename T>
	using GreaterThan = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T, T>;

	template<typename T>
	using GreaterThanOrEqualTo = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqualTo<T>, T>;

	template<typename T>
	using AtLeast = GreaterThanOrEqualTo<T>;

	const auto Zero = EqualTo<int64>(0);

	const auto Positive = GreaterThan<int64>(0);

	const auto Negative = LessThan<int64>(0);

	template<typename T>
	using InRange = UEST::Passthrough<UEST::Matchers::InRange<T>, T, T, T>;

	namespace Not
	{
		static struct
		{
			template<typename T>
			auto operator()() const
			{
				return UEST::Matchers::Not<UEST::Matchers::Null::Matcher, T>{};
			}
		} Null;

		static struct
		{
			template<typename T>
			auto operator()() const
			{
				return UEST::Matchers::False::Matcher<T>{};
			}
		} True;

		static struct
		{
			template<typename T>
			auto operator()() const
			{
				return UEST::Matchers::True::Matcher<T>{};
			}
		} False;

		template<typename T>
		using EqualTo = UEST::Passthrough<UEST::Matchers::Not<UEST::Matchers::EqualTo, T, T>, T, T>;

		template<typename T>
		using LessThan = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqualTo<T>, T, T>;

		template<typename T>
		using LessThanOrEqualTo = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T>;

		template<typename T>
		using GreaterThan = UEST::Passthrough<UEST::Matchers::LessThanOrEqualTo<T>, T, T>;

		template<typename T>
		using GreaterThanOrEqualTo = UEST::Passthrough<UEST::Matchers::LessThan<T>, T, T>;

		const auto Zero = EqualTo<int64>(0);

		const auto Positive = GreaterThan<int64>(0);

		const auto Negative = LessThan<int64>(0);

		template<typename T>
		using InRange = UEST::Passthrough<UEST::Matchers::Not<UEST::Matchers::InRange, T, T>, T, T>;
	} // namespace Not
} // namespace Is

// TODO: Provide ASSERT_THAT(Value) variant that tests that Value is true
#define ASSERT_THAT(Value, M) \
	do \
	{ \
		const auto& MatcherInstance = M.operator()<decltype(Value)>(); \
		if (!ensureAlwaysMsgf(MatcherInstance.Matches(Value), TEXT("%s: %s must %s"), TEXT(#Value), *CQTestConvert::ToString(Value), *MatcherInstance.Describe())) \
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
