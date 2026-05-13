template<typename T>
class Value{};
class Test {
    Value<int> v;

public:
    Value<int> getV() const
    {
        return v;
    }
};
