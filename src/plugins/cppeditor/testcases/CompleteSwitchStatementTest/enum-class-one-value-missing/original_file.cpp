enum class EnumType { V1, V2 };

void f()
{
    EnumType t;
    @switch (t) {
    case EnumType::V2:
        break;
    default:
        break;
    }
}
