#pragma once

#include "Elements/Common/TypedElementCommonTypes.h"
#include "Misc/AutomationTest.h"

// TODO: Get rid of Boost
#include <boost/preprocessor/comparison/greater.hpp>
#include <boost/preprocessor/seq/fold_left.hpp>
#include <boost/preprocessor/variadic/to_seq.hpp>

namespace UEST
{
	template<typename M, typename... P>
	concept Matcher = requires(M const m, P... p) {
		{
			M{p...}
		};
		// TODO: Can we require that there exists such template? template<typename T> bool M::Matches(const<T>&)
		//{ m.Matches(t) } -> std::same_as<bool>;
		{
			m.Describe()
		} -> UE::same_as<FString>;
	};

	namespace Matchers
	{
		struct Null final
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

		struct True final
		{
			template<typename T>
			    requires(UE::same_as<std::decay_t<T>, bool>)
			bool Matches(const bool& Value) const
			{
				return Value;
			}

			FString Describe() const
			{
				return TEXT("be true");
			}
		};

		struct False final
		{
			template<typename T>
			    requires(UE::same_as<std::decay_t<T>, bool>)
			bool Matches(const bool& Value) const
			{
				return !Value;
			}

			FString Describe() const
			{
				return TEXT("be false");
			}
		};

		struct Empty final
		{
			template<typename T>
			    requires requires(const T t) {
				    {
					    t.IsEmpty()
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value.IsEmpty();
			}

			FString Describe() const
			{
				return TEXT("be empty");
			}
		};

		struct Valid final
		{
			template<typename T>
			    requires requires(const T t) {
				    {
					    t.IsValid()
				    };
			    }
			bool Matches(const T& Value) const
			{
				return Value.IsValid();
			}

			FString Describe() const
			{
				return TEXT("be valid");
			}
		};

		template<typename P>
		struct EqualTo final : FNoncopyable
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

		template<typename E, typename T>
		// TODO: Add concepts
		struct NearlyEqualTo final : FNoncopyable
		{
			const E Expected;
			const T Tolerance;

			explicit NearlyEqualTo(E Expected, T Tolerance = UE_SMALL_NUMBER)
			    : Expected{Expected}
			    , Tolerance{Tolerance}
			{
			}

			template<typename>
			bool Matches(const FVector& Value) const
			{
				return Value.Equals(Expected, Tolerance);
			}

			template<typename>
			bool Matches(const float& Value) const
			{
				return FMath::IsNearlyEqual(Value, Expected, Tolerance);
			}

			template<typename>
			bool Matches(const double& Value) const
			{
				return FMath::IsNearlyEqual(Value, Expected, Tolerance);
			}

			FString Describe() const
			{
				return FString::Printf(TEXT("be nearly equal to %s with tolerance %s"), *ToString(Expected), *ToString(Tolerance));
			}
		};

		template<typename P>
		struct LessThan final : FNoncopyable
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
		struct LessThanOrEqualTo final : FNoncopyable
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
		struct GreaterThan final : FNoncopyable
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
		struct GreaterThanOrEqualTo final : FNoncopyable
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

		struct NaN final
		{
			template<typename T>
			    requires std::is_floating_point_v<std::remove_reference_t<T>>
			bool Matches(const T& Value) const
			{
				return FMath::IsNaN(Value);
			}
			FString Describe() const
			{
				return TEXT("be nullptr");
			}
		};

		template<typename Lower, typename Upper = Lower>
		struct InRange final : FNoncopyable
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
				if (!ensureAlwaysMsgf(From <= To, TEXT("Invalid range, %s is greater than %s"), *ToString(From), *ToString(To)))
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
		struct Not final : FNoncopyable
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
	constexpr inline auto Null = UEST::Matchers::Null{};
	constexpr inline auto True = UEST::Matchers::True{};
	constexpr inline auto False = UEST::Matchers::False{};
	constexpr inline auto Empty = UEST::Matchers::Empty{};
	constexpr inline auto Valid = UEST::Matchers::Valid{};
	constexpr inline auto NaN = UEST::Matchers::NaN{};

	template<typename T>
	using EqualTo = UEST::Matchers::EqualTo<T>;

	template<typename E, typename T>
	using NearlyEqualTo = UEST::Matchers::NearlyEqualTo<E, T>;

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

	const inline auto Zero = EqualTo<int64>(0);
	const inline auto Positive = GreaterThan<int64>(0);
	const inline auto Negative = LessThan<int64>(0);

	template<typename T>
	using InRange = UEST::Matchers::InRange<T>;

	namespace Not
	{
		const inline auto Null = UEST::Matchers::Not<UEST::Matchers::Null>{};
		const inline auto False = Is::True;
		const inline auto True = Is::False;
		const inline auto Empty = UEST::Matchers::Not<UEST::Matchers::Empty>{};
		const inline auto Valid = UEST::Matchers::Not<UEST::Matchers::Valid>{};
		const inline auto NaN = UEST::Matchers::Not<UEST::Matchers::NaN>{};

		template<typename T>
		using EqualTo = UEST::Matchers::Not<UEST::Matchers::EqualTo<T>, T>;

		template<typename E, typename T>
		using NearlyEqualTo = UEST::Matchers::Not<UEST::Matchers::NearlyEqualTo<E, T>, E, T>;

		template<typename T>
		using LessThan = UEST::Matchers::GreaterThanOrEqualTo<T>;

		template<typename T>
		using LessThanOrEqualTo = UEST::Matchers::GreaterThan<T>;

		template<typename T>
		using GreaterThan = UEST::Matchers::LessThanOrEqualTo<T>;

		template<typename T>
		using GreaterThanOrEqualTo = UEST::Matchers::LessThan<T>;

		const inline auto Zero = EqualTo<int64>(0);

		const inline auto Positive = GreaterThan<int64>(0);

		const inline auto Negative = LessThan<int64>(0);

		template<typename T>
		using InRange = UEST::Matchers::Not<UEST::Matchers::InRange<T>, T, T>;
	} // namespace Not
} // namespace Is

// TODO: Provide ASSERT_THAT(Value) variant that tests that Value is true
#define ASSERT_THAT(Value, ...) \
	do \
	{ \
		const auto& _M = __VA_ARGS__; \
		const auto& _V = Value; \
		if (!ensureAlwaysMsgf(_M.template Matches<std::decay_t<decltype(_V)>>(_V), TEXT("%s: %s must %s"), TEXT(#Value), *ToString(_V), *_M.Describe())) \
		{ \
			return; \
		} \
	} while (false)

class UEST_API FUESTTestBase : public FAutomationTestBase
{
	typedef FAutomationTestBase Super;

protected:
	FUESTTestBase(const FString& InName, bool bIsComplex);

	virtual uint32 GetRequiredDeviceNum() const override;

	virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override;

	virtual FString GetTestSourceFileName(const FString& InTestName) const override;

	virtual int32 GetTestSourceFileLine(const FString& InTestName) const override;

	virtual bool RunTest(const FString& InTestName) override;

	virtual void Setup() {}

	virtual void TearDown() {}

public:
	struct FTestMethodInfo final
	{
		FSimpleDelegate Delegate;
		const TCHAR* FileName;
		const int32 FileLine;
	};

	// TODO: Can we do this without delegates, just using method pointers?
	TMap<FString, FTestMethodInfo> TestMethods;
};

struct FUESTMethodRegistrar
{
	FUESTMethodRegistrar(FUESTTestBase& Test, FString&& Name, FUESTTestBase::FTestMethodInfo&& Info)
	{
		Test.TestMethods.Add(MoveTemp(Name), MoveTemp(Info));
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

#define UEST_CONCAT_SEQ_1(seq, fold_op, elem_op) elem_op(BOOST_PP_SEQ_HEAD(seq))
#define UEST_CONCAT_SEQ_N(seq, fold_op, elem_op) BOOST_PP_SEQ_FOLD_LEFT(fold_op, elem_op(BOOST_PP_SEQ_HEAD(seq)), BOOST_PP_SEQ_TAIL(seq))
#define UEST_CONCAT_SEQ(seq, fold_op, elem_op) BOOST_PP_IF(BOOST_PP_GREATER(BOOST_PP_SEQ_SIZE(seq), 1), UEST_CONCAT_SEQ_N, UEST_CONCAT_SEQ_1)(seq, fold_op, elem_op)

#define UEST_PRETTY_NAME_ELEM_OP(x) UE_STRINGIZE(x)
#define UEST_PRETTY_NAME_FOLD_OP(s, state, x) state "." UEST_PRETTY_NAME_ELEM_OP(x)
#define UEST_PRETTY_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_PRETTY_NAME_FOLD_OP, UEST_PRETTY_NAME_ELEM_OP)

#define UEST_CLASS_NAME_ELEM_OP(x) x
#define UEST_CLASS_NAME_FOLD_OP(s, state, x) UE_JOIN(state, UE_JOIN(_, x))
#define UEST_CLASS_NAME(...) UEST_CONCAT_SEQ(BOOST_PP_VARIADIC_TO_SEQ(__VA_ARGS__), UEST_CLASS_NAME_FOLD_OP, UEST_CLASS_NAME_ELEM_OP)

#if ENGINE_MAJOR_VERSION > 5 || (ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION >= 5)
#define UEST_GET_TEST_FLAGS_RETURN_TYPE EAutomationTestFlags
#else
#define UEST_GET_TEST_FLAGS_RETURN_TYPE uint32
#endif

#define TEST_CLASS_WITH_BASE_IMPL(BaseClass, bIsComplex, Flags, ClassName, PrettyName) \
	struct UE_JOIN(F, UE_JOIN(ClassName, Impl)); \
	struct UE_JOIN(F, ClassName) \
	    : public BaseClass \
	{ \
		typedef UE_JOIN(F, UE_JOIN(ClassName, Impl)) ThisClass; \
		typedef BaseClass Super; \
		UE_JOIN(F, ClassName) \
		() \
		    : Super(TEXT(PrettyName), bIsComplex) \
		{ \
		} \
		/* This using is needed so Rider understands that we are a runnable test */ \
		using Super::RunTest; \
		/* TODO: Only add this when bIsComplex is true */ \
		virtual void GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const override; \
		virtual FString GetBeautifiedTestName() const override \
		{ \
			return TEXT(PrettyName); \
		} \
		virtual UEST_GET_TEST_FLAGS_RETURN_TYPE GetTestFlags() const override \
		{ \
			return EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter | Flags; \
		} \
		using Super::GetTestSourceFileName; \
		using Super::GetTestSourceFileLine; \
		virtual FString GetTestSourceFileName() const override \
		{ \
			return TEXT(__FILE__); \
		} \
		virtual int32 GetTestSourceFileLine() const override \
		{ \
			return __LINE__; \
		} \
	}; \
	void UE_JOIN(F, ClassName)::GetTests(TArray<FString>& OutBeautifiedNames, TArray<FString>& OutTestCommands) const \
	{ \
		Super::GetTests(OutBeautifiedNames, OutTestCommands); \
	} \
	static const TUESTInstantiator<UE_JOIN(F, UE_JOIN(ClassName, Impl))> UE_JOIN(ClassName, Instantiator); \
	struct UE_JOIN(F, UE_JOIN(ClassName, Impl)) \
	    : public UE_JOIN(F, ClassName)

#define TEST_CLASS_WITH_BASE(BaseClass, bIsComplex, Flags, ...) TEST_CLASS_WITH_BASE_IMPL(BaseClass, bIsComplex, Flags, UEST_CLASS_NAME(__VA_ARGS__), UEST_PRETTY_NAME(__VA_ARGS__))

#define TEST_WITH_BASE(BaseClass, Flags, ...) \
	TEST_CLASS_WITH_BASE(BaseClass, false, Flags, __VA_ARGS__) \
	{ \
		virtual bool RunTest(const FString& Parameters) override \
		{ \
			DoTest(Parameters); \
			return true; \
		} \
		void DoTest(const FString& Parameters); \
		/* clang-format off */ \
	}; \
	/* clang-format on */ \
	void UE_JOIN(UE_JOIN(F, UEST_CLASS_NAME(__VA_ARGS__)), Impl)::DoTest(const FString& Parameters)

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
#define TEST(...) TEST_WITH_BASE(FUESTTestBase, EAutomationTestFlags::None, __VA_ARGS__)

#define TEST_DISABLED(...) TEST_WITH_BASE(FUESTTestBase, EAutomationTestFlags::Disabled, __VA_ARGS__)

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
#define TEST_CLASS(...) TEST_CLASS_WITH_BASE(FUESTTestBase, true, EAutomationTestFlags::None, __VA_ARGS__)
#define TEST_CLASS_DISABLED(...) TEST_CLASS_WITH_BASE(FUESTTestBase, true, EAutomationTestFlags::Disabled, __VA_ARGS__)

#define TEST_METHOD(MethodName) \
	FUESTMethodRegistrar reg##MethodName{*this, TEXT(#MethodName), {FSimpleDelegate::CreateRaw(this, &ThisClass::MethodName), TEXT(__FILE__), __LINE__}}; \
	void MethodName()

#define BEFORE_EACH() virtual void Setup() override
#define AFTER_EACH() virtual void TearDown() override
