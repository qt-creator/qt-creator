template<typename T> class TS {};
template<typename T, typename U> class TS<T(U)> {};

class S2 {
    TS<int(int)> @member;
};
