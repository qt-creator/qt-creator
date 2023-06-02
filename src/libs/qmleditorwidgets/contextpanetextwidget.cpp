// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "contextpanetextwidget.h"

#include "colorbutton.h"
#include "contextpanewidget.h"
#include "customcolordialog.h"
#include "fontsizespinbox.h"
#include "qmleditorwidgetstr.h"

#include <qmljs/qmljspropertyreader.h>
#include <utils/layoutbuilder.h>

#include <QButtonGroup>
#include <QComboBox>
#include <QFontComboBox>
#include <QLabel>
#include <QTimerEvent>
#include <QToolButton>
#include <QVariant>

namespace QmlEditorWidgets {

ContextPaneTextWidget::ContextPaneTextWidget(QWidget *parent) :
    QWidget(parent)
{
    setWindowTitle(Tr::tr("Text"));

    auto iconToolButton = [] (const QString &iconName, const QString &iconFallBack,
            bool autoExclusive = true) {
        auto result = new QToolButton;
        result->setAutoExclusive(autoExclusive);
        result->setCheckable(true);
        result->setFixedSize(30, 30);
        result->setIcon(QIcon::fromTheme(iconName,
                                         QIcon(":/qmldesigner/images/" + iconFallBack + ".png")));
        result->setIconSize({24, 24});
        return result;
    };

    auto colorButton = [] () {
        auto result = new ColorButton;
        result->setCheckable(true);
        result->setFixedSize(22, 22);
        result->setShowArrow(false);
        return result;
    };

    m_fontComboBox = new QFontComboBox;
    m_colorButton = colorButton();
    m_fontSizeSpinBox = new FontSizeSpinBox;
    m_fontSizeSpinBox->setMinimumWidth(60);

    m_boldButton = iconToolButton("format-text-bold", "bold-h-icon", false);
    m_italicButton = iconToolButton("format-text-italic", "italic-h-icon", false);
    m_underlineButton = iconToolButton("format-text-underline", "underline-h-icon", false);
    m_strikeoutButton = iconToolButton("format-text-strikethrough", "strikeout-h-icon", false);

    m_leftAlignmentButton = iconToolButton("format-justify-left", "alignmentleft-h-icon");
    m_centerHAlignmentButton = iconToolButton("format-justify-center", "alignmentcenterh-h-icon");
    m_rightAlignmentButton = iconToolButton("format-justify-right", "alignmentright-h-icon");

    m_topAlignmentButton = iconToolButton({}, "alignmenttop-h-icon");
    m_centerVAlignmentButton = iconToolButton({}, "alignmentmiddle-h-icon");
    m_bottomAlignmentButton = iconToolButton({}, "alignmentbottom-h-icon");

    m_styleLabel = new QLabel(Tr::tr("Style"));
    m_styleComboBox = new QComboBox;
    m_styleComboBox->addItems({"Normal", "Outline", "Raised", "Sunken"});
    m_textColorButton = colorButton();

    auto hAlignButtons = new QButtonGroup(this);
    hAlignButtons->addButton(m_leftAlignmentButton);
    hAlignButtons->addButton(m_centerHAlignmentButton);
    hAlignButtons->addButton(m_rightAlignmentButton);

    auto vAlignButtons = new QButtonGroup(this);
    vAlignButtons->addButton(m_topAlignmentButton);
    vAlignButtons->addButton(m_centerVAlignmentButton);
    vAlignButtons->addButton(m_bottomAlignmentButton);

    using namespace Layouting;
    Column {
        Row { m_fontComboBox, m_colorButton, m_fontSizeSpinBox, },
        Row {
            Column {
                Row { m_boldButton, m_italicButton, m_underlineButton, m_strikeoutButton, st, },
                Form { m_styleLabel, m_styleComboBox, m_textColorButton, },
            },
            Grid {
                m_leftAlignmentButton, m_centerHAlignmentButton, m_rightAlignmentButton, br,
                m_topAlignmentButton, m_centerVAlignmentButton, m_bottomAlignmentButton,
            },
        },
    }.attachTo(this);

    connect(m_colorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneTextWidget::onColorButtonToggled);
    connect(m_textColorButton, &QmlEditorWidgets::ColorButton::toggled,
            this, &ContextPaneTextWidget::onTextColorButtonToggled);

    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::accepted,
            this, &ContextPaneTextWidget::onColorDialogApplied);
    connect(parentContextWidget->colorDialog(), &CustomColorDialog::rejected,
            this, &ContextPaneTextWidget::onColorDialogCancled);

    connect(m_fontSizeSpinBox, &QmlEditorWidgets::FontSizeSpinBox::valueChanged,
            this, &ContextPaneTextWidget::onFontSizeChanged);
    connect(m_fontSizeSpinBox, &QmlEditorWidgets::FontSizeSpinBox::formatChanged,
            this, &ContextPaneTextWidget::onFontFormatChanged);

    connect(m_boldButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onBoldCheckedChanged);
    connect(m_italicButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onItalicCheckedChanged);
    connect(m_underlineButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onUnderlineCheckedChanged);
    connect(m_strikeoutButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onStrikeoutCheckedChanged);
    connect(m_fontComboBox, &QFontComboBox::currentFontChanged,
            this, &ContextPaneTextWidget::onCurrentFontChanged);

    connect(m_centerHAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);
    connect(m_leftAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);
    connect(m_rightAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onHorizontalAlignmentChanged);

    connect(m_centerVAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);
    connect(m_topAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);
    connect(m_bottomAlignmentButton, &QToolButton::toggled,
            this, &ContextPaneTextWidget::onVerticalAlignmentChanged);

    connect(m_styleComboBox, &QComboBox::currentIndexChanged,
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
        m_fontSizeSpinBox->setValue(variant.toInt(&b));
        m_fontSizeSpinBox->setEnabled(b);
        m_fontSizeSpinBox->setIsPointSize(true);
    } else if (!propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        m_fontSizeSpinBox->setValue(8);
        m_fontSizeSpinBox->setIsPointSize(true);
    }

    if (m_fontSizeTimer > 0) {
        killTimer(m_fontSizeTimer);
        m_fontSizeTimer = -1;
    }

    if (propertyReader->hasProperty(QLatin1String("font.pixelSize"))) {
        QVariant variant = propertyReader->readProperty(QLatin1String("font.pixelSize"));
        bool b;
        m_fontSizeSpinBox->setValue(variant.toInt(&b));

        m_fontSizeSpinBox->setEnabled(b);
        m_fontSizeSpinBox->setIsPixelSize(true);
    }

    m_boldButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.bold"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.bold"));
        if (checkIfBoolean(v))
            m_boldButton->setChecked(v.toBool());
        else
            m_boldButton->setEnabled(false);
    } else {
        m_boldButton->setChecked(false);
    }

    m_italicButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.italic"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.italic"));
        if (checkIfBoolean(v))
            m_italicButton->setChecked(v.toBool());
        else
            m_italicButton->setEnabled(false);
    } else {
        m_italicButton->setChecked(false);
    }

    m_underlineButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.underline"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.underline"));
        if (checkIfBoolean(v))
            m_underlineButton->setChecked(v.toBool());
        else
            m_underlineButton->setEnabled(false);
    } else {
        m_underlineButton->setChecked(false);
    }

    m_strikeoutButton->setEnabled(true);
    if (propertyReader->hasProperty(QLatin1String("font.strikeout"))) {
        QVariant v = propertyReader->readProperty(QLatin1String("font.strikeout"));
        if (checkIfBoolean(v))
            m_strikeoutButton->setChecked(v.toBool());
        else
            m_strikeoutButton->setEnabled(false);
    } else {
        m_strikeoutButton->setChecked(false);
    }

    if (propertyReader->hasProperty(QLatin1String("color")))
        m_colorButton->setColor(propertyReader->readProperty(QLatin1String("color")).toString());
    else
        m_colorButton->setColor(QLatin1String("black"));

    if (propertyReader->hasProperty(QLatin1String("styleColor")))
        m_textColorButton->setColor(propertyReader->readProperty(QLatin1String("styleColor")).toString());
    else
        m_textColorButton->setColor(QLatin1String("black"));

    if (propertyReader->hasProperty(QLatin1String("font.family"))) {
        QString familyName = propertyReader->readProperty(QLatin1String("font.family")).toString();
        QFont font;
        font.setFamily(familyName);

        m_fontComboBox->setCurrentFont(font);
        if (propertyReader->isBindingOrEnum(QLatin1String("font.family")))
            m_fontComboBox->setEnabled(false);
        else
            m_fontComboBox->setEnabled(true);
    }

    if (propertyReader->hasProperty(QLatin1String("horizontalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("horizontalAlignment")).toString();
        m_leftAlignmentButton->setChecked(true);
        m_leftAlignmentButton->setEnabled(true);
        if (alignment == QLatin1String("Text.AlignHCenter") || alignment == QLatin1String("AlignHCenter"))
            m_centerHAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignRight") || alignment == QLatin1String("AlignRight"))
            m_rightAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignLeft") || alignment == QLatin1String("AlignLeft"))
            m_leftAlignmentButton->setChecked(true);
        else
            m_leftAlignmentButton->setEnabled(false);
    } else {
        m_leftAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("verticalAlignment"))) {
        QString alignment = propertyReader->readProperty(QLatin1String("verticalAlignment")).toString();
        m_topAlignmentButton->setChecked(true);
        m_bottomAlignmentButton->setEnabled(true);
        if (alignment == QLatin1String("Text.AlignVCenter") || alignment == QLatin1String("AlignVCenter"))
            m_centerVAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.AlignBottom") || alignment == QLatin1String("AlignBottom"))
            m_bottomAlignmentButton->setChecked(true);
        else if (alignment == QLatin1String("Text.Top") || alignment == QLatin1String("AlignTop"))
            m_topAlignmentButton->setChecked(true);
        else
            m_bottomAlignmentButton->setEnabled(false);
    } else {
        m_topAlignmentButton->setChecked(true);
    }

    if (propertyReader->hasProperty(QLatin1String("style"))) {
        QString style = propertyReader->readProperty(QLatin1String("style")).toString();
        m_styleComboBox->setCurrentIndex(0);
        m_styleComboBox->setEnabled(true);
        if (style == QLatin1String("Text.Outline") || style == QLatin1String("Outline"))
            m_styleComboBox->setCurrentIndex(1);
        else if (style == QLatin1String("Text.Raised") || style == QLatin1String("Raised"))
            m_styleComboBox->setCurrentIndex(2);
        else if (style == QLatin1String("Text.Sunken") || style == QLatin1String("Sunken"))
            m_styleComboBox->setCurrentIndex(3);
        else if (style == QLatin1String("Text.Normal") || style == QLatin1String("Normal"))
            m_styleComboBox->setCurrentIndex(0);
        else
            m_styleComboBox->setEnabled(false);
    } else {
        m_styleComboBox->setCurrentIndex(0);
    }
}

void ContextPaneTextWidget::setVerticalAlignmentVisible(bool b)
{
    m_centerVAlignmentButton->setEnabled(b);
    m_topAlignmentButton->setEnabled(b);
    m_bottomAlignmentButton->setEnabled(b);
}

void ContextPaneTextWidget::setStyleVisible(bool b)
{
    m_styleComboBox->setEnabled(b);
    m_styleLabel->setEnabled(b);
    m_textColorButton->setEnabled(b);
}

void ContextPaneTextWidget::onTextColorButtonToggled(bool flag)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    if (flag)
        m_colorButton->setChecked(false);
    QPoint p = mapToGlobal(m_textColorButton->pos());
    parentContextWidget->colorDialog()->setupColor(m_textColorButton->color().toString());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorButtonToggled(bool flag)
{
    if (flag)
        m_textColorButton->setChecked(false);
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    QPoint p = mapToGlobal(m_colorButton->pos());
    parentContextWidget->colorDialog()->setupColor(m_colorButton->color().toString());
    p = parentContextWidget->colorDialog()->parentWidget()->mapFromGlobal(p);
    parentContextWidget->onShowColorDialog(flag, p);
}

void ContextPaneTextWidget::onColorDialogApplied(const QColor &)
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    if (m_textColorButton->isChecked())
        emit  propertyChanged(QLatin1String("styleColor"),parentContextWidget->colorDialog()->color()); //write back color
    if (m_colorButton->isChecked())
        emit  propertyChanged(QLatin1String("color"),parentContextWidget->colorDialog()->color()); //write back color
    m_textColorButton->setChecked(false);
    m_colorButton->setChecked(false);
}

void ContextPaneTextWidget::onColorDialogCancled()
{
    ContextPaneWidget *parentContextWidget = qobject_cast<ContextPaneWidget*>(parentWidget());
    parentContextWidget->onShowColorDialog(false, QPoint());
    m_colorButton->setChecked(false);
    m_colorButton->setChecked(false);
}

void ContextPaneTextWidget::onFontSizeChanged(int)
{
    if (m_fontSizeTimer > 0)
        killTimer(m_fontSizeTimer);
    m_fontSizeTimer = startTimer(200);
}

void ContextPaneTextWidget::onFontFormatChanged()
{
    int size = m_fontSizeSpinBox->value();
    if (m_fontSizeSpinBox->isPointSize())
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
    if (m_centerHAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignHCenter");
    else if (m_leftAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignLeft");
    else if (m_rightAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignRight");
    if (m_horizontalAlignment != alignment) {
        m_horizontalAlignment = alignment;
        if (alignment == QLatin1String("Text.AlignLeft"))
            emit removeProperty(QLatin1String("horizontalAlignment"));
        else
            emit propertyChanged(QLatin1String("horizontalAlignment"), alignment);
    }
}

void ContextPaneTextWidget::onStyleComboBoxChanged(int index)
{
    const QString style = m_styleComboBox->itemText(index);
    if (style == QLatin1String("Normal"))
        emit removeProperty(QLatin1String("style"));
    else
        emit propertyChanged(QLatin1String("style"), QVariant(QLatin1String("Text.") + style));
}

void ContextPaneTextWidget::onVerticalAlignmentChanged()
{
    QString alignment;
    if (m_centerVAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignVCenter");
    else if (m_topAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignTop");
    else if (m_bottomAlignmentButton->isChecked())
        alignment = QLatin1String("Text.AlignBottom");
    if (m_verticalAlignment != alignment) {
        m_verticalAlignment = alignment;
        if (alignment == QLatin1String("Text.AlignTop"))
            emit removeProperty(QLatin1String("verticalAlignment"));
        else
            emit propertyChanged(QLatin1String("verticalAlignment"), alignment);
    }
}

void ContextPaneTextWidget::timerEvent(QTimerEvent *event)
{
    if (event->timerId() == m_fontSizeTimer) {
        killTimer(m_fontSizeTimer);
        m_fontSizeTimer = -1;
        int value = m_fontSizeSpinBox->value();
        if (m_fontSizeSpinBox->isPointSize())
            emit propertyChanged(QLatin1String("font.pointSize"), value);
        else
            emit propertyChanged(QLatin1String("font.pixelSize"), value);
    }
}

} //QmlDesigner
