class Something
{
    const int &it;

public:
    const int &getIt() const;
    void setIt(const int &it);
};

const int &Something::getIt() const
{
    return it;
}

void Something::setIt(const int &it)
{
    this->it = it;
}
