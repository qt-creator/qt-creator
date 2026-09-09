struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it MEMBER m_it READ getIt NOTIFY itChanged BINDABLE bindableIt)
public:
    int getIt() const { return m_it; }
    QBindable<int> bindableIt() { return &m_it; }

public slots:
    void setIt(int it) { if (m_it == it) return; m_it = it; emit itChanged(it); }
signals:
    void itChanged(int it);
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(XmarksTheSpot, int, m_it, (int{1 + 2}), &XmarksTheSpot::itChanged);
};
