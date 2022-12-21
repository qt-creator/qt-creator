// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "colorschemeedit.h"

#include <utils/layoutbuilder.h>
#include <utils/theme/theme.h>
#include <utils/utilsicons.h>

#include <QAbstractListModel>
#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QListView>
#include <QPushButton>
#include <QSpacerItem>
#include <QToolButton>

namespace TextEditor::Internal {

const int layoutSpacing = 6;

static QString colorButtonStyleSheet(const QColor &bgColor)
{
    QString rc("border-width: 2px; border-radius: 2px; border-color: black; ");
    rc += bgColor.isValid() ? "border-style: solid; background:" + bgColor.name() + ";"
                            : QString("border-style: dotted;");
    return rc;
}

class FormatsModel : public QAbstractListModel
{
public:
    FormatsModel(QObject *parent = nullptr):
        QAbstractListModel(parent)
    {
    }

    void setFormatDescriptions(const FormatDescriptions *descriptions)
    {
        beginResetModel();
        m_descriptions = descriptions;
        endResetModel();
    }

    void setBaseFont(const QFont &font)
    {
        emit layoutAboutToBeChanged(); // So the view adjust to new item height
        m_baseFont = font;
        emit layoutChanged();
        emitDataChanged(index(0));
    }

    void setColorScheme(const ColorScheme *scheme)
    {
        m_scheme = scheme;
        emitDataChanged(index(0));
    }

    int rowCount(const QModelIndex &parent) const override
    {
        return (parent.isValid() || !m_descriptions) ? 0 : int(m_descriptions->size());
    }

    QVariant data(const QModelIndex &index, int role) const override
    {
        if (!m_descriptions || !m_scheme)
            return QVariant();

        const FormatDescription &description = m_descriptions->at(index.row());

        switch (role) {
        case Qt::DisplayRole:
            return description.displayName();
        case Qt::ForegroundRole: {
            QColor foreground = m_scheme->formatFor(description.id()).foreground();
            if (foreground.isValid())
                return foreground;
            else
                return m_scheme->formatFor(C_TEXT).foreground();
        }
        case Qt::BackgroundRole: {
            QColor background = m_scheme->formatFor(description.id()).background();
            if (background.isValid())
                return background;
            else
                break;
        }
        case Qt::FontRole: {
            QFont font = m_baseFont;
            auto format = m_scheme->formatFor(description.id());
            font.setBold(format.bold());
            font.setItalic(format.italic());
            font.setUnderline(format.underlineStyle() != QTextCharFormat::NoUnderline);
            return font;
        }
        case Qt::ToolTipRole: {
            return description.tooltipText();
        }
        }
        return QVariant();
    }

    void emitDataChanged(const QModelIndex &i)
    {
        if (!m_descriptions)
            return;

        // If the text category changes, all indexes might have changed
        if (i.row() == 0)
            emit dataChanged(i, index(int(m_descriptions->size()) - 1));
        else
            emit dataChanged(i, i);
    }

private:
    const FormatDescriptions *m_descriptions = nullptr;
    const ColorScheme *m_scheme = nullptr;
    QFont m_baseFont;
};

ColorSchemeEdit::ColorSchemeEdit(QWidget *parent) :
    QWidget(parent),
    m_formatsModel(new FormatsModel(this))
{
    setContentsMargins(0, layoutSpacing, 0, 0);
    resize(513, 416);

    auto colorButton = [] () {
        auto tb = new QToolButton;
        tb->setMinimumWidth(56);
        return tb;
    };

    auto unsetButton = [](const QString &toolTip) {
        auto tb = new QPushButton;
        tb->setToolTip(toolTip);
        tb->setText(tr("Unset"));
        return tb;
    };

    auto headlineLabel = [] (const QString &text) {
        auto l = new QLabel(text);
        l->setContentsMargins(0, layoutSpacing * 2, 0, layoutSpacing / 2);
        QFont font = l->font();
        font.setBold(true);
        l->setFont(font);
        return l;
    };

    auto spinBox = [] () {
        auto sb = new QDoubleSpinBox;
        sb->setMinimum(-1.);
        sb->setMaximum(1.);
        sb->setSingleStep(0.05);
        return sb;
    };

    m_itemList = new QListView(this);
    m_itemList->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_itemList->setSelectionMode(QAbstractItemView::ExtendedSelection);
    m_itemList->setUniformItemSizes(true);

    m_builtinSchemeLabel = new QLabel(
        tr("<p align='center'><b>Builtin color schemes need to be <a href=\"copy\">copied</a><br/>"
           " before they can be changed</b></p>"));
    m_builtinSchemeLabel->setScaledContents(false);

    m_fontProperties = new QWidget;
    //m_fontProperties->setContentsMargins(0, 0, 0, 0);
    m_fontProperties->setMinimumWidth(212);

    m_foregroundLabel = new QLabel(tr("Foreground:"));
    m_foregroundToolButton = colorButton();
    m_eraseForegroundToolButton = unsetButton(tr("Unset foreground."));
    m_backgroundLabel = new QLabel(tr("Background:"));
    m_backgroundToolButton = colorButton();
    m_eraseBackgroundToolButton = unsetButton(tr("Unset background."));

    m_relativeForegroundHeadline = headlineLabel(tr("Relative Foreground"));
    m_foregroundSaturationLabel = new QLabel(tr("Saturation:"));
    m_foregroundSaturationSpinBox = spinBox();
    m_foregroundLightnessLabel = new QLabel(tr("Lightness:"));
    m_foregroundLightnessSpinBox = spinBox();

    m_relativeBackgroundHeadline = headlineLabel(tr("Relative Background"));
    m_backgroundSaturationLabel = new QLabel(tr("Saturation:"));
    m_backgroundSaturationSpinBox = spinBox();
    m_backgroundLightnessLabel = new QLabel(tr("Lightness:"));
    m_backgroundLightnessSpinBox = spinBox();

    m_fontHeadline = headlineLabel(tr("Font"));
    m_boldCheckBox = new QCheckBox(tr("Bold"));
    m_italicCheckBox = new QCheckBox(tr("Italic"));

    m_underlineHeadline = headlineLabel(tr("Underline"));
    m_underlineLabel = new QLabel(tr("Color:"));
    m_underlineColorToolButton = colorButton();
    m_eraseUnderlineColorToolButton = unsetButton(tr("Unset background."));
    m_underlineComboBox = new QComboBox;

    m_itemList->setModel(m_formatsModel);
    m_builtinSchemeLabel->setVisible(m_readOnly);

    auto bottomSpacer = new QWidget;
    bottomSpacer->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Expanding);

    using namespace Utils::Layouting;

    Row {
        m_itemList,
        m_builtinSchemeLabel,
        m_fontProperties,
    }.attachTo(this, WithoutMargins);

    Grid {
        m_foregroundLabel, m_foregroundToolButton, m_eraseForegroundToolButton, br,
        m_backgroundLabel, m_backgroundToolButton, m_eraseBackgroundToolButton, br,

        Span {3, m_relativeForegroundHeadline}, br,
        m_foregroundSaturationLabel, Span {2, m_foregroundSaturationSpinBox}, br,
        m_foregroundLightnessLabel, Span {2, m_foregroundLightnessSpinBox}, br,

        Span {3, m_relativeBackgroundHeadline}, br,
        m_backgroundSaturationLabel, Span {2, m_backgroundSaturationSpinBox}, br,
        m_backgroundLightnessLabel, Span {2, m_backgroundLightnessSpinBox}, br,

        Span {3, m_fontHeadline}, br,
        Span {3, Row {m_boldCheckBox, m_italicCheckBox, st}}, br,

        Span {3, m_underlineHeadline}, br,
        m_underlineLabel, m_underlineColorToolButton, m_eraseUnderlineColorToolButton, br,

        Span {3, m_underlineComboBox}, br,

        bottomSpacer, br,
    }.attachTo(m_fontProperties);

    populateUnderlineStyleComboBox();

    connect(m_itemList->selectionModel(), &QItemSelectionModel::currentRowChanged,
            this, &ColorSchemeEdit::currentItemChanged);
    connect(m_foregroundToolButton, &QAbstractButton::clicked,
            this, &ColorSchemeEdit::changeForeColor);
    connect(m_backgroundToolButton, &QAbstractButton::clicked,
            this, &ColorSchemeEdit::changeBackColor);
    connect(m_eraseBackgroundToolButton, &QAbstractButton::clicked,
            this, &ColorSchemeEdit::eraseBackColor);
    connect(m_eraseForegroundToolButton, &QAbstractButton::clicked,
            this, &ColorSchemeEdit::eraseForeColor);
    connect(m_foregroundSaturationSpinBox, &QDoubleSpinBox::valueChanged,
            this, &ColorSchemeEdit::changeRelativeForeColor);
    connect(m_foregroundLightnessSpinBox, &QDoubleSpinBox::valueChanged,
            this, &ColorSchemeEdit::changeRelativeForeColor);
    connect(m_backgroundSaturationSpinBox, &QDoubleSpinBox::valueChanged,
            this, &ColorSchemeEdit::changeRelativeBackColor);
    connect(m_backgroundLightnessSpinBox, &QDoubleSpinBox::valueChanged,
            this, &ColorSchemeEdit::changeRelativeBackColor);
    connect(m_boldCheckBox, &QAbstractButton::toggled,
            this, &ColorSchemeEdit::checkCheckBoxes);
    connect(m_italicCheckBox, &QAbstractButton::toggled,
            this, &ColorSchemeEdit::checkCheckBoxes);
    connect(m_underlineColorToolButton, &QToolButton::clicked,
            this, &ColorSchemeEdit::changeUnderlineColor);
    connect(m_eraseUnderlineColorToolButton, &QToolButton::clicked,
            this, &ColorSchemeEdit::eraseUnderlineColor);
    connect(m_underlineComboBox, &QComboBox::currentIndexChanged,
            this, &ColorSchemeEdit::changeUnderlineStyle);
    connect(m_builtinSchemeLabel, &QLabel::linkActivated, this, &ColorSchemeEdit::copyScheme);
}

ColorSchemeEdit::~ColorSchemeEdit() = default;

void ColorSchemeEdit::setFormatDescriptions(const FormatDescriptions &descriptions)
{
    m_descriptions = descriptions;
    m_formatsModel->setFormatDescriptions(&m_descriptions);

    if (!m_descriptions.empty())
        m_itemList->setCurrentIndex(m_formatsModel->index(0));
}

void ColorSchemeEdit::setBaseFont(const QFont &font)
{
    m_formatsModel->setBaseFont(font);
}

void ColorSchemeEdit::setReadOnly(bool readOnly)
{
    if (m_readOnly == readOnly)
        return;

    m_readOnly = readOnly;

    m_fontProperties->setVisible(!readOnly);
    m_builtinSchemeLabel->setVisible(readOnly);
    updateControls();
}

void ColorSchemeEdit::setColorScheme(const ColorScheme &colorScheme)
{
    m_scheme = colorScheme;
    m_formatsModel->setColorScheme(&m_scheme);
    setItemListBackground(m_scheme.formatFor(C_TEXT).background());
    updateControls();
}

const ColorScheme &ColorSchemeEdit::colorScheme() const
{
    return m_scheme;
}

void ColorSchemeEdit::currentItemChanged(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    m_curItem = index.row();
    updateControls();
}

void ColorSchemeEdit::updateControls()
{
    updateForegroundControls();
    updateBackgroundControls();
    updateRelativeForegroundControls();
    updateRelativeBackgroundControls();
    updateFontControls();
    updateUnderlineControls();
}

void ColorSchemeEdit::updateForegroundControls()
{
    const auto &formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowForegroundControl);

    m_relativeForegroundHeadline->setEnabled(isVisible);
    m_foregroundLabel->setVisible(isVisible);
    m_foregroundToolButton->setVisible(isVisible);
    m_eraseForegroundToolButton->setVisible(isVisible);

    m_foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
    m_eraseForegroundToolButton->setEnabled(!m_readOnly
                                                && m_curItem > 0
                                                && format.foreground().isValid());
}

void ColorSchemeEdit::updateBackgroundControls()
{
    const auto formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowBackgroundControl);

    m_relativeBackgroundHeadline->setVisible(isVisible);
    m_backgroundLabel->setVisible(isVisible);
    m_backgroundToolButton->setVisible(isVisible);
    m_eraseBackgroundToolButton->setVisible(isVisible);

    m_backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));
    m_eraseBackgroundToolButton->setEnabled(!m_readOnly
                                                && m_curItem > 0
                                                && format.background().isValid());
}

void ColorSchemeEdit::updateRelativeForegroundControls()
{
    const auto &formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    QSignalBlocker saturationSignalBlocker(m_foregroundSaturationSpinBox);
    QSignalBlocker lightnessSignalBlocker(m_foregroundLightnessSpinBox);

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowRelativeForegroundControl);

    m_relativeForegroundHeadline->setVisible(isVisible);
    m_foregroundSaturationLabel->setVisible(isVisible);
    m_foregroundLightnessLabel->setVisible(isVisible);
    m_foregroundSaturationSpinBox->setVisible(isVisible);
    m_foregroundLightnessSpinBox->setVisible(isVisible);

    bool isEnabled = !m_readOnly && !format.foreground().isValid();

    m_relativeForegroundHeadline->setEnabled(isEnabled);
    m_foregroundSaturationLabel->setEnabled(isEnabled);
    m_foregroundLightnessLabel->setEnabled(isEnabled);
    m_foregroundSaturationSpinBox->setEnabled(isEnabled);
    m_foregroundLightnessSpinBox->setEnabled(isEnabled);

    m_foregroundSaturationSpinBox->setValue(format.relativeForegroundSaturation());
    m_foregroundLightnessSpinBox->setValue(format.relativeForegroundLightness());
}

void ColorSchemeEdit::updateRelativeBackgroundControls()
{
    const auto &formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    QSignalBlocker saturationSignalBlocker(m_backgroundSaturationSpinBox);
    QSignalBlocker lightnessSignalBlocker(m_backgroundLightnessSpinBox);

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowRelativeBackgroundControl);

    m_relativeBackgroundHeadline->setVisible(isVisible);
    m_backgroundSaturationLabel->setVisible(isVisible);
    m_backgroundLightnessLabel->setVisible(isVisible);
    m_backgroundSaturationSpinBox->setVisible(isVisible);
    m_backgroundLightnessSpinBox->setVisible(isVisible);

    bool isEnabled = !m_readOnly && !format.background().isValid();

    m_relativeBackgroundHeadline->setEnabled(isEnabled);
    m_backgroundSaturationLabel->setEnabled(isEnabled);
    m_backgroundLightnessLabel->setEnabled(isEnabled);
    m_backgroundSaturationSpinBox->setEnabled(isEnabled);
    m_backgroundLightnessSpinBox->setEnabled(isEnabled);

    m_backgroundSaturationSpinBox->setValue(format.relativeBackgroundSaturation());
    m_backgroundLightnessSpinBox->setValue(format.relativeBackgroundLightness());
}

void ColorSchemeEdit::updateFontControls()
{
    const auto formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    QSignalBlocker boldSignalBlocker(m_boldCheckBox);
    QSignalBlocker italicSignalBlocker(m_italicCheckBox);

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowFontControls);

    m_fontHeadline->setVisible(isVisible);
    m_boldCheckBox->setVisible(isVisible);
    m_italicCheckBox->setVisible(isVisible);

    m_boldCheckBox->setChecked(format.bold());
    m_italicCheckBox->setChecked(format.italic());

}

void ColorSchemeEdit::updateUnderlineControls()
{
    const auto formatDescription = m_descriptions[m_curItem];
    const Format &format = m_scheme.formatFor(formatDescription.id());

    QSignalBlocker comboBoxSignalBlocker(m_underlineComboBox);

    bool isVisible = !m_readOnly
                     && formatDescription.showControl(FormatDescription::ShowUnderlineControl);

    m_underlineHeadline->setVisible(isVisible);
    m_underlineLabel->setVisible(isVisible);
    m_underlineColorToolButton->setVisible(isVisible);
    m_eraseUnderlineColorToolButton->setVisible(isVisible);
    m_underlineComboBox->setVisible(isVisible);

    m_underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(format.underlineColor()));
    m_eraseUnderlineColorToolButton->setEnabled(!m_readOnly
                                                    && m_curItem > 0
                                                    && format.underlineColor().isValid());
    int index = m_underlineComboBox->findData(QVariant::fromValue(int(format.underlineStyle())));
    m_underlineComboBox->setCurrentIndex(index);
}

void ColorSchemeEdit::changeForeColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).foreground();
    const QColor newColor = QColorDialog::getColor(color, m_boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseForegroundToolButton->setEnabled(true);

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setForeground(newColor);
        m_formatsModel->emitDataChanged(index);
    }

    updateControls();
}

void ColorSchemeEdit::changeBackColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).background();
    const QColor newColor = QColorDialog::getColor(color, m_boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseBackgroundToolButton->setEnabled(true);

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBackground(newColor);
        m_formatsModel->emitDataChanged(index);
        // Synchronize item list background with text background
        if (index.row() == 0)
            setItemListBackground(newColor);
    }

    updateControls();
}

void ColorSchemeEdit::eraseBackColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseBackgroundToolButton->setEnabled(false);

    const QList<QModelIndex> indexes = m_itemList->selectionModel()->selectedRows();
    for (const QModelIndex &index : indexes) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBackground(newColor);
        m_formatsModel->emitDataChanged(index);
    }

    updateControls();
}

void ColorSchemeEdit::eraseForeColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseForegroundToolButton->setEnabled(false);

    const QList<QModelIndex> indexes = m_itemList->selectionModel()->selectedRows();
    for (const QModelIndex &index : indexes) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setForeground(newColor);
        m_formatsModel->emitDataChanged(index);
    }

    updateControls();
}

void ColorSchemeEdit::changeRelativeForeColor()
{
    if (m_curItem == -1)
        return;

    double saturation = m_foregroundSaturationSpinBox->value();
    double lightness = m_foregroundLightnessSpinBox->value();

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setRelativeForegroundSaturation(saturation);
        m_scheme.formatFor(category).setRelativeForegroundLightness(lightness);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeRelativeBackColor()
{
    if (m_curItem == -1)
        return;

    double saturation = m_backgroundSaturationSpinBox->value();
    double lightness = m_backgroundLightnessSpinBox->value();

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setRelativeBackgroundSaturation(saturation);
        m_scheme.formatFor(category).setRelativeBackgroundLightness(lightness);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::eraseRelativeForeColor()
{
    if (m_curItem == -1)
        return;

    m_foregroundSaturationSpinBox->setValue(0.0);
    m_foregroundLightnessSpinBox->setValue(0.0);

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setRelativeForegroundSaturation(0.0);
        m_scheme.formatFor(category).setRelativeForegroundLightness(0.0);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::eraseRelativeBackColor()
{
    if (m_curItem == -1)
        return;

    m_backgroundSaturationSpinBox->setValue(0.0);
    m_backgroundLightnessSpinBox->setValue(0.0);

    const QList<QModelIndex> indexes = m_itemList->selectionModel()->selectedRows();
    for (const QModelIndex &index : indexes) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setRelativeBackgroundSaturation(0.0);
        m_scheme.formatFor(category).setRelativeBackgroundLightness(0.0);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::checkCheckBoxes()
{
    if (m_curItem == -1)
        return;

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBold(m_boldCheckBox->isChecked());
        m_scheme.formatFor(category).setItalic(m_italicCheckBox->isChecked());
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeUnderlineColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).underlineColor();
    const QColor newColor = QColorDialog::getColor(color, m_boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseUnderlineColorToolButton->setEnabled(true);

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setUnderlineColor(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::eraseUnderlineColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_eraseUnderlineColorToolButton->setEnabled(false);

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setUnderlineColor(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeUnderlineStyle(int comboBoxIndex)
{
    if (m_curItem == -1)
        return;

    for (const QModelIndex &index : m_itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        const QVariant value = m_underlineComboBox->itemData(comboBoxIndex);
        auto enumeratorIndex = static_cast<QTextCharFormat::UnderlineStyle>(value.toInt());
        m_scheme.formatFor(category).setUnderlineStyle(enumeratorIndex);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::setItemListBackground(const QColor &color)
{
    QPalette pal;
    pal.setColor(QPalette::Base, color);
    m_itemList->setPalette(pal);
}

void ColorSchemeEdit::populateUnderlineStyleComboBox()
{
    m_underlineComboBox->addItem(tr("No Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::NoUnderline)));
    m_underlineComboBox->addItem(tr("Single Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::SingleUnderline)));
    m_underlineComboBox->addItem(tr("Wave Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::WaveUnderline)));
    m_underlineComboBox->addItem(tr("Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DotLine)));
    m_underlineComboBox->addItem(tr("Dash Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashUnderline)));
    m_underlineComboBox->addItem(tr("Dash-Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashDotLine)));
    m_underlineComboBox->addItem(tr("Dash-Dot-Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashDotDotLine)));
}

} // TextEditor::Internal
