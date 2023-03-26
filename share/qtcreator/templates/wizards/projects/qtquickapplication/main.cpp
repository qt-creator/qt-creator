%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}

int main(int argc, char *argv[])
{
@if %{UseVirtualKeyboard}
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

@endif
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
@if !%{HasLoadFromModule}
    const QUrl url(u"qrc:/%{JS: value('ProjectName')}/Main.qml"_qs);
@endif
@if %{HasFailureSignal}
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed,
                     &app, []() { QCoreApplication::exit(-1); },
                     Qt::QueuedConnection);
@else
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
@endif
@if %{HasLoadFromModule}
    engine.loadFromModule("%{JS: value('ProjectName')}", "Main");
@else
    engine.load(url);
@endif

    return app.exec();
}
