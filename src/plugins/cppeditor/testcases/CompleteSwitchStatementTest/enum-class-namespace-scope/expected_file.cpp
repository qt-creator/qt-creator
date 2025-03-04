namespace N { enum class EnumType { V1, V2 }; };

void f(N::EnumType t) {
    switch (t) {
    case N::EnumType::V1:
        break;
    case N::EnumType::V2:
        break;
    }
}
