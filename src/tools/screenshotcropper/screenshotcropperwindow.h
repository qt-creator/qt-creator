#ifndef SCREENSHOTCROPPERWINDOW_H
#define SCREENSHOTCROPPERWINDOW_H

#include <QMainWindow>
#include "screenshotcropper.h"

using namespace QtSupport::Internal;

namespace Ui {
class ScreenShotCropperWindow;
}

class ScreenShotCropperWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit ScreenShotCropperWindow(QWidget *parent = 0);
    ~ScreenShotCropperWindow();

    void loadData(const QString &areasXmlFile, const QString &imagesFolder);

public slots:
    void selectImage(int index);
    void setArea(const QRect &area);
    void saveData();

private:
    AreasOfInterest m_areasOfInterest;
    QString m_areasOfInterestFile;
    QString m_imagesFolder;
    Ui::ScreenShotCropperWindow *ui;
};

#endif // SCREENSHOTCROPPERWINDOW_H
