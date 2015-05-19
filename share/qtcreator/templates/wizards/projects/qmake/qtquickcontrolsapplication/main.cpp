%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], [%{UseQApplication} ? "QWidgets/QApplication" : "QtGui/QGuiApplication", "QQml/QQmlApplicationEngine"])}
int main(int argc, char *argv[])
{
@if %{UseQApplication}
    QApplication app(argc, argv);
@else
    QGuiApplication app(argc, argv);
@endif

    QQmlApplicationEngine engine;
    engine.load(QUrl(QStringLiteral("qrc:/main.qml")));

    return app.exec();
}
