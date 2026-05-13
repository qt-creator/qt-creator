class Foo
{
public:
    enum Bar { b1, b2 };

    Bar bar() const;
    void setBar(Bar newBar);

private:
    Bar m_bar;
};
