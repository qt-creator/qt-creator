namespace N{
template<typename T>
struct vector{
    using iterator = T*;
};
using int_vector = vector<int>;
}
using namespace N@;
int_vector ints;
int_vector::iterator intIter;
using vec = vector<int>;
vec::iterator it;
