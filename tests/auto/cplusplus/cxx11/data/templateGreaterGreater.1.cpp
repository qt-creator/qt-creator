template <class i, int j = 1>
class Y {};
template <int i>
class X {};

Y<X<6>, 7> x;
Y<X<1>> y;
X< (1 >> 2) > z;
auto a = static_cast<X<1>>(X<1>());

