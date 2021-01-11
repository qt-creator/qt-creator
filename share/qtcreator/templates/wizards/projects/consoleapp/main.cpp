%{Cpp:LicenseTemplate}\
%{JS: QtSupport.qtIncludes([ 'QtCore/QCoreApplication' ],
                           [ 'QtCore/QCoreApplication' ]) }\
@if %{HasTranslation}
#include <QLocale>
#include <QTranslator>
@endif

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
@if %{HasTranslation}

    QTranslator translator;
    const QStringList uiLanguages = QLocale::system().uiLanguages();
    for (const QString &locale : uiLanguages) {
        const QString baseName = "%{JS: value('ProjectName') + '_'}" + QLocale(locale).name();
        if (translator.load(":/i18n/" + baseName)) {
            a.installTranslator(&translator);
            break;
        }
    }
@endif

    return a.exec();
}
