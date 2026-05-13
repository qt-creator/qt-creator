struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    Q_PROPERTY(int it READ getIt RESET resetIt)

public:
    int getIt() const;

public slots:
    void resetIt()
    {
        static int defaultValue{}; // TODO: Adapt to use your actual default value
        m_it = defaultValue;
    }

private:
    int m_it;
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
