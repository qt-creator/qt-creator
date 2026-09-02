struct Socket
{
    void connect(void *, void *, void *);
    void open();
};

void Socket::open()
{
    connect(nullptr, nullptr, [this]{ doSt@uff(); });
}
