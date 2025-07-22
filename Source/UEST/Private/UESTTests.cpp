#include "UEST.h"

TEST(UEST, SimpleTest)
{
	ASSERT_THAT(true, Is::True);

	bool t = true;
	ASSERT_THAT(t, Is::True);
	ASSERT_THAT(t, Is::Not::False);

	bool f = false;
	ASSERT_THAT(f, Is::False);
	ASSERT_THAT(f, Is::Not::True);

	const int* v1 = nullptr;
	ASSERT_THAT(v1, Is::Null);

	const int v2 = 42;
	ASSERT_THAT(&v2, Is::Not::Null);

	// TODO: Need explicit <int> for now because of clang bug: https://github.com/llvm/llvm-project/issues/73093
	ASSERT_THAT(42, Is::EqualTo<int>(42));
	ASSERT_THAT(42, Is::Not::EqualTo<int>(43));

	ASSERT_THAT(1.0, Is::NearlyEqualTo<double>(1.1, 0.2));
	ASSERT_THAT(1.0, Is::Not::NearlyEqualTo<double>(1.1, 0.05));

	ASSERT_THAT(42, Is::LessThan<int>(43));
	ASSERT_THAT(42, Is::Not::LessThan<int>(41));

	ASSERT_THAT(42, Is::GreaterThan<int>(41));
	ASSERT_THAT(42, Is::Not::GreaterThan<int>(43));

	ASSERT_THAT(0, Is::Zero);
	ASSERT_THAT(1, Is::Not::Zero);
	ASSERT_THAT(1, Is::Positive);
	ASSERT_THAT(1, Is::Not::Negative);
	ASSERT_THAT(-1, Is::Negative);
	ASSERT_THAT(-1, Is::Not::Positive);

	ASSERT_THAT(1, Is::InRange<int>(0, 2));
	ASSERT_THAT(1, Is::Not::InRange<int>(2, 4));

	ASSERT_THAT(FString{}, Is::Empty);
	ASSERT_THAT(FString{"abc"}, Is::Not::Empty);

	ASSERT_THAT(TSharedPtr<int>{}, Is::Not::Valid);

	ASSERT_THAT(NAN, Is::NaN);
}

TEST(UEST, Test, With, Deep, Naming)
{
	ASSERT_THAT(true, Is::True);
}

TEST_DISABLED(UEST, Test, Disabled)
{
	ASSERT_THAT(true, Is::False);
}

// clang-format off
TEST_CLASS(UEST, SimpleTestClass)
{
	TEST_METHOD(Test1)
	{
		ASSERT_THAT(true, Is::True);
	}

	TEST_METHOD(Test2)
	{
		ASSERT_THAT(true, Is::True);
	}
};

TEST_CLASS(UEST, TestClass, With, Deep, Naming)
{
	TEST_METHOD(Test1)
	{
		ASSERT_THAT(true, Is::True);
	}

	TEST_METHOD(Test2)
	{
		ASSERT_THAT(true, Is::True);
	}
};

TEST_CLASS(UEST, BeforeAfter)
{
	int32 BeforeCalled = 0;
	int32 AfterCalled = 0;

	BEFORE_EACH()
	{
		BeforeCalled++;
	}

	AFTER_EACH()
	{
		AfterCalled++;
	}

	TEST_METHOD(Test1)
	{
		ASSERT_THAT(BeforeCalled, Is::EqualTo<int>(1));
		ASSERT_THAT(AfterCalled, Is::EqualTo<int>(0));
	}

	TEST_METHOD(Test2)
	{
		ASSERT_THAT(BeforeCalled, Is::EqualTo<int>(2));
		ASSERT_THAT(AfterCalled, Is::EqualTo<int>(1));
	}
};

TEST_CLASS_DISABLED(UEST, TestClass, Disabled)
{
	TEST_METHOD(This_Will_Not_Run)
	{
		ASSERT_THAT(true, Is::False);
	}
};
// clang-format on
