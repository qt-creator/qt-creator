namespace N { enum EnumType { V1, V2 }; };

void f(N::EnumType t) {
    @switch (t) {
    }
}
