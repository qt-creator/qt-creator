struct C { enum EnumType { V1, V2 }; };

void f(C::EnumType t) {
    switch (t) {
    case C::V1:
        break;
    case C::V2:
        break;
    }
}
