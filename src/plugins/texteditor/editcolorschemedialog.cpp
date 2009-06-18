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

#include <QtGui/QColorDialog>

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

EditColorSchemeDialog::EditColorSchemeDialog(const FormatDescriptions &fd,
                                             const FontSettings &fontSettings,
                                             const ColorScheme &scheme,
                                             QWidget *parent) :
    QDialog(parent),
    m_descriptions(fd),
    m_fontSettings(fontSettings),
    m_scheme(scheme),
    m_curItem(-1),
    m_ui(new Ui::EditColorSchemeDialog)
{
    m_ui->setupUi(this);

    foreach (const FormatDescription &d, fd)
        m_ui->itemListWidget->addItem(d.trName());

    connect(m_ui->itemListWidget, SIGNAL(itemSelectionChanged()), SLOT(itemChanged()));
    connect(m_ui->foregroundToolButton, SIGNAL(clicked()), SLOT(changeForeColor()));
    connect(m_ui->backgroundToolButton, SIGNAL(clicked()), SLOT(changeBackColor()));
    connect(m_ui->eraseBackgroundToolButton, SIGNAL(clicked()), SLOT(eraseBackColor()));
    connect(m_ui->boldCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));
    connect(m_ui->italicCheckBox, SIGNAL(toggled(bool)), SLOT(checkCheckBoxes()));

    if (!m_descriptions.empty())
        m_ui->itemListWidget->setCurrentRow(0);
}

EditColorSchemeDialog::~EditColorSchemeDialog()
{
    delete m_ui;
}

void EditColorSchemeDialog::itemChanged()
{
    QListWidgetItem *item = m_ui->itemListWidget->currentItem();
    if (!item)
        return;

    const int numFormats = m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        if (m_descriptions[i].trName() == item->text()) {
            m_curItem = i;
            const Format &format = m_scheme.formatFor(m_descriptions[i].name());
            m_ui->foregroundToolButton->setStyleSheet(colorButtonStyleSheet(format.foreground()));
            m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(format.background()));

            m_ui->eraseBackgroundToolButton->setEnabled(i > 0 && format.background().isValid());

            const bool boldBlocked = m_ui->boldCheckBox->blockSignals(true);
            m_ui->boldCheckBox->setChecked(format.bold());
            m_ui->boldCheckBox->blockSignals(boldBlocked);
            const bool italicBlocked = m_ui->italicCheckBox->blockSignals(true);
            m_ui->italicCheckBox->setChecked(format.italic());
            m_ui->italicCheckBox->blockSignals(italicBlocked);
            updatePreview();
            break;
        }
    }
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

    const int numFormats = m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = m_ui->itemListWidget->findItems(m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            m_scheme.formatFor(m_descriptions[i].name()).setForeground(newColor);
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

    const int numFormats = m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = m_ui->itemListWidget->findItems(m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            m_scheme.formatFor(m_descriptions[i].name()).setBackground(newColor);
    }

    updatePreview();
}

void EditColorSchemeDialog::eraseBackColor()
{
    if (m_curItem == -1)
        return;
    QColor newColor;
    m_ui->backgroundToolButton->setStyleSheet(colorButtonStyleSheet(newColor));

    const int numFormats = m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = m_ui->itemListWidget->findItems(m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected())
            m_scheme.formatFor(m_descriptions[i].name()).setBackground(newColor);
    }
    m_ui->eraseBackgroundToolButton->setEnabled(false);

    updatePreview();
}

void EditColorSchemeDialog::checkCheckBoxes()
{
    if (m_curItem == -1)
        return;
    const int numFormats = m_descriptions.size();
    for (int i = 0; i < numFormats; i++) {
        QList<QListWidgetItem*> items = m_ui->itemListWidget->findItems(m_descriptions[i].trName(), Qt::MatchExactly);
        if (!items.isEmpty() && items.first()->isSelected()) {
            m_scheme.formatFor(m_descriptions[i].name()).setBold(m_ui->boldCheckBox->isChecked());
            m_scheme.formatFor(m_descriptions[i].name()).setItalic(m_ui->italicCheckBox->isChecked());
        }
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
