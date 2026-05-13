class Something
{
    int &it;

public:
    int &getIt() const;
    void setIt(const int &it);
};

int &Something::getIt() const
{
    return it;
}

void Something::setIt(const int &it)
{
    this->it = it;
}
