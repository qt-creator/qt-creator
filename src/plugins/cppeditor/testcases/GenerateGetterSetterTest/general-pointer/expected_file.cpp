class Something
{
    int *it;

public:
    int *getIt() const;
    void setIt(int *it);
};

int *Something::getIt() const
{
    return it;
}

void Something::setIt(int *it)
{
    this->it = it;
}
