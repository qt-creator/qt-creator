namespace N{
template<typename T>
struct vector{
    using iterator = T*;
};
using int_vector = vector<int>;
}
N::int_vector ints;
N::int_vector::iterator intIter;
using vec = N::vector<int>;
vec::iterator it;
