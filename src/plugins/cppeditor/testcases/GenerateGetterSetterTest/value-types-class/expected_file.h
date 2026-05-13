class NoVal{};
class Test {
    NoVal n;

public:
    const NoVal &getN() const
    {
        return n;
    }
};
