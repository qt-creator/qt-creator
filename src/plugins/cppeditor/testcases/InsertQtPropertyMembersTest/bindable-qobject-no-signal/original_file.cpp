struct QObject { void connect(); }
struct XmarksTheSpot : public QObject {
    @Q_PROPERTY(int it READ getIt WRITE setIt BINDABLE bindableIt)
};
