class Test {
    Q_PROPER@TY(int fooBar_test READ give_me_foo_bar_test WRITE Seet_FooBar_test RESET set_fooBarTest_toDefault NOTIFY newFooBarTestValue FINAL)

public:
    int give_me_foo_bar_test() const
    {
        return mem_fooBar_test;
    }
    void Seet_FooBar_test(int New_Foo_Bar_Test)
    {
        if (mem_fooBar_test == New_Foo_Bar_Test)
            return;
        mem_fooBar_test = New_Foo_Bar_Test;
        emit newFooBarTestValue();
    }
    void set_fooBarTest_toDefault()
    {
        Seet_FooBar_test({}); // TODO: Adapt to use your actual default value
    }

signals:
    void newFooBarTestValue();
};
