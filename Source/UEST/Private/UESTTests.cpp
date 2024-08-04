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
	// TODO: Doesn't compile. Need more coffee to understand how to fix.
	// ASSERT_THAT(1, Is::Not::InRange<int>(2, 4));
}

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

TEST(UEST, Test, With, Deep, Naming)
{
	ASSERT_THAT(true, Is::True);
}

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
