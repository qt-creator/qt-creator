enum class E {A, B};
template<typename T> struct S {
    static T theType() { return T(); }
};
int main() {
    switch (S<E>::theType()) {
    case E::A:
        break;
    case E::B:
        break;
    }
}
