class Test {
    int m_fooBar_test;

public:
    int give_me_foo_bar_test() const
    {
        return m_fooBar_test;
    }
    void Seet_FooBar_test(int New_Foo_Bar_Test)
    {
        if (m_fooBar_test == New_Foo_Bar_Test)
            return;
        m_fooBar_test = New_Foo_Bar_Test;
        emit newFooBarTestValue();
    }
    void set_fooBarTest_toDefault()
    {
        Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
    }

signals:
    void newFooBarTestValue();

private:
    Q_PROPERTY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)
};
