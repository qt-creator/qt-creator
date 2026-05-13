class Foo {
public:
    int bar() const;
    void setBar(int bar);
    void resetBar();

private:
    int m_bar;
};

inline void Foo::resetBar()
{
    setBar({}); // TODO: Adapt to use your actual default defaultValue
}
