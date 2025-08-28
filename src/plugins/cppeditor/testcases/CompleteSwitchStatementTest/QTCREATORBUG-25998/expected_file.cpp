// TODO: Type minimization.
template <typename T> T enumCast(int value) { return static_cast<T>(value); }
class Test {
    enum class E { V1, V2 };
    void func(int i) {
        switch (enumCast<E>(i)) {
        case Test::E::V1:
            break;
        case Test::E::V2:
            break;
        }
    }
};
