struct QObject { void connect(); }
struct Pair { int a; int b; Pair(int a, int b); };
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(Pair it MEMBER m_it READ getIt NOTIFY itChanged BINDABLE bindableIt)
public:
    Pair getIt() const { return m_it; }
    QBindable<Pair> bindableIt() { return &m_it; }

public slots:
    void setIt(Pair it) { m_it = it; emit itChanged(it); }
signals:
    void itChanged(Pair it);
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(XmarksTheSpot, Pair, m_it, (Pair{1, 2}), &XmarksTheSpot::itChanged);
};
