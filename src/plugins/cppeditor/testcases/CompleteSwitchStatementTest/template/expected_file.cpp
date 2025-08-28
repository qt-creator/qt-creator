enum E {EA, EB};
template<typename T> struct S {
    static T theType() { return T(); }
};
int main() {
    switch (S<E>::theType()) {
    case EA:
        break;
    case EB:
        break;
    }
}
