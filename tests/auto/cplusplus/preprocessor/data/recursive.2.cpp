#define a(x) b(x) c(x)
#define b(x) c(x) a(x)
#define c(x) a(x) b(x)

b(1)
a(1)
