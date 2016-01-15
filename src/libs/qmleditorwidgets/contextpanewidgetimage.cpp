/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "contextpanewidgetimage.h"

#include "ui_contextpanewidgetimage.h"
#include "ui_contextpanewidgetborderimage.h"
#include <qmljs/qmljspropertyreader.h>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsEffect>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSlider>
#include <QToolButton>
#include <QDebug>

namespace QmlEditorWidgets {

bool LabelFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonDblClick) {
        emit doubleClicked();
        event->accept();
        return true;
    }
    return QObject::eventFilter(obj, event);
}

bool WheelFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::Wheel) {
        if (obj
            && obj->isWidgetType()
            && obj != m_target) {
            QApplication::sendEvent(m_target, event);
            return true;
        }
    }
    return QObject::eventFilter(obj, event);
}

ContextPaneWidgetImage::ContextPaneWidgetImage(QWidget *parent, bool borderImage) :
    QWidget(parent),
    ui(0), uiBorderImage(0), previewWasVisible(false)
{
    LabelFilter *labelFilter = new LabelFilter(this);

    m_borderImage = borderImage;

    if (m_borderImage) {
        uiBorderImage = new Ui::ContextPaneWidgetBorderImage;
        uiBorderImage->setupUi(this);
        m_fileWidget = uiBorderImage->fileWidget;
        m_sizeLabel = uiBorderImage->sizeLabel;
        uiBorderImage->label->setToolTip(tr("Double click for preview."));
        uiBorderImage->label->installEventFilter(labelFilter);


        connect(uiBorderImage->verticalTileRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onVerticalStretchChanged);
        connect(uiBorderImage->verticalStretchRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onVerticalStretchChanged);
        connect(uiBorderImage->verticalTileRadioButtonNoCrop, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onVerticalStretchChanged);

        connect(uiBorderImage->horizontalTileRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onHorizontalStretchChanged);
        connect(uiBorderImage->horizontalStretchRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onHorizontalStretchChanged);
        connect(uiBorderImage->horizontalTileRadioButtonNoCrop, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onHorizontalStretchChanged);
        PreviewLabel *previewLabel = previewDialog()->previewLabel();
        connect(previewLabel, &PreviewLabel::leftMarginChanged,
                this, &ContextPaneWidgetImage::onLeftMarginsChanged);
        connect(previewLabel, &PreviewLabel::rightMarginChanged,
                this, &ContextPaneWidgetImage::onRightMarginsChanged);
        connect(previewLabel, &PreviewLabel::topMarginChanged,
                this, &ContextPaneWidgetImage::onTopMarginsChanged);
        connect(previewLabel, &PreviewLabel::bottomMarginChanged,
                this, &ContextPaneWidgetImage::onBottomMarginsChanged);

    } else {
        ui = new Ui::ContextPaneWidgetImage;
        ui->setupUi(this);
        ui->label->setToolTip(tr("Double click for preview."));
        ui->label->installEventFilter(labelFilter);
        m_fileWidget = ui->fileWidget;
        m_sizeLabel = ui->sizeLabel;

        connect(ui->stretchRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
        connect(ui->tileRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
        connect(ui->horizontalStretchRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
        connect(ui->verticalStretchRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
        connect(ui->preserveAspectFitRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
        connect(ui->cropAspectFitRadioButton, &QRadioButton::toggled,
                this, &ContextPaneWidgetImage::onStretchChanged);
    }
    previewDialog();
    m_fileWidget->setShowComboBox(true);
    m_fileWidget->setFilter(QLatin1String("*.png *.gif *.jpg"));

    connect(m_fileWidget, &FileWidget::fileNameChanged,
            this, &ContextPaneWidgetImage::onFileNameChanged);
    connect(labelFilter, &LabelFilter::doubleClicked,
            this, &ContextPaneWidgetImage::onPixmapDoubleClicked);

}

ContextPaneWidgetImage::~ContextPaneWidgetImage()
{
    if (ui)
        delete ui;
    if (uiBorderImage)
        delete uiBorderImage;
}

void ContextPaneWidgetImage::setProperties(QmlJS::PropertyReader *propertyReader)
{
    if (m_borderImage) {

        int leftBorder = 0;
        int rightBorder = 0;
        int topBorder = 0;
        int bottomBorder = 0;

        if (propertyReader->hasProperty(QLatin1String("border.left")))
            leftBorder =propertyReader->readProperty(QLatin1String("border.left")).toInt();
        if (propertyReader->hasProperty(QLatin1String("border.right")))
            rightBorder =propertyReader->readProperty(QLatin1String("border.right")).toInt();
        if (propertyReader->hasProperty(QLatin1String("border.top")))
            topBorder =propertyReader->readProperty(QLatin1String("border.top")).toInt();
        if (propertyReader->hasProperty(QLatin1String("border.bottom")))
            bottomBorder =propertyReader->readProperty(QLatin1String("border.bottom")).toInt();
        previewDialog()->previewLabel()->setMargins(leftBorder, topBorder, rightBorder, bottomBorder);

        if (propertyReader->hasProperty(QLatin1String("horizontalTileMode"))) {
            QString fillMode = propertyReader->readProperty(QLatin1String("horizontalTileMode")).toString();
            if (fillMode.contains(QLatin1String("BorderImage.")))
                fillMode.remove(QLatin1String("BorderImage."));

            uiBorderImage->horizontalStretchRadioButton->setChecked(true);

            if (fillMode == QLatin1String("Stretch"))
                uiBorderImage->horizontalStretchRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Repeat"))
                uiBorderImage->horizontalTileRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Round"))
                uiBorderImage->horizontalTileRadioButtonNoCrop->setChecked(true);
        } else {
            //uiBorderImage
            uiBorderImage->horizontalStretchRadioButton->setChecked(true);
        }
        if (propertyReader->hasProperty(QLatin1String("verticalTileMode"))) {
            QString fillMode = propertyReader->readProperty(QLatin1String("verticalTileMode")).toString();
            if (fillMode.contains(QLatin1String("BorderImage.")))
                fillMode.remove(QLatin1String("BorderImage."));

            uiBorderImage->verticalStretchRadioButton->setChecked(true);

            if (fillMode == QLatin1String("Stretch"))
                uiBorderImage->verticalStretchRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Repeat"))
                uiBorderImage->verticalTileRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Round"))
                uiBorderImage->verticalTileRadioButtonNoCrop->setChecked(true);
        } else {
            //uiBorderImage
            uiBorderImage->verticalStretchRadioButton->setChecked(true);
        }
    } else {
        if (propertyReader->hasProperty(QLatin1String("fillMode"))) {
            QString fillMode = propertyReader->readProperty(QLatin1String("fillMode")).toString();
            if (fillMode.contains(QLatin1String("Image.")))
                fillMode.remove(QLatin1String("Image."));

            ui->stretchRadioButton->setChecked(true);

            if (fillMode == QLatin1String("Image.Tile") || fillMode == QLatin1String("Tile"))
                ui->tileRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Image.TileVertically") || fillMode == QLatin1String("TileVertically"))
                ui->horizontalStretchRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Image.TileHorizontally") || fillMode == QLatin1String("TileHorizontally"))
                ui->verticalStretchRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Image.PreserveAspectFit") || fillMode == QLatin1String("PreserveAspectFit"))
                ui->preserveAspectFitRadioButton->setChecked(true);
            if (fillMode == QLatin1String("Image.PreserveAspectCrop") || fillMode == QLatin1String("PreserveAspectCrop"))
                ui->cropAspectFitRadioButton->setChecked(true);
        } else {
            ui->stretchRadioButton->setChecked(true);
        }
    }
    if (propertyReader->hasProperty(QLatin1String("source"))) {
        QString source = propertyReader->readProperty(QLatin1String("source")).toString();
        m_fileWidget->setFileName(source);
        if (QFile::exists(m_path + QLatin1Char('/') + source))
            setPixmap(m_path + QLatin1Char('/') + source);
        else
            setPixmap(source);
    } else {
        m_sizeLabel->clear();
        m_fileWidget->setFileName(QUrl());
        setPixmap(QString());
    }
}

void ContextPaneWidgetImage::setPath(const QString& path)
{
    m_path = path;
    m_fileWidget->setPath(QUrl::fromLocalFile(m_path));
}

void PreviewDialog::setZoom(int z)
{
    m_zoom = z;
    m_label->setZoom(z);
    switch (m_zoom) {
    case 1:
        m_zoomLabel->setText(QLatin1String("100%"));
        m_slider->setValue(1);
        break;
    case 2:
        m_zoomLabel->setText(QLatin1String("200%"));
        m_slider->setValue(2);
        break;
    case 4:
        m_zoomLabel->setText(QLatin1String("400%"));
        m_slider->setValue(3);
        break;
    case 6:
        m_zoomLabel->setText(QLatin1String("600%"));
        m_slider->setValue(4);
        break;
    case 8:
        m_zoomLabel->setText(QLatin1String("800%"));
        m_slider->setValue(5);
        break;
    case 10:
        m_zoomLabel->setText(QLatin1String("1000%"));
        m_slider->setValue(6);
        break;
    default: break;
    }
    setPixmap(m_pixmap, m_zoom);
}

void PreviewDialog::setIsBorderImage(bool b)
{
    m_borderImage = b;
    m_label->setIsBorderImage(b);
}

PreviewLabel *PreviewDialog::previewLabel() const
{
    return m_label;
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

void ContextPaneWidgetImage::onHorizontalStretchChanged()
{
    QString stretch;
    if (uiBorderImage->horizontalStretchRadioButton->isChecked())
        stretch = QLatin1String("BorderImage.Stretch");
    if (uiBorderImage->horizontalTileRadioButton->isChecked())
        stretch = QLatin1String("BorderImage.Repeat");
    if (uiBorderImage->horizontalTileRadioButtonNoCrop->isChecked())
        stretch = QLatin1String("BorderImage.Round");

    if (stretch == QLatin1String("BorderImage.Stretch"))
        emit removeProperty(QLatin1String("horizontalTileMode"));
    else
        emit propertyChanged(QLatin1String("horizontalTileMode"), stretch);
}

void ContextPaneWidgetImage::onVerticalStretchChanged()
{
    QString stretch;
    if (uiBorderImage->verticalStretchRadioButton->isChecked())
        stretch = QLatin1String("BorderImage.Stretch");
    if (uiBorderImage->verticalTileRadioButton->isChecked())
        stretch = QLatin1String("BorderImage.Repeat");
    if (uiBorderImage->verticalTileRadioButtonNoCrop->isChecked())
        stretch = QLatin1String("BorderImage.Round");

    if (stretch == QLatin1String("BorderImage.Stretch"))
        emit removeProperty(QLatin1String("verticalTileMode"));
    else
        emit propertyChanged(QLatin1String("verticalTileMode"), stretch);

}

void ContextPaneWidgetImage::onFileNameChanged()
{
    if (m_fileWidget->fileName().isNull())
        emit removeProperty(QLatin1String("source"));
    else
        emit propertyChanged(QLatin1String("source"), QString(QLatin1Char('\"') + m_fileWidget->fileName() + QLatin1Char('\"')));
}

void ContextPaneWidgetImage::onPixmapDoubleClicked()
{
    previewDialog()->setParent(parentWidget()->parentWidget());
    previewDialog()->setMaximumSize(previewDialog()->parentWidget()->size() - QSize(150, 100));
    if (m_borderImage)
        previewDialog()->setZoom(4);
    previewDialog()->setIsBorderImage(m_borderImage);

    QPoint p = parentWidget()->pos();
    p = p + QPoint(-2, -2);
    previewDialog()->show();
    previewDialog()->update();
    previewDialog()->move(p);
    //previewDialog()->adjustSize();
    if ((previewDialog()->pos().x() + previewDialog()->width()) > previewDialog()->parentWidget()->width())
        previewDialog()->move(previewDialog()->parentWidget()->width() - (previewDialog()->width()) - 40, p.y());

    if ((previewDialog()->pos().y() + previewDialog()->height()) > previewDialog()->parentWidget()->height())
        previewDialog()->move(previewDialog()->pos().x(), previewDialog()->parentWidget()->height() - (previewDialog()->height()) - 40);

    if (previewDialog()->pos().x() < 0)
        previewDialog()->move(0, previewDialog()->pos().y());
    if (previewDialog()->pos().y() < 0)
        previewDialog()->move(previewDialog()->pos().x(), 0);

    previewDialog()->raise();
}

void ContextPaneWidgetImage::onLeftMarginsChanged()
{
    if (previewDialog()->previewLabel()->leftMarging())
        propertyChanged(QLatin1String("border.left"), previewDialog()->previewLabel()->leftMarging());
    else
        emit removeProperty(QLatin1String("border.left"));
}

void ContextPaneWidgetImage::onRightMarginsChanged()
{
    if (previewDialog()->previewLabel()->rightMarging())
        propertyChanged(QLatin1String("border.right"), previewDialog()->previewLabel()->rightMarging());
    else
        emit removeProperty(QLatin1String("border.right"));


}

void ContextPaneWidgetImage::onTopMarginsChanged()
{
    if (previewDialog()->previewLabel()->topMarging())
        propertyChanged(QLatin1String("border.top"), previewDialog()->previewLabel()->topMarging());
    else
        emit removeProperty(QLatin1String("border.top"));
}

void ContextPaneWidgetImage::onBottomMarginsChanged()
{
    if (previewDialog()->previewLabel()->bottomMarging())
        propertyChanged(QLatin1String("border.bottom"), previewDialog()->previewLabel()->bottomMarging());
    else
        emit removeProperty(QLatin1String("border.bottom"));

}

static inline Qt::TileRule stringToRule(const QString &s)
{
    if (s == QLatin1String("Stretch"))
        return  Qt::StretchTile;
    if (s == QLatin1String("Repeat"))
        return  Qt::RepeatTile;
    if (s == QLatin1String("Round"))
        return Qt::RoundTile;

    qWarning("QDeclarativeGridScaledImage: Invalid tile rule specified. Using Stretch.");
    return  Qt::StretchTile;
}

static inline bool parseSciFile(const QString &fileName, QString &pixmapFileName, int &left, int &right, int &top, int &bottom, Qt::TileRule &horizontalTileRule, Qt::TileRule &verticalTileRule)
{
    int l = -1;
    int r = -1;
    int t = -1;
    int b = -1;
    QString imgFile;

    QFile data(fileName);
    if (!data.open(QIODevice::ReadOnly))
        return false;

    QByteArray raw;
    while (raw = data.readLine(), !raw.isEmpty()) {
        QString line = QString::fromUtf8(raw.trimmed());
        if (line.isEmpty() || line.startsWith(QLatin1Char('#')))
            continue;

        QStringList list = line.split(QLatin1Char(':'));
        if (list.count() != 2)
            return false;

        list[0] = list[0].trimmed();
        list[1] = list[1].trimmed();

        if (list[0] == QLatin1String("border.left"))
            l = list[1].toInt();
        else if (list[0] == QLatin1String("border.right"))
            r = list[1].toInt();
        else if (list[0] == QLatin1String("border.top"))
            t = list[1].toInt();
        else if (list[0] == QLatin1String("border.bottom"))
            b = list[1].toInt();
        else if (list[0] == QLatin1String("source"))
            imgFile = list[1];
        else if (list[0] == QLatin1String("horizontalTileRule"))
            horizontalTileRule = stringToRule(list[1]);
        else if (list[0] == QLatin1String("verticalTileRule"))
           verticalTileRule = stringToRule(list[1]);
    }

    if (l < 0 || r < 0 || t < 0 || b < 0 || imgFile.isEmpty())
        return false;

    left = l; right = r; top = t; bottom = b;

    pixmapFileName = imgFile;

    return true;
}

void ContextPaneWidgetImage::setPixmap(const QString &fileName)
{
    QPixmap pix(76,76);
    pix.fill(Qt::black);

    if (m_borderImage) {
        QString localFileName(fileName);
        if (QFile(fileName).exists()) {
            if (fileName.endsWith(QLatin1String("sci"))) {
                QString pixmapFileName;
                int left = 0;
                int right = 0;
                int top = 0;
                int bottom = 0;

                Qt::TileRule horizontalTileRule;
                Qt::TileRule verticalTileRule;
                if (parseSciFile(fileName, pixmapFileName, left, right, top, bottom, horizontalTileRule, verticalTileRule)) {
                    localFileName = QFileInfo(fileName).absoluteDir().absolutePath() + QLatin1Char('/') + pixmapFileName;
                    previewDialog()->previewLabel()->setMargins(left, top, right, bottom);
                } else { // sci file not parsed correctly
                    uiBorderImage->sizeLabel->clear();
                    return;
                }
            }
            QPixmap source(localFileName);
            if (source.isNull())
                source = pix;
            previewDialog()->setPixmap(source, previewDialog()->zoom());
            uiBorderImage->sizeLabel->setText(QString::number(source.width()) + QLatin1Char('x')
                                              + QString::number(source.height()));
            QPainter p(&pix);
            Qt::TileRule horizontalTileMode = Qt::StretchTile;
            Qt::TileRule verticalTileMode = Qt::StretchTile;
            if (uiBorderImage->horizontalTileRadioButton->isChecked())
                horizontalTileMode =Qt::RepeatTile;
            if (uiBorderImage->horizontalTileRadioButtonNoCrop->isChecked())
                horizontalTileMode =Qt::RoundTile;
            if (uiBorderImage->verticalTileRadioButton->isChecked())
                verticalTileMode =Qt::RepeatTile;
            if (uiBorderImage->verticalTileRadioButtonNoCrop->isChecked())
                verticalTileMode =Qt::RoundTile;
            QTileRules rules(horizontalTileMode, verticalTileMode);
            PreviewLabel *previewLabel = previewDialog()->previewLabel();
            QMargins margins(previewLabel->leftMarging() ,previewLabel->topMarging() ,previewLabel->rightMarging(), previewLabel->bottomMarging());
            qDrawBorderPixmap(&p, QRect(0, 0, 76, 76), margins, source, source.rect(), margins, rules);
            //p.drawPixmap(0,0,76,76, source);
        } else {
            uiBorderImage->sizeLabel->clear();
        }
        uiBorderImage->label->setPixmap(pix);
    } else {
        if (QFile(fileName).exists()) {
            QPixmap source(fileName);
            previewDialog()->setPixmap(source, 1);
            ui->sizeLabel->setText(QString::number(source.width()) + QLatin1Char('x') + QString::number(source.height()));
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
            ui->sizeLabel->clear();
        }

        ui->label->setPixmap(pix);

    }
}

PreviewDialog* ContextPaneWidgetImage::previewDialog()
{
    if (!m_previewDialog) {
        m_previewDialog = new PreviewDialog(this);
        m_previewDialog->hide();
    }

    return m_previewDialog.data();
}

void ContextPaneWidgetImage::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        if (ui)
            ui->retranslateUi(this);
        if (uiBorderImage)
            uiBorderImage->retranslateUi(this);
        break;
    default:
        break;
    }
}


void ContextPaneWidgetImage::hideEvent(QHideEvent * event)
{
    previewWasVisible = previewDialog()->isVisible();
    previewDialog()->hide();
    QWidget::hideEvent(event);
}

void ContextPaneWidgetImage::showEvent(QShowEvent* event)
{
    if (previewWasVisible)
        previewDialog()->show();
    QWidget::showEvent(event);
}

PreviewLabel::PreviewLabel(QWidget *parent)
    : QLabel(parent),
      m_dragging_left(false), m_dragging_right(false),
      m_dragging_top(false), m_dragging_bottom(false)

{
    m_zoom = 1;
    m_showBorders = false;
    m_left = 2;
    m_right = 2;
    m_top = 2;
    m_bottom = 2;
    setMouseTracking(true);
    setCursor(QCursor(Qt::ArrowCursor));
    m_hooverInfo = new QLabel(parentWidget());
    m_hooverInfo->hide();

    m_hooverInfo->setFrameShape(QFrame::StyledPanel);
    m_hooverInfo->setFrameShadow(QFrame::Sunken);
    QGraphicsDropShadowEffect *dropShadowEffect = new QGraphicsDropShadowEffect;
    dropShadowEffect->setBlurRadius(4);
    dropShadowEffect->setOffset(2, 2);
    m_hooverInfo->setGraphicsEffect(dropShadowEffect);
    m_hooverInfo->setAutoFillBackground(true);
    m_hooverInfo->raise();
}

void PreviewLabel::setZoom(int z)
{
    m_zoom = z;
}

void PreviewLabel::setIsBorderImage(bool b)
{
    m_borderImage = b;
}

void PreviewLabel::setMargins(int left, int top, int right, int bottom)
{
    m_left = left;
    m_top = top;
    m_right = right;
    m_bottom = bottom;
}

void PreviewLabel::paintEvent(QPaintEvent *event)
{
    QLabel::paintEvent(event);
    if (m_borderImage) {

        QPainter p(this);

        p.setOpacity(0.5);

        p.setBackgroundMode(Qt::TransparentMode);
        {
            QPen pen(Qt::SolidLine);
            pen.setColor("#F0F0F0");
            p.setPen(pen);

            if (m_left >= 0)
                p.drawLine(m_left * m_zoom, 4, m_left * m_zoom, height() - 4);
            if (m_right >= 0)
                p.drawLine(width() - m_right * m_zoom - 1, 4, width() - m_right * m_zoom - 1, height() - 4);
            if (m_top >= 0)
                p.drawLine(4, m_top * m_zoom, width() - 4, m_top * m_zoom);
            if (m_bottom >= 0)
                p.drawLine(4, height() - m_bottom * m_zoom - 1, width() - 4, height() - m_bottom * m_zoom - 1);
        }

        {
            QBrush brush(Qt::Dense4Pattern);
            brush.setColor("#101010");
            QPen pen(brush, 1, Qt::DotLine);
            pen.setColor("#101010");
            p.setPen(pen);

            if (m_left >= 0)
                p.drawLine(m_left * m_zoom, 4, m_left * m_zoom, height() - 4);
            if (m_right >= 0)
                p.drawLine(width() - m_right * m_zoom - 1, 4, width() - m_right * m_zoom - 1, height() - 4);
            if (m_top >= 0)
                p.drawLine(4, m_top * m_zoom, width() - 4, m_top * m_zoom);
            if (m_bottom >= 0)
                p.drawLine(4, height() - m_bottom * m_zoom - 1, width() - 4, height() - m_bottom * m_zoom - 1);
        }
    }
}

static inline bool rangeCheck(int target, int pos)
{
    return (target - 10 < pos) && (target + 10 > pos);
}

void PreviewLabel::mousePressEvent(QMouseEvent * event)
{
    if (!m_borderImage)
        return QLabel::mouseMoveEvent(event);

    bool bottom = false;

    if (event->button() == Qt::LeftButton) {
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
        if (rangeCheck(m_left * m_zoom, event->pos().x())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
            m_dragging_left = true;
            event->accept();
            m_hooverInfo->setText(QLatin1String("Left ") + QString::number(m_left));
            m_hooverInfo->show();
        } else if (rangeCheck(m_top * m_zoom, event->pos().y())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
            m_dragging_top = true;
            event->accept();
            m_hooverInfo->setText(QLatin1String("Top ") + QString::number(m_top));
            m_hooverInfo->show();
        } else if (rangeCheck(m_right * m_zoom, width() - event->pos().x())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
            m_dragging_right = true;
            event->accept();
            m_hooverInfo->setText(QLatin1String("Right ") + QString::number(m_right));
            m_hooverInfo->show();
        } else if (rangeCheck(m_bottom * m_zoom, height() - event->pos().y())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
            m_dragging_bottom = true;
            event->accept();
            m_hooverInfo->setText(QLatin1String("Bottom ") + QString::number(m_bottom));
            m_hooverInfo->show();
            bottom = true;
        } else {
            QLabel::mousePressEvent(event);
        }
        m_startPos = event->pos();
        if (bottom)
            m_hooverInfo->move(mapToParent(m_startPos) + QPoint(0, -10));
        else
            m_hooverInfo->move(mapToParent(m_startPos) + QPoint(0, 40));
        m_hooverInfo->resize(m_hooverInfo->sizeHint());
        m_hooverInfo->raise();
    }
}

void PreviewLabel::mouseReleaseEvent(QMouseEvent * event)
{
    if (!m_borderImage)
        return QLabel::mouseMoveEvent(event);

    if (m_dragging_left || m_dragging_top || m_dragging_right|| m_dragging_bottom) {
        m_hooverInfo->hide();
        if (m_dragging_left)
            emit leftMarginChanged();

        if (m_dragging_top)
            emit topMarginChanged();

        if (m_dragging_bottom)
            emit bottomMarginChanged();

        if (m_dragging_right)
            emit rightMarginChanged();

        m_dragging_left = false;
        m_dragging_top = false;
        m_dragging_right = false;
        m_dragging_bottom = false;
        QApplication::restoreOverrideCursor();
        event->accept();

    } else {
        QLabel::mouseReleaseEvent(event);
    }
}


static inline int limitPositive(int i)
{
    if (i >= 0)
        return i;
    return 0;
}

void PreviewLabel::mouseMoveEvent(QMouseEvent * event)
{
    if (!m_borderImage)
        return QLabel::mouseMoveEvent(event);

    QPoint p = event->pos();
    bool bottom = false;
    if (m_dragging_left) {
        m_left = p.x() / m_zoom;
        m_left = limitPositive(m_left);
        event->accept();
        m_hooverInfo->setText(QLatin1String("Left ") + QString::number(m_left));
        update();
    } else if (m_dragging_top) {
        m_top = p.y() / m_zoom;
        m_top = limitPositive(m_top);
        event->accept();
        m_hooverInfo->setText(QLatin1String("Top ") + QString::number(m_top));
        update();
    }  else if (m_dragging_right) {
        m_right = (width() - p.x()) / m_zoom;
        m_right = limitPositive(m_right);
        event->accept();
        m_hooverInfo->setText(QLatin1String("Right ") + QString::number(m_right));
        update();
    } else if (m_dragging_bottom) {
        m_bottom = (height() - p.y()) / m_zoom;
        m_bottom = limitPositive(m_bottom);
        event->accept();
        m_hooverInfo->setText(QLatin1String("Bottom ") + QString::number(m_bottom));
        bottom = true;
        update();
    } else if (rangeCheck(m_left * m_zoom, p.x())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        event->accept();
        m_hooverInfo->setText(QLatin1String("Left ") + QString::number(m_left));
        m_hooverInfo->show();
    } else if (rangeCheck(m_top * m_zoom, p.y())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        event->accept();
        m_hooverInfo->setText(QLatin1String("Top ") + QString::number(m_top));
        m_hooverInfo->show();
    } else if (rangeCheck(m_right * m_zoom, width() - p.x())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        event->accept();
        m_hooverInfo->setText(QLatin1String("Right ") + QString::number(m_right));
        m_hooverInfo->show();
    } else if (rangeCheck(m_bottom * m_zoom, height() - p.y())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        event->accept();
        m_hooverInfo->setText(QLatin1String("Bottom ") + QString::number(m_bottom));
        m_hooverInfo->show();
        bottom = true;
    } else {
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
        QLabel::mouseMoveEvent(event);
        m_hooverInfo->hide();
    }
    m_startPos = p;
    if (bottom)
        m_hooverInfo->move(mapToParent(p) + QPoint(0, -10));
    else
        m_hooverInfo->move(mapToParent(p) + QPoint(0, 40));
    m_hooverInfo->resize(m_hooverInfo->sizeHint());
    m_hooverInfo->raise();
}

void PreviewLabel::leaveEvent(QEvent* event )
{
    while (QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
    m_hooverInfo->hide();
    QLabel::leaveEvent(event);
}

PreviewDialog::PreviewDialog(QWidget *parent) : DragWidget(parent)
{
    m_borderImage = false;
    setAutoFillBackground(true);

    m_label = new PreviewLabel(this);
    m_slider = new QSlider(this);

    m_zoomLabel = new QLabel(this);

    setZoom(1);

    QVBoxLayout *layout = new QVBoxLayout(this);
    QHBoxLayout *horizontalLayout = new QHBoxLayout();
    QHBoxLayout *horizontalLayout2 = new QHBoxLayout();
    layout->setMargin(0);
    layout->setContentsMargins(2, 2, 2, 16);
    layout->setSpacing(4);
    QToolButton *toolButton = new QToolButton(this);
    QIcon icon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    toolButton->setIcon(icon);
    toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolButton->setFixedSize(icon.availableSizes().value(0) + QSize(4, 4));
    connect(toolButton, &QToolButton::clicked, this, &PreviewDialog::onTogglePane);

    QScrollArea *scrollArea = new QScrollArea(this);
    WheelFilter *wheelFilter = new WheelFilter(scrollArea);
    scrollArea->setWidget(m_label);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    m_slider->setOrientation(Qt::Horizontal);
    m_slider->setRange(1, 6);
    m_slider->setFixedWidth(80);
    m_zoomLabel->setFixedWidth(50);

    horizontalLayout->addWidget(toolButton);
    horizontalLayout->addSpacing(6);
    horizontalLayout->addWidget(m_slider);
    horizontalLayout->addSpacing(6);
    horizontalLayout->addWidget(m_zoomLabel);
    horizontalLayout->addStretch(1);

    layout->addLayout(horizontalLayout);
    horizontalLayout2->addSpacing(24);
    horizontalLayout2->addWidget(scrollArea);
    layout->addLayout(horizontalLayout2);

    wheelFilter->setTarget(this);

    connect(m_slider, &QSlider::valueChanged, this, &PreviewDialog::onSliderMoved);

    foreach (QWidget *childWidget, findChildren<QWidget*>()) {
        childWidget->installEventFilter(wheelFilter);
    }
}

void PreviewDialog::setPixmap(const QPixmap &p, int zoom)
{
    m_pixmap = p;
    if (!p.isNull())
        m_label->setPixmap(p.scaled(p.width() * zoom, p.height() * zoom));
    else
        m_label->setPixmap(QPixmap());
    m_label->adjustSize();
    m_zoom = zoom;
    m_label->setZoom(m_zoom);
    QSize size = m_label->pixmap()->size() + QSize(54, 44);
    if (size.width() < 180)
        size.setWidth(180);
    resize(size);
}

void PreviewDialog::wheelEvent(QWheelEvent* event)
{
    int delta = event->delta();
    event->accept();
    if (delta >  0) {
        if (m_zoom == 1)
            setZoom(2);
        else if (m_zoom == 2)
            setZoom(4);
        else if (m_zoom == 4)
            setZoom(6);
        else if (m_zoom == 6)
            setZoom(8);
        else if (m_zoom == 8)
            setZoom(10);
    } else {
        if (m_zoom == 10)
            setZoom(8);
        else if (m_zoom == 8)
            setZoom(6);
        else if (m_zoom == 6)
            setZoom(4);
        else if (m_zoom == 4)
            setZoom(2);
        else if (m_zoom == 2)
            setZoom(1);
    }
    setPixmap(m_pixmap, m_zoom);
}

void PreviewDialog::onTogglePane()
{
    hide();
}

void PreviewDialog::onSliderMoved(int value)
{
    switch (value) {
    case 1: setZoom(1); break;
    case 2: setZoom(2); break;
    case 3: setZoom(4); break;
    case 4: setZoom(6); break;
    case 5: setZoom(8); break;
    case 6: setZoom(10); break;
    default: break;
    }
}

} //QmlDesigner
