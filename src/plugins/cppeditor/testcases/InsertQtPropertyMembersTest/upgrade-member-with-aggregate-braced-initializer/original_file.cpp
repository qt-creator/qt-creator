struct S { int a; int b; };
struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(S it MEMBER m_it READ getIt NOTIFY itChanged)
public:
    S getIt() const { return m_it; }
public slots:
    void setIt(S it) { m_it = it; emit itChanged(it); }
signals:
    void itChanged(S it);
private:
    S m_it{1, 2};
};
