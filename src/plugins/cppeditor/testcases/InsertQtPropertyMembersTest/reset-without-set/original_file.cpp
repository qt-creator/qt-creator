struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(int it READ getIt RESET resetIt NOTIFY itChanged)
};
