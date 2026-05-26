struct XmarksTheSpot {
    Q_PROPERTY(int it READ getIt WRITE setIt BINDABLE bindableIt)

public:
    int getIt() const;
    void setIt(int it)
    {
        m_it = it;
    }
    QBindable<int> bindableIt() { return &m_it; }

private:
    QProperty<int> m_it;
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
