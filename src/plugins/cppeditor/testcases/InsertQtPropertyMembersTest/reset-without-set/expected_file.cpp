struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it READ getIt RESET resetIt NOTIFY itChanged)

public:
    int getIt() const;

public slots:
    void resetIt()
    {
        static int defaultValue{}; // TODO: Adapt to use your actual default value
        if (m_it == defaultValue)
            return;
        m_it = defaultValue;
        emit itChanged(m_it);
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
