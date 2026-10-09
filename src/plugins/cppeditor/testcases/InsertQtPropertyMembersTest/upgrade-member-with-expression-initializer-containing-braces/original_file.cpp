struct QObject { void connect(); }
struct Pair { int a; int b; Pair(int a, int b); };
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(Pair it MEMBER m_it READ getIt NOTIFY itChanged)
public:
    Pair getIt() const { return m_it; }
public slots:
    void setIt(Pair it) { m_it = it; emit itChanged(it); }
signals:
    void itChanged(Pair it);
private:
    Pair m_it = Pair{1, 2};
};
