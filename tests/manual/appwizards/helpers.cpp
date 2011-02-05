#include "qmlstandaloneapp.h"
#include "html5app.h"
#include <QtCore>

using namespace Qt4ProjectManager::Internal;

static bool writeFile(const QByteArray &data, const QString &targetFile)
{
    const QFileInfo fileInfo(targetFile);
    QDir().mkpath(fileInfo.absolutePath());
    QFile file(fileInfo.absoluteFilePath());
    file.open(QIODevice::WriteOnly);
    Q_ASSERT(file.isOpen());
    return file.write(data) != -1;
}

bool QmlStandaloneApp::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(QmlAppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (useExistingMainQml() ? true : writeFile(generateFile(QmlAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon))
            && writeFile(generateFile(QmlAppGeneratedFileInfo::DesktopFile, errorMessage), path(Desktop));
}

bool Html5App::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(Html5AppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (useExistingIndexHtml() ? true : writeFile(generateFile(Html5AppGeneratedFileInfo::IndexHtmlFile, errorMessage), path(IndexHtml)))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::DesktopFile, errorMessage), path(Desktop));
}

QString AbstractMobileApp::templatesRoot()
{
    return QLatin1String("../../../share/qtcreator/templates/");
}
