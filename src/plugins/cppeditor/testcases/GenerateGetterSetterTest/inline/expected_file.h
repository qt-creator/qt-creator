class Foo {
public:
    int bar;
    int getBar() const;
    void setBar(int value);
    void resetBar();
signals:
    void barChanged();
private:
    Q_PROPERTY(int bar READ getBar WRITE setBar RESET resetBar NOTIFY barChanged FINAL)
};

inline int Foo::getBar() const
{
    return bar;
}

inline void Foo::setBar(int value)
{
    if (bar == value)
        return;
    bar = value;
    emit barChanged();
}

inline void Foo::resetBar()
{
    setBar({}); // TODO: Adapt to use your actual default defaultValue
}
