#include "qtquickapp.h"
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

bool QtQuickApp::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(QtQuickAppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (m_mainQmlMode != ModeImport ? true : writeFile(generateFile(QtQuickAppGeneratedFileInfo::MainQmlFile, errorMessage), path(MainQml)))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerPriFile, errorMessage), path(AppViewerPri))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerCppFile, errorMessage), path(AppViewerCpp))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::AppViewerHFile, errorMessage), path(AppViewerH))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::SymbianSvgIconFile, errorMessage), path(SymbianSvgIcon))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::MaemoPngIconFile, errorMessage), path(MaemoPngIcon))
            && writeFile(generateFile(QtQuickAppGeneratedFileInfo::DesktopFile, errorMessage), path(Desktop));
}

bool Html5App::generateFiles(QString *errorMessage) const
{
    return     writeFile(generateFile(Html5AppGeneratedFileInfo::MainCppFile, errorMessage), path(MainCpp))
            && writeFile(generateFile(Html5AppGeneratedFileInfo::AppProFile, errorMessage), path(AppPro))
            && (mainHtmlMode() != ModeGenerate ? true : writeFile(generateFile(Html5AppGeneratedFileInfo::MainHtmlFile, errorMessage), path(MainHtml)))
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
