%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], [%{UseQApplication} ? "QtWidgets/QApplication" : "QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}
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
