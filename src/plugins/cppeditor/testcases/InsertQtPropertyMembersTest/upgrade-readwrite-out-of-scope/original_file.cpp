struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)
public:
    int getIt() const { return m_it; }
public slots:
    void setIt(int it) { if (m_it == it) return; m_it = it; emit itChanged(it); }
signals:
    void itChanged(int it);
private:
    int m_it;
};
