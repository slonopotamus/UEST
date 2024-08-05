#pragma once

#include "Assert/CQTestConvert.h"
#include "Misc/AutomationTest.h"

namespace UEST
{
	template<typename T>
	// TODO: Make this FNoncopyable?
	struct IMatcher
	{
		virtual ~IMatcher() = default;

		virtual bool Matches(const T& Value) const = 0;

		virtual FString Describe() const = 0;
	};

	// TODO: Can we get rid of <P> using concept for M?
	template<typename M, typename... P>
	// TODO: Add requires
	struct Passthrough
	{
		M Matcher;

		explicit Passthrough(P... Args)
		    : Matcher{MoveTemp(Args)...}
		{
		}

		template<typename U>
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
			struct Matcher final : IMatcher<T>
			{
				virtual bool Matches(const T& Value) const override
				{
					return Value == nullptr;
				}

				virtual FString Describe() const override
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
			struct Matcher final : IMatcher<T>
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
			struct Matcher final : IMatcher<T>
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

			template<typename T>
			auto operator()() const
			{
				return Matcher<T>{};
			}
		} False;

		template<typename T>
		// TODO: Add requires
		struct EqualTo final : IMatcher<T>
		{
			const T Expected;

			explicit EqualTo(T Expected)
			    : Expected(MoveTemp(Expected))
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				return Value == Expected;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be equal to %s"), *CQTestConvert::ToString(Expected));
			}
		};

		template<typename T>
		// TODO: Add requires
		struct LessThan final : IMatcher<T>
		{
			const T Expected;

			explicit LessThan(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				return Value < Expected;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be less than %s"), *CQTestConvert::ToString(Expected));
			}
		};

		template<typename T>
		// TODO: Add requires
		struct LessThanOrEqualTo final : IMatcher<T>
		{
			const T Expected;

			explicit LessThanOrEqualTo(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				return Value <= Expected;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be less than or equal to %s"), *CQTestConvert::ToString(Expected));
			}
		};

		template<typename T>
		// TODO: Add requires
		struct GreaterThan final : IMatcher<T>
		{
			const T Expected;

			explicit GreaterThan(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				return Value > Expected;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be greater than %s"), *CQTestConvert::ToString(Expected));
			}
		};

		template<typename T>
		// TODO: Add requires
		struct GreaterThanOrEqualTo final : IMatcher<T>
		{
			const T Expected;

			explicit GreaterThanOrEqualTo(T Expected)
			    : Expected{MoveTemp(Expected)}
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				return Value >= Expected;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be greater than or equal to %s"), *CQTestConvert::ToString(Expected));
			}
		};

		template<typename T>
		// TODO: Add requires
		struct InRange final : IMatcher<T>
		{
			const T From;
			const T To;

			explicit InRange(T From, T To)
			    : From{MoveTemp(From)}
			    , To{MoveTemp(To)}
			{
			}

			virtual bool Matches(const T& Value) const override
			{
				if (!ensureAlwaysMsgf(From <= To, TEXT("Invalid range, %s is greater than %s"), *CQTestConvert::ToString(From), *CQTestConvert::ToString(To)))
				{
					return false;
				}

				return Value >= From && Value <= To;
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("be in range from %s to %s"), *CQTestConvert::ToString(From), *CQTestConvert::ToString(To));
			}
		};

		// TODO: Can we get rid of <T> and <P> using concept for <M>?
		template<typename M, typename T, typename... P>
		// TODO: Add requires
		struct Not : IMatcher<T>
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

			virtual bool Matches(const T& Value) const override
			{
				return !Nested.Matches(Value);
			}

			virtual FString Describe() const override
			{
				return FString::Printf(TEXT("not %s"), *Nested.Describe());
			}
		};
	} // namespace Matchers
} // namespace UEST

namespace Is
{
	constexpr const auto& Null = UEST::Matchers::Null;
	constexpr const auto& True = UEST::Matchers::True;
	constexpr const auto& False = UEST::Matchers::False;

	template<typename T>
	using EqualTo = UEST::Passthrough<UEST::Matchers::EqualTo<T>, T>;

	template<typename T>
	using LessThan = UEST::Passthrough<UEST::Matchers::LessThan<T>, T>;

	template<typename T>
	using LessThanOrEqualTo = UEST::Passthrough<UEST::Matchers::LessThanOrEqualTo<T>, T>;

	template<typename T>
	using AtMost = LessThanOrEqualTo<T>;

	template<typename T>
	using GreaterThan = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T>;

	template<typename T>
	using GreaterThanOrEqualTo = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqualTo<T>, T>;

	template<typename T>
	using AtLeast = GreaterThanOrEqualTo<T>;

	const auto Zero = EqualTo<int64>(0);

	const auto Positive = GreaterThan<int64>(0);

	const auto Negative = LessThan<int64>(0);

	template<typename T>
	using InRange = UEST::Passthrough<UEST::Matchers::InRange<T>, T, T>;

	namespace Not
	{
		static struct
		{
			template<typename T>
			auto operator()() const
			{
				return UEST::Matchers::Not<UEST::Matchers::Null::Matcher<T>, T>{};
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
		using EqualTo = UEST::Passthrough<UEST::Matchers::Not<UEST::Matchers::EqualTo<T>, T, T>, T>;

		template<typename T>
		using LessThan = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqualTo<T>, T>;

		template<typename T>
		using LessThanOrEqualTo = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T>;

		template<typename T>
		using GreaterThan = UEST::Passthrough<UEST::Matchers::LessThanOrEqualTo<T>, T>;

		template<typename T>
		using GreaterThanOrEqualTo = UEST::Passthrough<UEST::Matchers::LessThan<T>, T>;

		const auto Zero = EqualTo<int64>(0);

		const auto Positive = GreaterThan<int64>(0);

		const auto Negative = LessThan<int64>(0);

		template<typename T>
		using InRange = UEST::Passthrough<UEST::Matchers::Not<UEST::Matchers::InRange<T>, T, T>, T, T>;
	} // namespace Not
} // namespace Is

// TODO: Provide ASSERT_THAT(Value) variant that tests that Value is true
#define ASSERT_THAT(Value, Matcher) \
	do \
	{ \
		const auto& MatcherInstance = Matcher.operator()<decltype(Value)>(); \
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
