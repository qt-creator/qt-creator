struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(int it MEMBER m_it)
private:
    int m_it;
};
