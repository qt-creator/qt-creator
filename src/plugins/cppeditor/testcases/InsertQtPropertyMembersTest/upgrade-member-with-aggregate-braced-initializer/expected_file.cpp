struct S { int a; int b; };
struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(S it MEMBER m_it READ getIt NOTIFY itChanged BINDABLE bindableIt)
public:
    S getIt() const { return m_it; }
    QBindable<S> bindableIt() { return &m_it; }

public slots:
    void setIt(S it) { m_it = it; emit itChanged(it); }
signals:
    void itChanged(S it);
private:
    Q_OBJECT_BINDABLE_PROPERTY_WITH_ARGS(XmarksTheSpot, S, m_it, (S{1, 2}), &XmarksTheSpot::itChanged);
};
