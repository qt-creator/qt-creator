enum Types {
    TypeA,
    TypeC,
    TypeB,
    TypeD,
    TypeE = TypeD
};

int main()
{
    int j;
    Types t = TypeA;
    Types t2 = TypeB;
    bool b = true;
    enum { foo, bla } i;

    // all handled, don't activate
    switch (t) {
    case TypeA:
    case TypeB:
    case TypeC:
    case TypeD:
    case TypeE:
        ;
    }

    // TypeD must still be added for the outer switch
    switch (t) {
    case TypeA:
        switch (t2) {
        case TypeD: ;
        default: ;
        }
        break;
    case TypeB:
    case TypeE:
        break;
    default:
        ;
    }

    // Resolve type for expressions as switch condition
    switch (b ? t : t2) {
    case TypeA:;
    }

    // Not a named type
    switch (i) {
    case bla:
        ;
    }

    // ignore pathological case that doesn't have a compound statement
    switch (i)
    case foo: ;
}
