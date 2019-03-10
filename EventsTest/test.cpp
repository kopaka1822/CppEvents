#include "pch.h"

#define TestSuite EventTest

// simple event creation
TEST(TestSuite, Invoke) 
{
	Event<int> e;

	bool called = false;
	auto h1 = e.subscribe([&called](int i)
	{
		EXPECT_EQ(i, 1);
		called = true;
	});

	EXPECT_FALSE(called);
	e.invoke(1);
	EXPECT_TRUE(called);
}

TEST(TestSuite, ManualSubsribeUnsubscribe)
{
	Event<int> e;

	int h1Count = 0;
	Handler<int> h1([&h1Count](int i)
	{
		h1Count += i;
	});

	int h2Count = 0;
	Handler<int> h2([&h2Count](int i)
	{
		h2Count += i;
	});

	e.invoke(5);
	ASSERT_EQ(h1Count, 0);

	e.subscribe(&h1);
	e.invoke(5);
	ASSERT_EQ(h1Count, 5);

	e.subscribe(&h2);
	e.invoke(2);
	ASSERT_EQ(h1Count, 7);
	ASSERT_EQ(h2Count, 2);

	e.unsubscribe(&h1);
	e.invoke(1);
	ASSERT_EQ(h1Count, 7);
	ASSERT_EQ(h2Count, 3);

	// unsubscribe again
	ASSERT_NO_THROW(e.unsubscribe(&h1));
	e.invoke(1);
	ASSERT_EQ(h2Count, 4);
}

// handler unsubscribe after going out of scope
TEST(TestSuite, HandlerScope)
{
	Event<int> e;

	int h1Count = 0;
	int h2Count = 0;

	auto h1 = e.subscribe([&h1Count](int i)
	{
		h1Count += i;
	});

	{
		auto h2 = e.subscribe([&h2Count](int i)
		{
			h2Count += i;
		});

		e.invoke(3);
		EXPECT_EQ(h1Count, 3);
		EXPECT_EQ(h2Count, 3);
	}

	e.invoke(1);
	EXPECT_EQ(h1Count, 4);
	EXPECT_EQ(h2Count, 3);
}

TEST(TestSuite, HandlerReset)
{
	Event<int> e1;
	Event<int> e2;

	int h1Count = 0;
	auto h1 = e1.subscribe([&h1Count](int i)
	{
		h1Count += i;
	});

	e1.invoke(1);
	EXPECT_EQ(h1Count, 1);

	e2.subscribe(&h1);
	e2.invoke(1);
	EXPECT_EQ(h1Count, 2);

	h1.reset();

	e1.invoke(10);
	e2.invoke(10);
	EXPECT_EQ(h1Count, 2);
}

TEST(TestSuite, EventScope)
{
	{
		Handler<int> h1([](int i)
		{
			EXPECT_EQ(i, 1);
		});

		{
			Event<int> e;
			e.subscribe(&h1);

			e.invoke(1);
		}
	}
	// handler should not throw on destruction
}

TEST(TestSuite, EventReset)
{
	int h1Count = 0;
	Handler<int> h1([&h1Count](int i)
	{
		h1Count += i;
	});

	int h2Count = 0;
	Handler<int> h2([&h2Count](int i)
	{
		h2Count += i;
	});

	Event<int> e;
	e.subscribe(&h1);
	e.subscribe(&h2);

	e.invoke(1);
	ASSERT_EQ(h1Count, 1);
	ASSERT_EQ(h2Count, 1);

	e.reset();
	e.invoke(1);
	ASSERT_EQ(h1Count, 1);
	ASSERT_EQ(h2Count, 1);
}

TEST(TestSuite, HandlerClassConstructor)
{
	class TestClass
	{
	public:
		explicit TestClass(Event<int>& e)
			:
		mutHandler(this, &TestClass::testFunc1),
		constHandler(this, &TestClass::testFunc2)
		{
			e.subscribe(&mutHandler);
			e.subscribe(&constHandler);
		}

		void testFunc1(int arg)
		{
			val = arg;
		}

		void testFunc2(int arg) const
		{
			EXPECT_EQ(arg, 2);
		}

		int val = 0;
		Handler<int> mutHandler;
		Handler<int> constHandler;
	};

	Event<int> e;

	{
		TestClass c(e);

		e.invoke(2);
		ASSERT_EQ(c.val, 2);
	}

	ASSERT_NO_THROW(e.invoke(1));
}