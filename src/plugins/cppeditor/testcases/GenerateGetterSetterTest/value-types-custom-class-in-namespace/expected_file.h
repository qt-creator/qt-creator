namespace N1{
class Value{};
}
class Test {
    N1::Value v;

public:
    N1::Value getV() const
    {
        return v;
    }
};
