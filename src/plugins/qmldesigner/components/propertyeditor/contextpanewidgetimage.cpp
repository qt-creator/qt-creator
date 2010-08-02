#include "contextpanewidgetimage.h"
#include "ui_contextpanewidgetimage.h"
#include "ui_contextpanewidgetborderimage.h"
#include <qmljs/qmljspropertyreader.h>
#include <QFile>
#include <QPixmap>
#include <QPainter>
#include <QGraphicsEffect>
#include <QMouseEvent>
#include <QScrollArea>
#include <QSlider>
#include <QDebug>

namespace QmlDesigner {

bool LabelFilter::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::MouseButtonPress) {
        return true;
    }
    if (event->type() == QEvent::MouseButtonRelease) {
        return true;
    }
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
        uiBorderImage->label->setToolTip(tr("double click for preview"));
        uiBorderImage->label->installEventFilter(labelFilter);


        connect(uiBorderImage->verticalTileRadioButton,  SIGNAL(toggled(bool)), this, SLOT(onVerticalStretchChanged()));
        connect(uiBorderImage->verticalStretchRadioButton,  SIGNAL(toggled(bool)), this, SLOT(onVerticalStretchChanged()));
        connect(uiBorderImage->verticalTileRadioButtonNoCrop,  SIGNAL(toggled(bool)), this, SLOT(onVerticalStretchChanged()));

        connect(uiBorderImage->horizontalTileRadioButton,  SIGNAL(toggled(bool)), this, SLOT(onHorizontalStretchChanged()));
        connect(uiBorderImage->horizontalStretchRadioButton,  SIGNAL(toggled(bool)), this, SLOT(onHorizontalStretchChanged()));
        connect(uiBorderImage->horizontalTileRadioButtonNoCrop, SIGNAL(toggled(bool)), this, SLOT(onHorizontalStretchChanged()));
        connect(previewDialog()->previewLabel(), SIGNAL(leftMarginChanged()), this, SLOT(onLeftMarginsChanged()));
        connect(previewDialog()->previewLabel(), SIGNAL(rightMarginChanged()), this, SLOT(onRightMarginsChanged()));
        connect(previewDialog()->previewLabel(), SIGNAL(topMarginChanged()), this, SLOT(onTopMarginsChanged()));
        connect(previewDialog()->previewLabel(), SIGNAL(bottomMarginChanged()), this, SLOT(onBottomMarginsChanged()));

    } else {
        ui = new Ui::ContextPaneWidgetImage;
        ui->setupUi(this);
        ui->label->setToolTip(tr("double click for preview"));
        ui->label->installEventFilter(labelFilter);        
        m_fileWidget = ui->fileWidget;
        m_sizeLabel = ui->sizeLabel;

        connect(ui->stretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
        connect(ui->tileRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
        connect(ui->horizontalStretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
        connect(ui->verticalStretchRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
        connect(ui->preserveAspectFitRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));
        connect(ui->cropAspectFitRadioButton, SIGNAL(toggled(bool)), this, SLOT(onStretchChanged()));        
    }
    previewDialog();
    m_fileWidget->setShowComboBox(true);
    m_fileWidget->setFilter("*.png *.gif *.jpg");

    connect(m_fileWidget, SIGNAL(fileNameChanged(QUrl)), this, SLOT(onFileNameChanged()));
    connect(labelFilter, SIGNAL(doubleClicked()), this, SLOT(onPixmapDoubleClicked()));

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
        if (propertyReader->hasProperty(QLatin1String("horizontalTileMode"))) {
            QString fillMode = propertyReader->readProperty(QLatin1String("horizontalTileMode")).toString();
            if (fillMode.contains("BorderImage."))
                fillMode.remove("BorderImage.");

            uiBorderImage->horizontalStretchRadioButton->setChecked(true);

            if (fillMode == "Stretch")
                uiBorderImage->horizontalStretchRadioButton->setChecked(true);
            if (fillMode == "Repeat")
                uiBorderImage->horizontalTileRadioButton->setChecked(true);
            if (fillMode == "Round")
                uiBorderImage->horizontalTileRadioButtonNoCrop->setChecked(true);
        } else {
            //uiBorderImage
            uiBorderImage->horizontalStretchRadioButton->setChecked(true);
        }
        if (propertyReader->hasProperty(QLatin1String("verticalTileMode"))) {
            QString fillMode = propertyReader->readProperty(QLatin1String("verticalTileMode")).toString();
            if (fillMode.contains("BorderImage."))
                fillMode.remove("BorderImage.");

            uiBorderImage->verticalStretchRadioButton->setChecked(true);

            if (fillMode == "Stretch")
                uiBorderImage->verticalStretchRadioButton->setChecked(true);
            if (fillMode == "Repeat")
                uiBorderImage->verticalTileRadioButton->setChecked(true);
            if (fillMode == "Round")
                uiBorderImage->verticalTileRadioButtonNoCrop->setChecked(true);
        } else {
            //uiBorderImage
            uiBorderImage->verticalStretchRadioButton->setChecked(true);
        }
    } else {
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
    }
    if (propertyReader->hasProperty(QLatin1String("source"))) {
        QString source = propertyReader->readProperty(QLatin1String("source")).toString();
        m_fileWidget->setFileName(source);
        if (QFile::exists(m_path + '/' + source))
            setPixmap(m_path + '/' + source);
        else
            setPixmap(source);
    } else {
        m_sizeLabel->setText("");
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


void ContextPaneWidgetImage::setPixmap(const QString &fileName)
{
    QPixmap pix(76,76);
    pix.fill(Qt::black);

    if (m_borderImage) {
        if (QFile(fileName).exists()) {
            QPixmap source(fileName);
            previewDialog()->setPixmap(source, previewDialog()->zoom());
            uiBorderImage->sizeLabel->setText(QString::number(source.width()) + 'x' + QString::number(source.height()));
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
            QMargins margins(previewDialog()->previewLabel()->leftMarging() ,previewDialog()->previewLabel()->topMarging() ,previewDialog()->previewLabel()->rightMarging(), previewDialog()->previewLabel()->bottomMarging());
            qDrawBorderPixmap(&p, QRect(0, 0, 76, 76), margins, source, source.rect(), margins, rules);
            //p.drawPixmap(0,0,76,76, source);
        } else {
            uiBorderImage->sizeLabel->setText("");
        }
        uiBorderImage->label->setPixmap(pix);
    } else {
        if (QFile(fileName).exists()) {
            QPixmap source(fileName);
            previewDialog()->setPixmap(source, 1);
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

PreviewLabel::PreviewLabel(QWidget *parent) : QLabel(parent), m_dragging_top(false), m_dragging_bottom(false),
                                                              m_dragging_left(false), m_dragging_right(false)
{
    m_zoom = 1;
    m_showBorders = false;
    m_left = 2;
    m_right = 2;
    m_top = 2;
    m_bottom = 2;
    setMouseTracking(true);
    setCursor(QCursor(Qt::ArrowCursor));
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

            p.drawLine(m_left * m_zoom, 4, m_left * m_zoom, height() - 4);
            p.drawLine(width() - m_right * m_zoom, 4, width() - m_right * m_zoom, height() - 4);

            p.drawLine(4, m_top * m_zoom, width() - 4, m_top * m_zoom);
            p.drawLine(4, height() - m_bottom * m_zoom, width() - 4, height() - m_bottom * m_zoom);
        }

        {
            QBrush brush(Qt::Dense4Pattern);
            brush.setColor("#101010");
            QPen pen(brush, 1, Qt::DotLine);
            pen.setColor("#101010");
            p.setPen(pen);

            p.drawLine(m_left * m_zoom, 4, m_left * m_zoom, height() - 4);
            p.drawLine(width() - m_right * m_zoom, 4, width() - m_right * m_zoom, height() - 4);

            p.drawLine(4, m_top * m_zoom, width() - 4, m_top * m_zoom);
            p.drawLine(4, height() - m_bottom * m_zoom, width() - 4, height() - m_bottom * m_zoom);
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

    if (event->button() == Qt::LeftButton) {
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
        if (rangeCheck(m_left * m_zoom, event->pos().x())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
            m_dragging_left = true;
            event->accept();
        } else if (rangeCheck(m_top * m_zoom, event->pos().y())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
            m_dragging_top = true;
            event->accept();
        } else if (rangeCheck(m_right * m_zoom, width() - event->pos().x())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
            m_dragging_right = true;
            event->accept();
        } else if (rangeCheck(m_bottom * m_zoom, height() - event->pos().y())) {
            QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
            m_dragging_bottom = true;
            event->accept();
        } else {
            QLabel::mousePressEvent(event);
        }
        m_startPos = event->pos();
    }
}

void PreviewLabel::mouseReleaseEvent(QMouseEvent * event)
{
    if (!m_borderImage)
        return QLabel::mouseMoveEvent(event);

    if (m_dragging_left || m_dragging_top || m_dragging_right|| m_dragging_bottom) {

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


static inline int limit(int i, int zoom)
{
    static bool flag1 = 1;
    static bool flag2 = 1;
    if (zoom == 1)
        return i;
    if (i < 0) {
        int v = i / zoom;
        if (v)
            return v;
        if (zoom == 2) {
            flag1 =!flag1;
            return flag1 ? -1 : 0;
        }
        flag1 =!flag1;
        if (flag1)
            flag2 =!flag2;

        return flag1 && flag2 ? -1 : 0;
    }
    if (i > 0) {
        int v = i / zoom;
        if (v)
            return v;
        if (zoom == 2) {
            flag1 =!flag1;
            return flag1 ? 1 : 0;
        }
        flag1 =!flag1;
        if (flag1)
            flag2 =!flag2;

        return flag1 && flag2 ? 1 : 0;
    }
    return 0;
}

void PreviewLabel::mouseMoveEvent(QMouseEvent * event)
{
    if (!m_borderImage)
        return QLabel::mouseMoveEvent(event);

    QPoint p = event->pos();
    if (m_dragging_left) {
        m_left += limit(p.x() - m_startPos.x(), m_zoom);
        event->accept();       
        update();
    } else if (m_dragging_top) {
        m_top += limit(p.y() - m_startPos.y(), m_zoom);
        event->accept();
        update();
    }  else if (m_dragging_right) {
        m_right += limit(m_startPos.x() - p.x(), m_zoom);
        event->accept();        
        update();
    } else if (m_dragging_bottom) {
        m_bottom += limit(m_startPos.y() - p.y(), m_zoom);
        event->accept();        
        update();
    } else if (rangeCheck(m_left * m_zoom, p.x())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        event->accept();
    } else if (rangeCheck(m_top * m_zoom, p.y())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        event->accept();
    } else if (rangeCheck(m_right * m_zoom, width() - p.x())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeHorCursor));
        event->accept();
    } else if (rangeCheck(m_bottom * m_zoom, height() - p.y())) {
        QApplication::setOverrideCursor(QCursor(Qt::SizeVerCursor));
        event->accept();
    } else {
        if (QApplication::overrideCursor())
            QApplication::restoreOverrideCursor();
        QLabel::mouseMoveEvent(event);
    }
    m_startPos = p;
}

void PreviewLabel::leaveEvent(QEvent* event )
{
    while (QApplication::overrideCursor())
        QApplication::restoreOverrideCursor();
    QLabel::leaveEvent(event);
}

PreviewDialog::PreviewDialog(QWidget *parent) : DragWidget(parent)
{
    m_zoom = 1;
    m_borderImage = false;
    setAutoFillBackground(true);

    m_label = new PreviewLabel(this);
    m_slider = new QSlider(this);

    QGridLayout *layout = new QGridLayout(this);
    layout->setMargin(0);
    layout->setContentsMargins(2, 2, 2, 6);
    layout->setSpacing(4);
    QToolButton *toolButton = new QToolButton(this);
    QIcon icon(style()->standardIcon(QStyle::SP_DockWidgetCloseButton));
    toolButton->setIcon(icon);
    toolButton->setToolButtonStyle(Qt::ToolButtonIconOnly);
    toolButton->setFixedSize(icon.availableSizes().value(0) + QSize(4, 4));
    connect(toolButton, SIGNAL(clicked()), this, SLOT(onTogglePane()));
    layout->addWidget(toolButton, 0, 0, 1, 1);        

    QScrollArea *scrollArea = new QScrollArea(this);
    WheelFilter *wheelFilter = new WheelFilter(scrollArea);
    //scrollArea->installEventFilter(wheelFilter);
    scrollArea->setWidget(m_label);
    scrollArea->setFrameStyle(QFrame::NoFrame);
    m_slider->setOrientation(Qt::Horizontal);
    m_slider->setMaximumWidth(120);
    //layout->addItem(new QSpacerItem(10, 10), 0, 1, 1, 1);
    layout->addWidget(m_slider, 0, 1, 1, 2);
    layout->addWidget(scrollArea, 1, 1, 2, 2);

    wheelFilter->setTarget(this);

    foreach (QWidget *childWidget, findChildren<QWidget*>()) {
        childWidget->installEventFilter(wheelFilter);
    }
}

void PreviewDialog::setPixmap(const QPixmap &p, int zoom)
{
    m_pixmap = p;
    m_label->setPixmap(p.scaled(p.width() * zoom, p.height() * zoom));
    m_label->adjustSize();
    m_zoom = zoom;
    m_label->setZoom(m_zoom);
    QSize size = m_label->pixmap()->size() + QSize(44, 44);
    if (size.width() < 140)
        size.setWidth(140);
    resize(size);
}

void PreviewDialog::wheelEvent(QWheelEvent* event)
{
    int delta = event->delta();
    event->accept();
    if (delta >  0) {
        if (m_zoom == 1)
            m_zoom = 2;
        else if (m_zoom == 2)
            m_zoom = 4;
        else if (m_zoom == 4)
            m_zoom = 6;
        else if (m_zoom == 6)
            m_zoom = 8;
        else if (m_zoom == 8)
            m_zoom = 10;
    } else {
        if (m_zoom == 10)
            m_zoom = 8;
        else if (m_zoom == 8)
            m_zoom = 6;
        else if (m_zoom == 6)
            m_zoom = 4;
        else if (m_zoom == 4)
            m_zoom = 2;
        else if (m_zoom == 2)
            m_zoom = 1;
    }
    setPixmap(m_pixmap, m_zoom);
}

void PreviewDialog::onTogglePane()
{
    hide();
}

} //QmlDesigner
