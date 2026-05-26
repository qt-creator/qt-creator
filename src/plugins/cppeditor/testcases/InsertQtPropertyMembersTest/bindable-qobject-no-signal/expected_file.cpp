struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it READ getIt WRITE setIt BINDABLE bindableIt)

public:
    int getIt() const;
    QBindable<int> bindableIt() { return &m_it; }

public slots:
    void setIt(int it)
    {
        m_it = it;
    }

private:
    Q_OBJECT_BINDABLE_PROPERTY(XmarksTheSpot, int, m_it);
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
