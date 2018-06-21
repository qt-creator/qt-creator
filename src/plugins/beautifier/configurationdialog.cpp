/****************************************************************************
**
** Copyright (C) 2016 Lorenz Haas
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

#include "configurationdialog.h"
#include "ui_configurationdialog.h"

#include "abstractsettings.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <QPushButton>
#include <QRegularExpressionValidator>

namespace Beautifier {
namespace Internal {

ConfigurationDialog::ConfigurationDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConfigurationDialog)
{
    ui->setupUi(this);

    // Filter out characters which are not allowed in a file name
    QRegularExpressionValidator *fileNameValidator = new QRegularExpressionValidator(
                QRegularExpression("^[^\\/\\\\\\?\\>\\<\\*\\%\\:\\\"\\']*$"), ui->name);
    ui->name->setValidator(fileNameValidator);

    updateDocumentation();
    connect(ui->name, &QLineEdit::textChanged, this, &ConfigurationDialog::updateOkButton);
    updateOkButton(); // force initial test.
    connect(ui->editor, &ConfigurationEditor::documentationChanged,
            this, &ConfigurationDialog::updateDocumentation);

    // Set palette and font according to settings
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::instance()->fontSettings();
    const QTextCharFormat tf = fs.toTextCharFormat(TextEditor::C_TEXT);
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(TextEditor::C_SELECTION);

    QPalette pal;
    pal.setColor(QPalette::Base, tf.background().color());
    pal.setColor(QPalette::Text, tf.foreground().color());
    pal.setColor(QPalette::Foreground, tf.foreground().color());
    if (selectionFormat.background().style() != Qt::NoBrush)
        pal.setColor(QPalette::Highlight, selectionFormat.background().color());
    pal.setBrush(QPalette::HighlightedText, selectionFormat.foreground());
    ui->documentation->setPalette(pal);
    ui->editor->setPalette(pal);

    ui->documentation->setFont(tf.font());
    ui->editor->setFont(tf.font());

    // Set style sheet for documentation browser
    const QTextCharFormat tfOption = fs.toTextCharFormat(TextEditor::C_FIELD);
    const QTextCharFormat tfParam = fs.toTextCharFormat(TextEditor::C_STRING);

    const QString css = QString::fromLatin1("span.param {color: %1; background-color: %2;} "
                                            "span.option {color: %3; background-color: %4;} "
                                            "p {text-align: justify;}")
            .arg(tfParam.foreground().color().name())
            .arg(tfParam.background().style() == Qt::NoBrush
                 ? QString() : tfParam.background().color().name())
            .arg(tfOption.foreground().color().name())
            .arg(tfOption.background().style() == Qt::NoBrush
                 ? QString() : tfOption.background().color().name());
    ui->documentation->document()->setDefaultStyleSheet(css);
}

ConfigurationDialog::~ConfigurationDialog()
{
    delete ui;
}

void ConfigurationDialog::setSettings(AbstractSettings *settings)
{
    m_settings = settings;
    ui->editor->setSettings(m_settings);
}

void ConfigurationDialog::clear()
{
    ui->name->clear();
    ui->editor->clear();
    m_currentKey.clear();
    updateOkButton();
}

QString ConfigurationDialog::key() const
{
    return ui->name->text().simplified();
}

void ConfigurationDialog::setKey(const QString &key)
{
    m_currentKey = key;
    ui->name->setText(m_currentKey);
    if (m_settings)
        ui->editor->setPlainText(m_settings->style(m_currentKey));
    else
        ui->editor->clear();
}

QString ConfigurationDialog::value() const
{
    return ui->editor->toPlainText();
}

void ConfigurationDialog::updateOkButton()
{
    const QString key = ui->name->text().simplified();
    const bool exists = m_settings && key != m_currentKey && m_settings->styleExists(key);
    ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!(key.isEmpty() || exists));
}

void ConfigurationDialog::updateDocumentation(const QString &word, const QString &docu)
{
    if (word.isEmpty())
        ui->documentationHeader->setText(tr("Documentation"));
    else
        ui->documentationHeader->setText(tr("Documentation for \"%1\"").arg(word));
    ui->documentation->setHtml(docu);
}

} // namespace Internal
} // namespace Beautifier
