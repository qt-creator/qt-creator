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

#include "contextpanetextwidget.h"
#include "contextpanewidget.h"
#include "customcolordialog.h"
#include "ui_contextpanetext.h"
#include <qmljs/qmljspropertyreader.h>
#include <QTimerEvent>
#include <QVariant>
namespace QmlEditorWidgets {

ContextPaneTextWidget::ContextPaneTextWidget(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ContextPaneTextWidget),
    m_fontSizeTimer(-1)
{
    ui->setupUi(this);
    ui->boldButton->setIcon(QIcon::fromTheme(QLatin1String("format-text-bold"),
            QIcon(QLatin1String(":/qmldesigner/images/bold-h-icon.png"))));
    ui->italicButton->setIcon(QIcon::fromTheme(QLatin1String("format-text-italic"),
            QIcon(QLatin1String(":/qmldesigner/images/italic-h-icon.png"))));
    ui->underlineButton->setIcon(QIcon::fromTheme(QLatin1String("format-text-underline"),
            QIcon(QLatin1String(":/qmldesigner/images/underline-h-icon.png"))));
    ui->strikeoutButton->setIcon(QIcon::fromTheme(QLatin1String("format-text-strikethrough"),
            QIcon(QLatin1String(":/qmldesigner/images/strikeout-h-icon.png"))));

    ui->leftAlignmentButton->setIcon(QIcon::fromTheme(QLatin1String("format-justify-left"),
            QIcon(QLatin1String(":/qmldesigner/images/alignmentleft-h-icon.png"))));
    ui->centerHAlignmentButton->setIcon(QIcon::fromTheme(QLatin1String("format-justify-center"),
            QIcon(QLatin1String(":/qmldesigner/images/alignmentcenterh-h-icon.png"))));
    ui->rightAlignmentButton->setIcon(QIcon::fromTheme(QLatin1String("format-justify-right"),
            QIcon(QLatin1String(":/qmldesigner/images/alignmentright-h-icon.png"))));

    ui->centerVAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentmiddle-h-icon.png")));

    ui->bottomAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmentbottom-h-icon.png")));
    ui->topAlignmentButton->setIcon(QIcon(QLatin1String(":/qmldesigner/images/alignmenttop-h-icon.png")));

    ui->colorButton->setShowArrow(false);
    ui->textColorButton->setShowArrow(false);

    connect(ui->colorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneTextWidget::onColorButtonToggled);
    connect(ui->textColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneTextWidget::onTextColorButtonToggled);

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::accepted,
            this, &ContextPaneTextWidget::onColorDialogApplied);
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::rejected,
            this, &ContextPaneTextWidget::onColorDialogCancled);

    connect(ui->fontSizeSpinBox,
            static_cast<void (QmlEditorWidgets::FontSizeSpinBox::*)(int)>(&QmlEditorWidgets::FontSizeSpinBox::valueChanged),
            this, &ContextPaneTextWidget::onFontSizeChanged);
    connect(ui->fontSizeSpinBox, &QmlEditorWidgets::FontSizeSpinBox::formatChanged,
            this, &ContextPaneTextWidget::onFontFormatChanged);

    connect(ui->boldButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onBoldCheckedChanged);
    connect(ui->italicButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onItalicCheckedChanged);
    connect(ui->underlineButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onUnderlineCheckedChanged);
    connect(ui->strikeoutButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onStrikeoutCheckedChanged);
    connect(ui->fontComboBox, &QFontComboBox::currentFontChanged,
            this, &ContextPaneTextWidget::onCurrentFontChanged);

    connect(ui->centerHAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);
    connect(ui->leftAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);
    connect(ui->rightAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);

    connect(ui->centerVAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);
    connect(ui->topAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);
    connect(ui->bottomAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);

    connect(ui->styleComboBox, static_cast<void (QComboBox::*)(const QString &)>(&QComboBox::currentIndexChanged),
            this, &ContextPaneTextWidget::onStyleComboBoxChanged);
}

static inline bool checkIfBoolean(const QVariant &v)
{
    return (v.toString() == QLatin1String("true") || v.toString() == QLatin1String("false"));
}

void ContextPaneTextWidget::setProperties(QmlJS::PropertyReader *propertyReader)
{
    if (propertyReader->hasProperty(QLatin1String("font.pointSize"))) {
        QVariant variant = propertyReader->readProperty(QLatin1String("font.pointSize"));
        bool b;
        ui->fontSizeSpinBox->setValue(variant.toInt(&b));
        ui->fontSizeSpinBox->setEnabled(b);
        ui->fontSizeSpinBox->setIsPointSize(true);
    } else if (!propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        ui->fontSizeSpinBox->setValue(8);
        ui->fontSizeSpinBox->setIsPointSize(true);
    }

    if (m_fontSizeTimer > 0) {
        killTimer(m_fontSizeTimer);
        m_fontSizeTimer = -1;
    }

    if (propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        QVariant variant = propertyReader->readProperty(QLatin1String("font.pixelSize"));
        bool b;
        ui->fontSizeSpinBox->setValue(variant.toInt(&b));

        ui->fontSizeSpinBox->setEnabled(b);
        ui->fontSizeSpinBox->setIsPixelSize(true);
    }

    ui->boldButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.bold"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.bold"));
        if (checkIfBoolean(v))
            ui->boldButton->setChecked(v.toBool());
        else
            ui->boldButton->setEnabled(false);
    } else {
        ui->boldButton->setChecked(false);
    }

    ui->italicButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.italic"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.italic"));
        if (checkIfBoolean(v))
            ui->italicButton->setChecked(v.toBool());
        else
            ui->italicButton->setEnabled(false);
    } else {
        ui->italicButton->setChecked(false);
    }

    ui->underlineButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.underline"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.underline"));
        if (checkIfBoolean(v))
            ui->underlineButton->setChecked(v.toBool());
        else
            ui->underlineButton->setEnabled(false);
    } else {
        ui->underlineButton->setChecked(false);
    }

    ui->strikeoutButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.strikeout"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.strikeout"));
        if (checkIfBoolean(v))
            ui->strikeoutButton->setChecked(v.toBool());
        else
            ui->strikeoutButton->setEnabled(false);
    } else {
        ui->strikeoutButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("color")))
        ui->colorButton->setColor(propertyReader->readProperty(QLatin1String("color")).toString());
    else
        ui->colorButton->setColor(QLatin1String("black"));

    if (propertyReader->hasProperty(QLatin1String("styleColor")))
        ui->textColorButton->setColor(propertyReader->readProperty(QLatin1String("styleColor")).toString());
    else
        ui->textColorButton->setColor(QLatin1String("black"));

    if (propertyReader->hasProperty(QLatin1String("font.family"))) {
        QString familyName = propertyReader->readProperty(QLatin1String("font.family")).toString();
        QFont font;
        font.setFamily(familyName);

        ui->fontComboBox->setCurrentFont(font);
        if (propertyReader->isBindingOrEnum(QLatin1String("font.family")))
            ui->fontComboBox->setEnabled(false);
        else
            ui->fontComboBox->setEnabled(true);
    }

    if (propertyReader->hasProperty(QLatin1String("horizontalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("horizontalAlignment")).toString();
        ui->leftAlignmentButton->setChecked(true);
        ui->leftAlignmentButton->setEnabled(true);
        if (alignment == QLatin1String("Text.AlignHCenter") || alignment == QLatin1String("AlignHCenter"))
            ui->centerHAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignRight") || alignment == QLatin1String("AlignRight"))
            ui->rightAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignLeft") || alignment == QLatin1String("AlignLeft"))
            ui->leftAlignmentButton->setChecked(true);
        else
            ui->leftAlignmentButton->setEnabled(false);
    } else {
        ui->leftAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("verticalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("verticalAlignment")).toString();
        ui->topAlignmentButton->setChecked(true);
        ui->bottomAlignmentButton->setEnabled(true);
        if (alignment == QLatin1String("Text.AlignVCenter") || alignment == QLatin1String("AlignVCenter"))
            ui->centerVAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignBottom") || alignment == QLatin1String("AlignBottom"))
            ui->bottomAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.Top") || alignment == QLatin1String("AlignTop"))
            ui->topAlignmentButton->setChecked(true);
        else
            ui->bottomAlignmentButton->setEnabled(false);
    } else {
        ui->topAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("style"))) {
        QString style = propertyReader->readProperty(QLatin1String("style")).toString();
        ui->styleComboBox->setCurrentIndex(0);
        ui->styleComboBox->setEnabled(true);
        if (style == QLatin1String("Text.Outline") || style == QLatin1String("Outline"))
            ui->styleComboBox->setCurrentIndex(1);
        else if (style == QLatin1String("Text.Raised") || style == QLatin1String("Raised"))
            ui->styleComboBox->setCurrentIndex(2);
        else if (style == QLatin1String("Text.Sunken") || style == QLatin1String("Sunken"))
            ui->styleComboBox->setCurrentIndex(3);
        else if (style == QLatin1String("Text.Normal") || style == QLatin1String("Normal"))
            ui->styleComboBox->setCurrentIndex(0);
        else
            ui->styleComboBox->setEnabled(false);
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
    parentContextWidget->colorDialog()->setupColor(ui->textColorButton->color().toString());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorButtonToggled(bool flag)
{
    if (flag)
        ui->textColorButton->setChecked(false);
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(ui->colorButton->pos());
    parentContextWidget->colorDialog()->setupColor(ui->colorButton->color().toString());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (ui->textColorButton->isChecked())
        emit  propertyChanged(QLatin1String("styleColor"),parentContextWidget->colorDialog()->color()); //write back color
    if (ui->colorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color()); //write back color
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
    if (m_fontSizeTimer > 0)
        killTimer(m_fontSizeTimer);
    m_fontSizeTimer = startTimer(200);
}

void ContextPaneTextWidget::onFontFormatChanged()
{
    int size = ui->fontSizeSpinBox->value();
    if (ui->fontSizeSpinBox->isPointSize())
        emit removeAndChangeProperty(QLatin1String("font.pixelSize"), QLatin1String("font.pointSize"), size, true);
    else
        emit removeAndChangeProperty(QLatin1String("font.pointSize"), QLatin1String("font.pixelSize"), size, true);

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
    emit propertyChanged(QLatin1String("font.family"), QVariant(QLatin1Char('"') + font.family() + QLatin1Char('"')));
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
        if (alignment == QLatin1String("Text.AlignTop"))
            emit removeProperty(QLatin1String("verticalAlignment"));
        else
            emit propertyChanged(QLatin1String("verticalAlignment"), alignment);
    }
}

ContextPaneTextWidget::~ContextPaneTextWidget()
{
    delete ui;
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
