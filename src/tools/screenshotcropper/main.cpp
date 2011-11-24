#include <QtGui>
#include "screenshotcropperwindow.h"

using namespace QtSupport::Internal;

const QString settingsKeyAreasXmlFile = QLatin1String("areasXmlFile");
const QString settingsKeyImagesFolder = QLatin1String("imagesFolder");

static void promptPaths(QString &areasXmlFile, QString &imagesFolder)
{
    QSettings settings(QLatin1String("Nokia"), QLatin1String("Qt Creator Screenshot Cropper"));

    areasXmlFile = settings.value(settingsKeyAreasXmlFile).toString();
    areasXmlFile = QFileDialog::getOpenFileName(0, QLatin1String("Select the 'images_areaofinterest.xml' file in Qt Creator's sources"), areasXmlFile);
    settings.setValue(settingsKeyAreasXmlFile, areasXmlFile);

    imagesFolder = settings.value(settingsKeyImagesFolder).toString();
    imagesFolder = QFileDialog::getExistingDirectory(0, QLatin1String("Select the 'doc/src/images' folder in Qt's sources"), imagesFolder);
    settings.setValue(settingsKeyImagesFolder, imagesFolder);
}

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QString areasXmlFile;
    QString imagesFolder;
    promptPaths(areasXmlFile, imagesFolder);

    ScreenShotCropperWindow w;
    w.show();
    w.loadData(areasXmlFile, imagesFolder);
    w.selectImage(0);

    return a.exec();
}
