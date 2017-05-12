%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}
