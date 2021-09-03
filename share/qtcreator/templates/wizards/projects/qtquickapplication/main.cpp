%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([], ["QtGui/QGuiApplication", "QtQml/QQmlApplicationEngine"])}
@if %{HasTranslation}
#include <QLocale>
#include <QTranslator>
@endif

int main(int argc, char *argv[])
{
@if %{UseVirtualKeyboard}
    qputenv("QT_IM_MODULE", QByteArray("qtvirtualkeyboard"));

@endif
@if !%{IsQt6}
@if %{SetQPAPhysicalSize}
    if (qEnvironmentVariableIsEmpty("QTGLESSTREAM_DISPLAY")) {
        qputenv("QT_QPA_EGLFS_PHYSICAL_WIDTH", QByteArray("213"));
        qputenv("QT_QPA_EGLFS_PHYSICAL_HEIGHT", QByteArray("120"));

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
        QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    }
@else
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
@endif
@endif
    QGuiApplication app(argc, argv);
@if %{HasTranslation}

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "%{JS: value('ProjectName') + '_'}" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            app.installTranslator(&translator);
            break;
        }
    }
@endif

    QQmlApplicationEngine engine;
@if %{IsQt6}
    const QUrl url(u"qrc:/%{JS: value('ProjectName')}/main.qml"_qs);
@else
    const QUrl url(QStringLiteral("qrc:/main.qml"));
@endif
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    return app.exec();
}
