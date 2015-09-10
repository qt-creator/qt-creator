/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "colorschemeedit.h"
#include "ui_colorschemeedit.h"

#include <QAbstractListModel>
#include <QColorDialog>

using namespace TextEditor;
using namespace TextEditor::Internal;

static inline QString colorButtonStyleSheet(const QColor &bgColor)
{
    if (bgColor.isValid()) {
        QString rc = QLatin1String("border: 2px solid black; border-radius: 2px; background:");
        rc += bgColor.name();
        return rc;
    }
    return QLatin1String("border: 2px dotted black; border-radius: 2px;");
}

namespace TextEditor {
namespace Internal {

class FormatsModel : public QAbstractListModel
{
public:
    FormatsModel(QObject *parent = 0):
        QAbstractListModel(parent),
        m_descriptions(0),
        m_scheme(0)
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

    int rowCount(const QModelIndex &parent) const
    {
        return (parent.isValid() || !m_descriptions) ? 0 : m_descriptions->size();
    }

    QVariant data(const QModelIndex &index, int role) const
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
            emit dataChanged(i, index(m_descriptions->size() - 1));
        else
            emit dataChanged(i, i);
    }

private:
    const FormatDescriptions *m_descriptions;
    const ColorScheme *m_scheme;
    QFont m_baseFont;
};

} // namespace Internal
} // namespace TextEditor

ColorSchemeEdit::ColorSchemeEdit(QWidget *parent) :
    QWidget(parent),
    m_curItem(-1),
    m_ui(new Ui::ColorSchemeEdit),
    m_formatsModel(new FormatsModel(this)),
    m_readOnly(false)
{
    m_ui->setupUi(this);
    m_ui->itemList->setModel(m_formatsModel);

    populateUnderlineStyleComboBox();

    connect(m_ui->itemList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(currentItemChanged(QModelIndex)));
    connect(m_ui->foregroundToolButton, SIGNAL(clicked()), SLOT(changeForeColor()));
    connect(m_ui->backgroundToolButton, SIGNAL(clicked()), SLOT(changeBackColor()));
    connect(m_ui->eraseBackgroundToolButton, SIGNAL(clicked()), SLOT(eraseBackColor()));
    connect(m_ui->eraseForegroundToolButton, SIGNAL(clicked()), SLOT(eraseForeColor()));
    connect(m_ui->boldCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
    connect(m_ui->italicCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
    connect(m_ui->underlineColorToolButton,
            &QToolButton::clicked,
            this,
            &ColorSchemeEdit::changeUnderlineColor);
    connect(m_ui->eraseUnderlineColorToolButton,
            &QToolButton::clicked,
            this,
            &ColorSchemeEdit::eraseUnderlineColor);
    connect(m_ui->underlineComboBox,
            static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged),
            this,
            &ColorSchemeEdit::changeUnderlineStyle);
}

ColorSchemeEdit::~ColorSchemeEdit()
{
    delete m_ui;
}

void ColorSchemeEdit::setFormatDescriptions(const FormatDescriptions &descriptions)
{
    m_descriptions = descriptions;
    m_formatsModel->setFormatDescriptions(&m_descriptions);

    if (!m_descriptions.empty())
        m_ui->itemList->setCurrentIndex(m_formatsModel->index(0));
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

    const bool enabled = !readOnly;
    m_ui->foregroundLabel->setEnabled(enabled);
    m_ui->foregroundToolButton->setEnabled(enabled);
    m_ui->backgroundLabel->setEnabled(enabled);
    m_ui->backgroundToolButton->setEnabled(enabled);
    m_ui->eraseBackgroundToolButton->setEnabled(enabled);
    m_ui->eraseForegroundToolButton->setEnabled(enabled);
    m_ui->boldCheckBox->setEnabled(enabled);
    m_ui->italicCheckBox->setEnabled(enabled);
    m_ui->underlineColorToolButton->setEnabled(enabled);
    m_ui->eraseUnderlineColorToolButton->setEnabled(enabled);
    m_ui->underlineComboBox->setEnabled(enabled);
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
    const Format &format = m_scheme.formatFor(m_descriptions[m_curItem].id());
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));

    m_ui->eraseBackgroundToolButton->setEnabled(!m_readOnly
                                                && m_curItem > 0
                                                && format.background().isValid());
    m_ui->eraseForegroundToolButton->setEnabled(!m_readOnly
                                                && m_curItem > 0
                                                && format.foreground().isValid());

    QSignalBlocker boldSignalBlocker(m_ui->boldCheckBox);
    m_ui->boldCheckBox->setChecked(format.bold());
    QSignalBlocker italicSignalBlocker(m_ui->italicCheckBox);
    m_ui->italicCheckBox->setChecked(format.italic());

    m_ui->underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(format.underlineColor()));
    m_ui->eraseUnderlineColorToolButton->setEnabled(!m_readOnly
                                                    && m_curItem > 0
                                                    && format.underlineColor().isValid());
    int index = m_ui->underlineComboBox->findData(QVariant::fromValue(int(format.underlineStyle())));
    QSignalBlocker comboBoxSignalBlocker(m_ui->underlineComboBox);
    m_ui->underlineComboBox->setCurrentIndex(index);
}

void ColorSchemeEdit::changeForeColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).foreground();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseForegroundToolButton->setEnabled(true);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setForeground(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeBackColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).background();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseBackgroundToolButton->setEnabled(true);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBackground(newColor);
        m_formatsModel->emitDataChanged(index);
        // Synchronize item list background with text background
        if (index.row() == 0)
            setItemListBackground(newColor);
    }
}

void ColorSchemeEdit::eraseBackColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseBackgroundToolButton->setEnabled(false);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBackground(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::eraseForeColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseForegroundToolButton->setEnabled(false);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setForeground(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::checkCheckBoxes()
{
    if (m_curItem == -1)
        return;

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setBold(m_ui->boldCheckBox->isChecked());
        m_scheme.formatFor(category).setItalic(m_ui->italicCheckBox->isChecked());
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeUnderlineColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).underlineColor();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_ui->underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseUnderlineColorToolButton->setEnabled(true);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
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
    m_ui->underlineColorToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseUnderlineColorToolButton->setEnabled(false);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        m_scheme.formatFor(category).setUnderlineColor(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeUnderlineStyle(int comboBoxIndex)
{
    if (m_curItem == -1)
        return;

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const TextStyle category = m_descriptions[index.row()].id();
        auto value = m_ui->underlineComboBox->itemData(comboBoxIndex);
        auto enumeratorIndex = static_cast<QTextCharFormat::UnderlineStyle>(value.toInt());
        m_scheme.formatFor(category).setUnderlineStyle(enumeratorIndex);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::setItemListBackground(const QColor &color)
{
    QPalette pal;
    pal.setColor(QPalette::Base, color);
    m_ui->itemList->setPalette(pal);
}

void ColorSchemeEdit::populateUnderlineStyleComboBox()
{
    m_ui->underlineComboBox->addItem(tr("No Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::NoUnderline)));
    m_ui->underlineComboBox->addItem(tr("Single Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::SingleUnderline)));
    m_ui->underlineComboBox->addItem(tr("Wave Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::WaveUnderline)));
    m_ui->underlineComboBox->addItem(tr("Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DotLine)));
    m_ui->underlineComboBox->addItem(tr("Dash Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashUnderline)));
    m_ui->underlineComboBox->addItem(tr("Dash-Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashDotLine)));
    m_ui->underlineComboBox->addItem(tr("Dash-Dot-Dot Underline"),
                                     QVariant::fromValue(int(QTextCharFormat::DashDotDotLine)));
}
