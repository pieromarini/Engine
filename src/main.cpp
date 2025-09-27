#include "core/core_inc.h"
#include "platform/os/os_inc.h"

#include "core/core_inc.cpp"
#include "platform/os/os_inc.cpp"

#include <cstdio>

using namespace pm;

struct Test {
	f32 a;
	u32 b;
};


int main() {
	printf("Hello\n");
	auto arena = ArenaAllocDefault();

	auto* t = PushStruct(arena, Test);
	t->a = 10.07f;
	t->b = 12;

	printf("%f %d\n", t->a, t->b);

	arenaRelease(arena);

	return 0;
}
