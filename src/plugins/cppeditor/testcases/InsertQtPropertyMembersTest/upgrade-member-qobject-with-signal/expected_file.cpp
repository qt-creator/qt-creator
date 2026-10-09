struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it MEMBER m_it NOTIFY itChanged BINDABLE bindableIt)

public:
    QBindable<int> bindableIt() { return &m_it; }

signals:
    void itChanged(int it);
private:
    Q_OBJECT_BINDABLE_PROPERTY(XmarksTheSpot, int, m_it, &XmarksTheSpot::itChanged);
};
