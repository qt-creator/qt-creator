#include "contextpanetextwidget.h"
#include "contextpanewidget.h"
#include "ui_contextpanetext.h"
#include <qmljs/qmljspropertyreader.h>
#include <QTimerEvent>

namespace QmlDesigner {

ContextPaneTextWidget::ContextPaneTextWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPaneTextWidget),
    m_fontSizeTimer(-1)
{
    ui->setupUi(this);
    ui->boldButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/bold-h-icon.png")));
    ui->italicButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/italic-h-icon.png")));
    ui->underlineButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/underline-h-icon.png")));
    ui->strikeoutButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/strikeout-h-icon.png")));

    ui->leftAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentleft-h-icon.png")));
    ui->centerHAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentcenterh-h-icon.png")));
    ui->rightAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentright-h-icon.png")));

    ui->centerVAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentmiddle-h-icon.png")));

    ui->bottomAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentbottom-h-icon.png")));
    ui->topAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmenttop-h-icon.png")));

    ui->colorButton->setShowArrow(false);
    ui->textColorButton->setShowArrow(false);

    connect(ui->colorButton, SIGNAL(toggled(bool)), this, SLOT(onColorButtonToggled(bool)));
    connect(ui->textColorButton, SIGNAL(toggled(bool)), this, SLOT(onTextColorButtonToggled(bool)));

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), SIGNAL(accepted(QColor)), this, SLOT(onColorDialogApplied(QColor)));
    connect(parentContextWidget->colorDialog(), SIGNAL(rejected()), this, SLOT(onColorDialogCancled()));

    connect(ui->fontSizeSpinBox, SIGNAL(valueChanged(int)), this, SLOT(onFontSizeChanged(int)));
    connect(ui->fontSizeSpinBox, SIGNAL(formatChanged()), this, SLOT(onFontFormatChanged()));

    connect(ui->boldButton, SIGNAL(toggled(bool)), this, SLOT(onBoldCheckedChanged(bool)));
    connect(ui->italicButton, SIGNAL(toggled(bool)), this, SLOT(onItalicCheckedChanged(bool)));
    connect(ui->underlineButton, SIGNAL(toggled(bool)), this, SLOT(onUnderlineCheckedChanged(bool)));
    connect(ui->strikeoutButton, SIGNAL(toggled(bool)), this, SLOT(onStrikeoutCheckedChanged(bool)));
    connect(ui->fontComboBox, SIGNAL(currentFontChanged(QFont)), this, SLOT(onCurrentFontChanged(QFont)));

    connect(ui->centerHAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onHorizontalAlignmentChanged()));
    connect(ui->leftAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onHorizontalAlignmentChanged()));
    connect(ui->rightAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onHorizontalAlignmentChanged()));

    connect(ui->centerVAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onVerticalAlignmentChanged()));
    connect(ui->topAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onVerticalAlignmentChanged()));
    connect(ui->bottomAlignmentButton, SIGNAL(toggled(bool)), this, SLOT(onVerticalAlignmentChanged()));

    connect(ui->styleComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(onStyleComboBoxChanged(QString)));
}

void ContextPaneTextWidget::setProperties(QmlJS::PropertyReader *propertyReader)
{
    if (propertyReader->hasProperty(QLatin1String("font.pointSize"))) {
        ui->fontSizeSpinBox->setValue(propertyReader->readProperty(QLatin1String("font.pointSize")).toInt());
        ui->fontSizeSpinBox->setIsPointSize(true);
    } else if (!propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        ui->fontSizeSpinBox->setValue(8);
        ui->fontSizeSpinBox->setIsPointSize(true);
        if (m_fontSizeTimer > 0) {
            killTimer(m_fontSizeTimer);
            m_fontSizeTimer = -1;
        }
    }

    if (propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        ui->fontSizeSpinBox->setValue(propertyReader->readProperty(QLatin1String("font.pixelSize")).toInt());
        ui->fontSizeSpinBox->setIsPixelSize(true);
    }

    if (propertyReader->hasProperty(QLatin1String("font.bold"))) {
        ui->boldButton->setChecked(propertyReader->readProperty(QLatin1String("font.bold")).toBool());
    } else {
        ui->boldButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("font.italic"))) {
        ui->italicButton->setChecked(propertyReader->readProperty(QLatin1String("font.italic")).toBool());
    } else {
        ui->italicButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("font.underline"))) {
        ui->underlineButton->setChecked(propertyReader->readProperty(QLatin1String("font.underline")).toBool());
    } else {
        ui->underlineButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("font.strikeout"))) {
        ui->strikeoutButton->setChecked(propertyReader->readProperty(QLatin1String("font.strikeout")).toBool());
    } else {
        ui->strikeoutButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("color"))) {
        ui->colorButton->setColor(propertyReader->readProperty("color").toString());
    } else {
        ui->colorButton->setColor(QLatin1String("black"));
    }

    if (propertyReader->hasProperty(QLatin1String("styleColor"))) {
        ui->textColorButton->setColor(propertyReader->readProperty("styleColor").toString());
    } else {
        ui->textColorButton->setColor(QLatin1String("black"));
    }

    if (propertyReader->hasProperty(QLatin1String("font.family"))) {
        QString familyName = propertyReader->readProperty(QLatin1String("font.family")).toString();
        QFont font;
        font.setFamily(familyName);
        ui->fontComboBox->setCurrentFont(font);
    }

    if (propertyReader->hasProperty(QLatin1String("horizontalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("horizontalAlignment")).toString();
        ui->leftAlignmentButton->setChecked(true);
        if (alignment == QLatin1String("Text.AlignHCenter") || alignment == "AlignHCenter")
            ui->centerHAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignRight") || alignment == QLatin1String("AlignRight"))
            ui->rightAlignmentButton->setChecked(true);
    } else {
        ui->leftAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("verticalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("verticalAlignment")).toString();
        ui->bottomAlignmentButton->setChecked(true);
        if (alignment == QLatin1String("Text.AlignVCenter") || alignment == QLatin1String("AlignVCenter"))
            ui->centerVAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignTop") || alignment == QLatin1String("AlignTop"))
            ui->topAlignmentButton->setChecked(true);
    } else {
        ui->topAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("style"))) {
        QString style = propertyReader->readProperty(QLatin1String("style")).toString();
        ui->styleComboBox->setCurrentIndex(0);
        if (style == QLatin1String("Text.Outline") || style == QLatin1String("Outline"))
            ui->styleComboBox->setCurrentIndex(1);
        if (style == QLatin1String("Text.Raised") || style == QLatin1String("Raised"))
            ui->styleComboBox->setCurrentIndex(2);
        else if (style == QLatin1String("Text.Sunken") || style == QLatin1String("Sunken"))
            ui->styleComboBox->setCurrentIndex(3);
    } else {
        ui->styleComboBox->setCurrentIndex(0);
    }
}

void ContextPaneTextWidget::setVerticalAlignmentVisible(bool b)
{
    ui->centerVAlignmentButton->setEnabled(b);
    ui->topAlignmentButton->setEnabled(b);
    ui->bottomAlignmentButton->setEnabled(b);
}

void ContextPaneTextWidget::setStyleVisible(bool b)
{
    ui->styleComboBox->setEnabled(b);
    ui->styleLabel->setEnabled(b);
    ui->textColorButton->setEnabled(b);
}

void ContextPaneTextWidget::onTextColorButtonToggled(bool flag)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    if (flag)
        ui->colorButton->setChecked(false);
    QPoint p = mapToGlobal(ui->textColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->textColorButton->color());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorButtonToggled(bool flag)
{
    if (flag)
        ui->textColorButton->setChecked(false);
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(ui->colorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->colorButton->color());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);    
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (ui->textColorButton->isChecked())
        emit  propertyChanged(QLatin1String("styleColor"),parentContextWidget->colorDialog()->color());; //write back color
    if (ui->colorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color());; //write back color
    ui->textColorButton->setChecked(false);
    ui->colorButton->setChecked(false);
}

void ContextPaneTextWidget::onColorDialogCancled()
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    ui->colorButton->setChecked(false);
    ui->colorButton->setChecked(false);
}

void ContextPaneTextWidget::onFontSizeChanged(int)
{  
    if (m_fontSizeTimer)
        killTimer(m_fontSizeTimer);
    m_fontSizeTimer = startTimer(200);
}

void ContextPaneTextWidget::onFontFormatChanged()
{
    int size = ui->fontSizeSpinBox->value();
    if (ui->fontSizeSpinBox->isPointSize()) {   
        emit removeAndChangeProperty(QLatin1String("font.pixelSize"), QLatin1String("font.pointSize"), size);
    } else {        
        emit removeAndChangeProperty(QLatin1String("font.pointSize"), QLatin1String("font.pixelSize"), size);
    }

}

void ContextPaneTextWidget::onBoldCheckedChanged(bool value)
{
    if (value)
        emit propertyChanged(QLatin1String("font.bold"), value);
    else
        emit removeProperty(QLatin1String("font.bold"));

}

void ContextPaneTextWidget::onItalicCheckedChanged(bool value)
{
    if (value)
        emit propertyChanged(QLatin1String("font.italic"), value);
    else
        emit removeProperty(QLatin1String("font.italic"));
}

void ContextPaneTextWidget::onUnderlineCheckedChanged(bool value)
{
    if (value)
        emit propertyChanged(QLatin1String("font.underline"), value);
    else
        emit removeProperty(QLatin1String("font.underline"));
}

void ContextPaneTextWidget::onStrikeoutCheckedChanged(bool value)
{
    if (value)
        emit propertyChanged(QLatin1String("font.strikeout"), value);
    else
        emit removeProperty(QLatin1String("font.strikeout"));
}

void ContextPaneTextWidget::onCurrentFontChanged(const QFont &font)
{
    font.family();
    emit propertyChanged(QLatin1String("font.family"), QVariant(QString('\"') + font.family() + QString('\"')));
}

void ContextPaneTextWidget::onHorizontalAlignmentChanged()
{
    QString alignment;
    if (ui->centerHAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignHCenter");
    else if (ui->leftAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignLeft");
    else if (ui->rightAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignRight");
    if (m_horizontalAlignment != alignment) {
        m_horizontalAlignment = alignment;
        if (alignment == QLatin1String("Text.AlignLeft"))
            emit removeProperty(QLatin1String("horizontalAlignment"));
        else
            emit propertyChanged(QLatin1String("horizontalAlignment"), alignment);
    }
}

void ContextPaneTextWidget::onStyleComboBoxChanged(const QString &style)
{
    if (style == QLatin1String("Normal"))
        emit removeProperty(QLatin1String("style"));
    else
        emit propertyChanged(QLatin1String("style"), QVariant(QLatin1String("Text.") + style));
}

void ContextPaneTextWidget::onVerticalAlignmentChanged()
{
    QString alignment;
    if (ui->centerVAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignVCenter");
    else if (ui->topAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignTop");
    else if (ui->bottomAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignBottom");
    if (m_verticalAlignment != alignment) {
        m_verticalAlignment = alignment;
        if (alignment == QLatin1String("Text.AlignBottom"))
            emit removeProperty(QLatin1String("verticalAlignment"));
        else
            emit propertyChanged(QLatin1String("verticalAlignment"), alignment);
    }
}


ContextPaneTextWidget::~ContextPaneTextWidget()
{
    delete ui;
}

void ContextPaneTextWidget::changeEvent(QEvent *e)
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

void ContextPaneTextWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_fontSizeTimer) {
        killTimer(m_fontSizeTimer);
        m_fontSizeTimer = -1;
        int value = ui->fontSizeSpinBox->value();
        if (ui->fontSizeSpinBox->isPointSize())
            emit propertyChanged(QLatin1String("font.pointSize"), value);
        else
            emit propertyChanged(QLatin1String("font.pixelSize"), value);
    }
}

} //QmlDesigner
