struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it MEMBER m_it BINDABLE bindableIt)

public:
    QBindable<int> bindableIt() { return &m_it; }

private:
    Q_OBJECT_BINDABLE_PROPERTY(XmarksTheSpot, int, m_it);
};
