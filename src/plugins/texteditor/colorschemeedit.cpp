/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "colorschemeedit.h"
#include "ui_colorschemeedit.h"

#include "texteditorconstants.h"

#include <QtCore/QAbstractListModel>
#include <QtGui/QColorDialog>

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
        m_descriptions = descriptions;
        reset();
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
            return description.trName();
        case Qt::ForegroundRole: {
            QColor foreground = m_scheme->formatFor(description.name()).foreground();
            if (foreground.isValid())
                return foreground;
            else
                return m_scheme->formatFor(QLatin1String(TextEditor::Constants::C_TEXT)).foreground();
        }
        case Qt::BackgroundRole: {
            QColor background = m_scheme->formatFor(description.name()).background();
            if (background.isValid())
                return background;
        }
        case Qt::FontRole: {
            QFont font = m_baseFont;
            font.setBold(m_scheme->formatFor(description.name()).bold());
            font.setItalic(m_scheme->formatFor(description.name()).italic());
            return font;
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
    m_ui->boldCheckBox->setEnabled(enabled);
    m_ui->italicCheckBox->setEnabled(enabled);
}

void ColorSchemeEdit::setColorScheme(const ColorScheme &colorScheme)
{
    m_scheme = colorScheme;
    m_formatsModel->setColorScheme(&m_scheme);
    setItemListBackground(m_scheme.formatFor(QLatin1String(TextEditor::Constants::C_TEXT)).background());
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
    const Format &format = m_scheme.formatFor(m_descriptions[m_curItem].name());
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));

    m_ui->eraseBackgroundToolButton->setEnabled(!m_readOnly
                                                && m_curItem > 0
                                                && format.background().isValid());

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
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].name()).foreground();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    QPalette p = m_ui->foregroundToolButton->palette();
    p.setColor(QPalette::Active, QPalette::Button, newColor);
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const QString category = m_descriptions[index.row()].name();
        m_scheme.formatFor(category).setForeground(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::changeBackColor()
{
    if (m_curItem == -1)
        return;
    QColor color = m_scheme.formatFor(m_descriptions[m_curItem].name()).background();
    const QColor newColor = QColorDialog::getColor(color, m_ui->boldCheckBox->window());
    if (!newColor.isValid())
        return;
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));
    m_ui->eraseBackgroundToolButton->setEnabled(true);

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const QString category = m_descriptions[index.row()].name();
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
        const QString category = m_descriptions[index.row()].name();
        m_scheme.formatFor(category).setBackground(newColor);
        m_formatsModel->emitDataChanged(index);
    }
}

void ColorSchemeEdit::checkCheckBoxes()
{
    if (m_curItem == -1)
        return;

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const QString category = m_descriptions[index.row()].name();
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
