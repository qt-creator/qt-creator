struct C { enum class EnumType { V1, V2 }; };

void f(C::EnumType t) {
    switch (t) {
    case C::EnumType::V1:
        break;
    case C::EnumType::V2:
        break;
    }
}
