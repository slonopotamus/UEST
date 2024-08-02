#include "UEST.h"

TEST(SimpleTest)
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
}

TEST_CLASS(SimpleTestClass)
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

TEST(Test, With, Deep, Naming)
{
	ASSERT_THAT(true, Is::True);
}

TEST_CLASS(TestClass, With, Deep, Naming)
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
