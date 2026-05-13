struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it READ getIt WRITE setIt RESET resetIt NOTIFY itChanged)

public:
    int getIt() const;

public slots:
    void setIt(int it)
    {
        if (m_it == it)
            return;
        m_it = it;
        emit itChanged(m_it);
    }
    void resetIt()
    {
        setIt({}); // TODO: Adapt to use your actual default value
    }

signals:
    void itChanged(int it);

private:
    int m_it;
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
