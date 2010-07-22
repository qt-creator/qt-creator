#include "contextpanewidgetimage.h"
#include "ui_contextpanewidgetimage.h"
#include <qmljs/qmljspropertyreader.h>
#include <QFile>
#include <QPixmap>
#include <QPainter>

namespace QmlDesigner {

ContextPaneWidgetImage::ContextPaneWidgetImage(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPaneWidgetImage)
{
    ui->setupUi(this);
    ui->fileWidget->setShowComboBox(true);
    ui->fileWidget->setFilter("*.png *.gif *.jpg");

    connect(ui->stretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
    connect(ui->tileRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
    connect(ui->horizontalStretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
    connect(ui->verticalStretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
    connect(ui->preserveAspectFitRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
    connect(ui->cropAspectFitRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));

    connect(ui->fileWidget, SIGNAL(fileNameChanged(QUrl)), this, SLOT(onFileNameChanged()));
}

ContextPaneWidgetImage::~ContextPaneWidgetImage()
{
    delete ui;
}

void ContextPaneWidgetImage::setProperties(QmlJS::PropertyReader *propertyReader)
{

    if (propertyReader->hasProperty(QLatin1String("fillMode"))) {
        QString fillMode = propertyReader->readProperty(QLatin1String("fillMode")).toString();
        if (fillMode.contains("Image."))
            fillMode.remove("Image.");

        ui->stretchRadioButton->setChecked(true);

        if (fillMode == "Image.Tile" || fillMode == "Tile")
            ui->tileRadioButton->setChecked(true);
        if (fillMode == "Image.TileVertically" || fillMode == "TileVertically")
            ui->horizontalStretchRadioButton->setChecked(true);
        if (fillMode == "Image.TileHorizontally" || fillMode == "TileHorizontally")
            ui->verticalStretchRadioButton->setChecked(true);
        if (fillMode == "Image.PreserveAspectFit" || fillMode == "PreserveAspectFit")
            ui->preserveAspectFitRadioButton->setChecked(true);
        if (fillMode == "Image.PreserveAspectCrop" || fillMode == "PreserveAspectCrop")
            ui->cropAspectFitRadioButton->setChecked(true);
    } else {
        ui->stretchRadioButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("source"))) {
        QString source = propertyReader->readProperty(QLatin1String("source")).toString();
        ui->fileWidget->setFileName(source);
        if (QFile::exists(m_path + '/' + source))
            setPixmap(m_path + '/' + source);
        else
            setPixmap(source);
    } else {
        ui->sizeLabel->setText("");
    }
}

void ContextPaneWidgetImage::setPath(const QString& path)
{
    m_path = path;
    ui->fileWidget->setPath(QUrl::fromLocalFile(m_path));
}

void ContextPaneWidgetImage::onStretchChanged()
{
    QString stretch;
    if (ui->stretchRadioButton->isChecked())
        stretch = QLatin1String("Image.Stretch");
    else if (ui->tileRadioButton->isChecked())
        stretch = QLatin1String("Image.Tile");
    else if (ui->horizontalStretchRadioButton->isChecked())
        stretch = QLatin1String("Image.TileVertically");
    else if (ui->verticalStretchRadioButton->isChecked())
        stretch = QLatin1String("Image.TileHorizontally");
    else if (ui->preserveAspectFitRadioButton->isChecked())
        stretch = QLatin1String("Image.PreserveAspectFit");
    else if (ui->cropAspectFitRadioButton->isChecked())
        stretch = QLatin1String("Image.PreserveAspectCrop");

    if (stretch == QLatin1String("Image.Stretch"))
        emit removeProperty(QLatin1String("fillMode"));
    else
        emit propertyChanged(QLatin1String("fillMode"), stretch);
}

void ContextPaneWidgetImage::onFileNameChanged()
{
    if (ui->fileWidget->fileName().isNull())
        emit removeProperty(QLatin1String("source"));
    else
        emit propertyChanged(QLatin1String("source"), QString(QLatin1Char('\"') + ui->fileWidget->fileName() + QLatin1Char('\"')));
}

void ContextPaneWidgetImage::setPixmap(const QString &fileName)
{
    QPixmap pix(76,76);
    pix.fill(Qt::black);

    if (QFile(fileName).exists()) {
        QPixmap source(fileName);
        ui->sizeLabel->setText(QString::number(source.width()) + 'x' + QString::number(source.height()));
        QPainter p(&pix);
        if (ui->stretchRadioButton->isChecked()) {
            p.drawPixmap(0,0,76,76, source);
        } else if (ui->tileRadioButton->isChecked()) {
            QPixmap small = source.scaled(38,38);
            p.drawTiledPixmap(0,0,76,76, small);
        } else if (ui->horizontalStretchRadioButton->isChecked()) {
            QPixmap small = source.scaled(38,38);
            QPixmap half = pix.scaled(38, 76);
            QPainter p2(&half);
            p2.drawTiledPixmap(0,0,38,76, small);
            p.drawPixmap(0,0,76,76, half);
        } else if (ui->verticalStretchRadioButton->isChecked()) {
            QPixmap small = source.scaled(38,38);
            QPixmap half = pix.scaled(76, 38);
            QPainter p2(&half);
            p2.drawTiledPixmap(0,0,76,38, small);
            p.drawPixmap(0,0,76,76, half);
        } else if (ui->preserveAspectFitRadioButton->isChecked()) {
            QPixmap preserved = source.scaledToWidth(76);
            int offset = (76 - preserved.height()) / 2;
            p.drawPixmap(0, offset, 76, preserved.height(), source);
        } else if (ui->cropAspectFitRadioButton->isChecked()) {
            QPixmap cropped = source.scaledToHeight(76);
            int offset = (76 - cropped.width()) / 2;
            p.drawPixmap(offset, 0, cropped.width(), 76, source);
        }
    } else {
        ui->sizeLabel->setText("");
    }

    ui->label->setPixmap(pix);
}

void ContextPaneWidgetImage::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

} //QmlDesigner
