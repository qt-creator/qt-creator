%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}

int main(int argc, char *argv[])
{
@if %{UseVirtualKeyboard}
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

@endif
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(u"qrc:/%{JS: value('ProjectName')}/main.qml"_qs);
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
