class Widget
{
public:
    int getValue(int extra) { return m_value + extra; }

private:
    int m_value = 0;
};

int user(Widget &w)
{
    return w.getVal@ue(5);
}
