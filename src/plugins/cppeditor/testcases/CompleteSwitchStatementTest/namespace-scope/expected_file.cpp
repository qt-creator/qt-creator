namespace N { enum EnumType { V1, V2 }; };

void f(N::EnumType t) {
    switch (t) {
    case N::V1:
        break;
    case N::V2:
        break;
    }
}
