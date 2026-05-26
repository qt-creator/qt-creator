struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged BINDABLE bindableIt)

public:
    int getIt() const;
    QBindable<int> bindableIt() { return &m_it; }

public slots:
    void setIt(int it)
    {
        if (m_it == it)
            return;
        m_it = it;
        emit itChanged(m_it);
    }

signals:
    void itChanged(int it);

private:
    Q_OBJECT_BINDABLE_PROPERTY(XmarksTheSpot, int, m_it, &XmarksTheSpot::itChanged);
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
