class {
    int m_foo;

public:
    int foo() const
    {
        return m_foo;
    }
    void setFoo(int value)
    {
        if (m_foo == value)
            return;
        m_foo = value;
        emit fooChanged();
    }
    void resetFoo()
    {
        setFoo({}); // TODO: Adapt to use your actual default defaultValue
    }

signals:
    void fooChanged();

private:
    Q_PROPERTY(int foo READ foo WRITE setFoo RESET resetFoo NOTIFY fooChanged FINAL)
} bar;
