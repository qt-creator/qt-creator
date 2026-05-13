class Something
{
    int m_;

public:
    int m() const;
    void setM(int m);
};

int Something::m() const
{
    return m_;
}

void Something::setM(int m)
{
    m_ = m;
}
