struct XmarksTheSpot {
    Q_PROPERTY(int it MEMBER m_it BINDABLE bindableIt)

public:
    QBindable<int> bindableIt() { return &m_it; }

private:
    QProperty<int> m_it(42);
};
