struct QObject { void connect(); }
class XmarksTheSpot : public QObject {
private:
    Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)
    int m_it;

public:
    void find();
    int getIt() const;
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
};

int XmarksTheSpot::getIt() const
{
    return m_it;
}
