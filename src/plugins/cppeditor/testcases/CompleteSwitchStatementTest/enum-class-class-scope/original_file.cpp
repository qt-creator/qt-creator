struct C { enum class EnumType { V1, V2 }; };

void f(C::EnumType t) {
    @switch (t) {
    }
}
