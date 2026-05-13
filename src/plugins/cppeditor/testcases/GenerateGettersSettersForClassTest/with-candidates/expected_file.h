class Foo {
public:
    int bar() const;
    void setBar(int bar) { m_bar = bar; }

    int getBar2() const;

    int m_alreadyPublic;

    void resetBar();
    void setBar2(int value);
    void resetBar2();
    const QString &getBar3() const;
    void setBar3(const QString &value);
    void resetBar3();

signals:
    void bar2Changed();
    void bar3Changed();

private:
    friend void distraction();
    class AnotherDistraction {};
    enum EvenMoreDistraction { val1, val2 };

    int m_bar;
    int bar2_;
    QString bar3;
    Q_PROPERTY(int bar2 READ getBar2 WRITE setBar2 RESET resetBar2 NOTIFY bar2Changed FINAL)
    Q_PROPERTY(QString bar3 READ getBar3 WRITE setBar3 RESET resetBar3 NOTIFY bar3Changed FINAL)
};

inline void Foo::resetBar()
{
    setBar({}); // TODO: Adapt to use your actual default defaultValue
}

inline void Foo::setBar2(int value)
{
    if (bar2_ == value)
        return;
    bar2_ = value;
    emit bar2Changed();
}

inline void Foo::resetBar2()
{
    setBar2({}); // TODO: Adapt to use your actual default defaultValue
}

inline const QString &Foo::getBar3() const
{
    return bar3;
}

inline void Foo::setBar3(const QString &value)
{
    if (bar3 == value)
        return;
    bar3 = value;
    emit bar3Changed();
}

inline void Foo::resetBar3()
{
    setBar3({}); // TODO: Adapt to use your actual default defaultValue
}
