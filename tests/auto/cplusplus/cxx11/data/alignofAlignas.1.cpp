int i = alignof(int);
int t = alignof(C::foo) * 7 + alignof(Foo *);

struct alignas(f()) Foo {};
struct alignas(42) Foo {};
struct alignas(double) Bar {};
alignas(Foo) alignas(Bar) Foo *buffer;
struct alignas(Mooze...) Gnarf {};
