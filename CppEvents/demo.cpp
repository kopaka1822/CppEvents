#include "../include/events/event.h"
#include <iostream>

using namespace evnt;

using CustomEvent = Event<int>;

CustomEvent MyEvent;
CustomEvent MyEvent2;

void testFunc(int i)
{
	std::cout << i << "testFunc\n";
}

void testFunc2(int i)
{
	std::cout << i << "testFunc2\n";
}

int main()
{
	CustomEvent::HandlerT myHand(testFunc);
	MyEvent.subscribe(&myHand);
	MyEvent2.subscribe(&myHand);

	{
		CustomEvent::HandlerT myHand2(testFunc2);
		MyEvent.subscribe(&myHand2);

		MyEvent.invoke(1);
	}

	MyEvent.invoke(2);

	myHand.reset();

	MyEvent.invoke(3);

	MyEvent.reset();

	MyEvent.invoke(4);
	return 0;
}
