struct QObject { void connect(); }
class XmarksTheSpot : public QObject {
private:
    @Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)
public:
    void find();
};
