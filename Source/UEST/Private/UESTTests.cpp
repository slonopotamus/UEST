#include "UEST.h"

TEST_CLASS(TestClass)
{
	TEST_METHOD(Test1)
	{
		ASSERT_THAT(true);
	}

	TEST_METHOD(Test2)
	{
		ASSERT_THAT(true);
	}
};

TEST(Test)
{
	ASSERT_THAT(true);
}

TEST(Test, With, Deep, Naming)
{
	ASSERT_THAT(true);
}

TEST_CLASS(TestClass, With, Deep, Naming)
{
	TEST_METHOD(Test1)
	{
		ASSERT_THAT(true);
	}

	TEST_METHOD(Test2)
	{
		ASSERT_THAT(true);
	}
};
