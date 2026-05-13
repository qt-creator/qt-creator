class Test {
    int& r;

public:
    int &getR() const
    {
        return r;
    }
    void setR(const int &newR)
    {
        r = newR;
    }
};
