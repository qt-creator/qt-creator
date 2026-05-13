class @Foo {
public:
    int bar() const;
    void setBar(int bar) { m_bar = bar; }

    int getBar2() const;

    int m_alreadyPublic;

private:
    friend void distraction();
    class AnotherDistraction {};
    enum EvenMoreDistraction { val1, val2 };

    int m_bar;
    int bar2_;
    QString bar3;
};
