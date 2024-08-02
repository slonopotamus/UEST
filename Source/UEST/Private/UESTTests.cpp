#include "UEST.h"

TEST_CLASS(UEST_TestClass)
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

TEST(UEST_Test)
{
	ASSERT_THAT(true);
}
