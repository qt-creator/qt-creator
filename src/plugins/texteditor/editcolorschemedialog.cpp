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
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#include "editcolorschemedialog.h"
#include "ui_editcolorschemedialog.h"

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
    FormatsModel(const FormatDescriptions &fd,
                 const FontSettings &fontSettings,
                 const ColorScheme &scheme,
                 QObject *parent = 0):
        QAbstractListModel(parent),
        m_fd(fd),
        m_scheme(scheme),
        m_baseFont(fontSettings.family(), fontSettings.fontSize())
    {
    }

    int rowCount(const QModelIndex &parent) const
    { return parent.isValid() ? 0 : m_fd.size(); }

    QVariant data(const QModelIndex &index, int role) const
    {
        const FormatDescription &description = m_fd.at(index.row());
        switch (role) {
        case Qt::DisplayRole:
            return description.trName();
        case Qt::ForegroundRole: {
            QColor foreground = m_scheme.formatFor(description.name()).foreground();
            if (foreground.isValid())
                return foreground;
            else
                return m_scheme.formatFor(QLatin1String(TextEditor::Constants::C_TEXT)).foreground();
        }
        case Qt::BackgroundRole: {
            QColor background = m_scheme.formatFor(description.name()).background();
            if (background.isValid())
                return background;
            else
                return m_scheme.formatFor(QLatin1String(TextEditor::Constants::C_TEXT)).background();
        }
        case Qt::FontRole: {
            QFont font = m_baseFont;
            font.setBold(m_scheme.formatFor(description.name()).bold());
            font.setItalic(m_scheme.formatFor(description.name()).italic());
            return font;
        }
        }
        return QVariant();
    }

    void emitDataChanged(const QModelIndex &i)
    {
        // If the text category changes, all indexes might have changed
        if (i.row() == 0)
            emit dataChanged(i, index(m_fd.size() - 1));
        else
            emit dataChanged(i, i);
    }

private:
    const FormatDescriptions &m_fd;
    const ColorScheme &m_scheme;
    const QFont m_baseFont;
};

} // namespace Internal
} // namespace TextEditor

EditColorSchemeDialog::EditColorSchemeDialog(const FormatDescriptions &fd,
                                             const FontSettings &fontSettings,
                                             QWidget *parent) :
    QDialog(parent),
    m_descriptions(fd),
    m_fontSettings(fontSettings),
    m_scheme(fontSettings.colorScheme()),
    m_curItem(-1),
    m_ui(new Ui::EditColorSchemeDialog),
    m_formatsModel(new FormatsModel(m_descriptions, m_fontSettings, m_scheme, this))
{
    m_ui->setupUi(this);
    m_ui->nameEdit->setText(m_scheme.name());
    m_ui->itemList->setModel(m_formatsModel);

    connect(m_ui->itemList->selectionModel(), SIGNAL(currentRowChanged(QModelIndex,QModelIndex)),
            SLOT(itemChanged(QModelIndex)));
    connect(m_ui->foregroundToolButton, SIGNAL(clicked()), SLOT(changeForeColor()));
    connect(m_ui->backgroundToolButton, SIGNAL(clicked()), SLOT(changeBackColor()));
    connect(m_ui->eraseBackgroundToolButton, SIGNAL(clicked()), SLOT(eraseBackColor()));
    connect(m_ui->boldCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
    connect(m_ui->italicCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));

    if (!m_descriptions.empty())
        m_ui->itemList->setCurrentIndex(m_formatsModel->index(0));
}

EditColorSchemeDialog::~EditColorSchemeDialog()
{
    delete m_ui;
}

void EditColorSchemeDialog::accept()
{
    m_scheme.setName(m_ui->nameEdit->text());
    QDialog::accept();
}

void EditColorSchemeDialog::itemChanged(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    m_curItem = index.row();

    const Format &format = m_scheme.formatFor(m_descriptions[m_curItem].name());
    m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));

    m_ui->eraseBackgroundToolButton->setEnabled(m_curItem > 0 && format.background().isValid());

    const bool boldBlocked = m_ui->boldCheckBox->blockSignals(true);
    m_ui->boldCheckBox->setChecked(format.bold());
    m_ui->boldCheckBox->blockSignals(boldBlocked);
    const bool italicBlocked = m_ui->italicCheckBox->blockSignals(true);
    m_ui->italicCheckBox->setChecked(format.italic());
    m_ui->italicCheckBox->blockSignals(italicBlocked);
    updatePreview();
}

void EditColorSchemeDialog::changeForeColor()
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

    updatePreview();
}

void EditColorSchemeDialog::changeBackColor()
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
    }

    updatePreview();
}

void EditColorSchemeDialog::eraseBackColor()
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

    updatePreview();
}

void EditColorSchemeDialog::checkCheckBoxes()
{
    if (m_curItem == -1)
        return;

    foreach (const QModelIndex &index, m_ui->itemList->selectionModel()->selectedRows()) {
        const QString category = m_descriptions[index.row()].name();
        m_scheme.formatFor(category).setBold(m_ui->boldCheckBox->isChecked());
        m_scheme.formatFor(category).setItalic(m_ui->italicCheckBox->isChecked());
        m_formatsModel->emitDataChanged(index);
    }

    updatePreview();
}

void EditColorSchemeDialog::updatePreview()
{
    if (m_curItem == -1)
        return;

    const Format &currentFormat = m_scheme.formatFor(m_descriptions[m_curItem].name());
    const Format &baseFormat = m_scheme.formatFor(QLatin1String("Text"));

    QPalette pal = QApplication::palette();
    if (baseFormat.foreground().isValid()) {
        pal.setColor(QPalette::Text, baseFormat.foreground());
        pal.setColor(QPalette::Foreground, baseFormat.foreground());
    }
    if (baseFormat.background().isValid())
        pal.setColor(QPalette::Base, baseFormat.background());

    m_ui->previewTextEdit->setPalette(pal);

    QTextCharFormat format;
    if (currentFormat.foreground().isValid())
        format.setForeground(QBrush(currentFormat.foreground()));
    if (currentFormat.background().isValid())
        format.setBackground(QBrush(currentFormat.background()));
    format.setFontFamily(m_fontSettings.family());
    format.setFontStyleStrategy(m_fontSettings.antialias() ? QFont::PreferAntialias : QFont::NoAntialias);
    format.setFontPointSize(m_fontSettings.fontSize());
    format.setFontItalic(currentFormat.italic());
    if (currentFormat.bold())
        format.setFontWeight(QFont::Bold);
    m_ui->previewTextEdit->setCurrentCharFormat(format);

    m_ui->previewTextEdit->setPlainText(tr("\n\tThis is only an example."));
}
