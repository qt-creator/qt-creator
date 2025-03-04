enum class EnumType { V1, V2 } t;

void f()
{
    switch (t) {
    case EnumType::V1:
        break;
    case EnumType::V2:
        break;
    }
}
