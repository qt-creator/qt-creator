#include "screenshotcropperwindow.h"
#include "ui_screenshotcropperwindow.h"
#include <QtGui/QListWidget>
#include <QtCore/QDebug>

using namespace QtSupport::Internal;

ScreenShotCropperWindow::ScreenShotCropperWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::ScreenShotCropperWindow)
{
    ui->setupUi(this);
    connect(ui->m_filenamesList, SIGNAL(currentRowChanged(int)), SLOT(selectImage(int)));
    connect(ui->m_cropImageView, SIGNAL(cropAreaChanged(QRect)), SLOT(setArea(QRect)));
    connect(ui->m_buttonBox, SIGNAL(accepted()), SLOT(saveData()));
    connect(ui->m_buttonBox, SIGNAL(rejected()), SLOT(close()));
}

ScreenShotCropperWindow::~ScreenShotCropperWindow()
{
    delete ui;
}

void ScreenShotCropperWindow::loadData(const QString &areasXmlFile, const QString &imagesFolder)
{
    m_areasOfInterestFile = areasXmlFile;
    m_areasOfInterest = ScreenshotCropper::loadAreasOfInterest(m_areasOfInterestFile);
    m_imagesFolder = imagesFolder;
    foreach (const QString &pngFile, m_areasOfInterest.keys())
        ui->m_filenamesList->addItem(pngFile);
}

void ScreenShotCropperWindow::selectImage(int index)
{
    const QString fileName = ui->m_filenamesList->item(index)->text();
    ui->m_cropImageView->setImage(QImage(m_imagesFolder + QLatin1Char('/') + fileName));
    ui->m_cropImageView->setArea(m_areasOfInterest.value(fileName));
}

void ScreenShotCropperWindow::setArea(const QRect &area)
{
    const QListWidgetItem *item = ui->m_filenamesList->currentItem();
    if (!item)
        return;
    const QString currentFile = item->text();
    m_areasOfInterest.insert(currentFile, area);
}

void ScreenShotCropperWindow::saveData()
{
    if (!ScreenshotCropper::saveAreasOfInterest(m_areasOfInterestFile, m_areasOfInterest))
        qFatal("Cannot write %s", qPrintable(m_areasOfInterestFile));
}
