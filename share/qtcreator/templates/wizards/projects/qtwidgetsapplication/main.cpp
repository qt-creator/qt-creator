%{Cpp:LicenseTemplate}\
#include "%{HdrFileName}"

%{JS: QtSupport.qtIncludes([ 'QtGui/QApplication' ], [ 'QtWidgets/QApplication' ]) }\
@if %{HasTranslation}
#include <QLocale>
#include <QTranslator>
@endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
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
    %{Class} w;
    w.show();
    return a.exec();
}
