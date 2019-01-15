%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}
int main(int argc, char *argv[])
{
@if %{UseVirtualKeyboard}
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

@endif
@if %{SetQPAPhysicalSize}
    if (qEnvironmentVariableIsEmpty("QTGLESSTREAM_DISPLAY")) {
        qputenv("QT_QPA_EGLFS_PHYSICAL_WIDTH", QByteArray("213"));
        qputenv("QT_QPA_EGLFS_PHYSICAL_HEIGHT", QByteArray("120"));

        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    }
@else
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
@endif

    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
