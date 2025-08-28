enum EnumType { V1, V2 };

void f()
{
    EnumType t;
    switch (t) {
    case V1:
        break;
    case V2:
        break;
    default:
        break;
    }
}
