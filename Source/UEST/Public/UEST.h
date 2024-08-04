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

namespace UEST
{
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
			const T& Expected;

			EqualTo(const T& Expected)
			    : Expected(Expected)
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
			const T& Expected;

			LessThan(const T& Expected)
			    : Expected{Expected}
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
		struct LessThanOrEqual final : IMatcher<T>
		{
			const T& Expected;

			LessThanOrEqual(const T& Expected)
			    : Expected{Expected}
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
			const T& Expected;

			GreaterThan(const T& Expected)
			    : Expected{Expected}
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
		struct GreaterThanOrEqual final : IMatcher<T>
		{
			const T& Expected;

			GreaterThanOrEqual(const T& Expected)
			    : Expected{Expected}
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

		// TODO: Can we get rid of <T> using concepts?
		template<typename T, typename M>
		// TODO: Add requires
		struct Not : IMatcher<T>
		{
			M Nested;

			Not() = default;

			Not(M&& Nested)
			    : Nested{MoveTemp(Nested)}
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

	template<typename M, typename... P>
	// TODO: Add requires
	struct Passthrough
	{
		M Matcher;

		explicit Passthrough(const P&... Args)
		    : Matcher{Args...}
		{
		}

		template<typename U>
		auto operator()() const
		{
			return Matcher;
		}
	};
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
	using LessThanOrEqual = UEST::Passthrough<UEST::Matchers::LessThanOrEqual<T>, T>;

	template<typename T>
	using GreaterThan = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T>;

	template<typename T>
	using GreaterThanOrEqual = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqual<T>, T>;

	namespace Not
	{
		static struct
		{
			template<typename T>
			auto operator()() const
			{
				return UEST::Matchers::Not<T, UEST::Matchers::Null::Matcher<T>>{};
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
		using EqualTo = UEST::Passthrough<UEST::Matchers::Not<T, UEST::Matchers::EqualTo<T>>, T>;

		template<typename T>
		using LessThan = UEST::Passthrough<UEST::Matchers::GreaterThanOrEqual<T>, T>;

		template<typename T>
		using LessThanOrEqual = UEST::Passthrough<UEST::Matchers::GreaterThan<T>, T>;

		template<typename T>
		using GreaterThan = UEST::Passthrough<UEST::Matchers::LessThanOrEqual<T>, T>;

		template<typename T>
		using GreaterThanOrEqual = UEST::Passthrough<UEST::Matchers::LessThan<T>, T>;
	} // namespace Not
} // namespace Is

#define ASSERT_THAT(Value, Matcher) \
	do \
	{ \
		const auto& MatcherInstance = Matcher.operator()<decltype(Value)>(); \
		if (!ensureAlwaysMsgf(MatcherInstance.Matches(Value), TEXT("%s must %s"), *CQTestConvert::ToString(Value), *MatcherInstance.Describe())) \
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
	static TUESTInstantiator<BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl))> BOOST_PP_CAT(ClassName, Instantiator); \
	struct BOOST_PP_CAT(F, BOOST_PP_CAT(ClassName, Impl)) \
	    : public BOOST_PP_CAT(F, ClassName)

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

#define TEST_METHOD(MethodName) \
	FUESTMethodRegistrar reg##MethodName{TEXT(#MethodName), *this, FSimpleDelegate::CreateRaw(this, &ThisClass::MethodName)}; \
	void MethodName()
