%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QGui/QGuiApplication", "QQml/QQmlApplicationEngine"])}
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
