/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
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
            font.setBold(m_scheme->formatFor(description.id()).bold());
            font.setItalic(m_scheme->formatFor(description.id()).italic());
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

    connect(m_ui->itemList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(currentItemChanged(QModelIndex)));
    connect(m_ui->foregroundToolButton, SIGNAL(clicked()), SLOT(changeForeColor()));
    connect(m_ui->backgroundToolButton, SIGNAL(clicked()), SLOT(changeBackColor()));
    connect(m_ui->eraseBackgroundToolButton, SIGNAL(clicked()), SLOT(eraseBackColor()));
    connect(m_ui->eraseForegroundToolButton, SIGNAL(clicked()), SLOT(eraseForeColor()));
    connect(m_ui->boldCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
    connect(m_ui->italicCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
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

    const bool boldBlocked = m_ui->boldCheckBox->blockSignals(true);
    m_ui->boldCheckBox->setChecked(format.bold());
    m_ui->boldCheckBox->blockSignals(boldBlocked);
    const bool italicBlocked = m_ui->italicCheckBox->blockSignals(true);
    m_ui->italicCheckBox->setChecked(format.italic());
    m_ui->italicCheckBox->blockSignals(italicBlocked);
}

void ColorSchemeEdit::changeForeColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].id()).foreground();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    QPalette p = m_ui->foregroundToolButton->palette();
    p.setColor(QPalette::Active, QPalette::Button, newColor);
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

void ColorSchemeEdit::setItemListBackground(const QColor &color)
{
    QPalette pal = m_ui->itemList->palette();
    pal.setColor(QPalette::Base, color);
    m_ui->itemList->setPalette(pal);
}
