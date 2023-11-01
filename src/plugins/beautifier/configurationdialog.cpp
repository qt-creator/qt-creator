// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "configurationdialog.h"

#include "beautifiertool.h"
#include "beautifiertr.h"
#include "configurationeditor.h"

#include <texteditor/fontsettings.h>
#include <texteditor/texteditorsettings.h>

#include <utils/layoutbuilder.h>

#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRegularExpressionValidator>
#include <QSplitter>
#include <QTextEdit>

namespace Beautifier::Internal {

ConfigurationDialog::ConfigurationDialog(QWidget *parent)
    : QDialog(parent)
{
    resize(640, 512);

    m_name = new QLineEdit;

    m_editor = new ConfigurationEditor;

    m_documentationHeader = new QLabel;

    m_documentation = new QTextEdit;
    m_documentation->setReadOnly(true);

    m_buttonBox = new QDialogButtonBox(this);
    m_buttonBox->setOrientation(Qt::Horizontal);
    m_buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);

    using namespace Layouting;

    Column {
        Group {
            title(Tr::tr("Name")),
            Column { m_name }
        },
        Group {
            title(Tr::tr("Value")),
            Column {
                m_editor,
                m_documentationHeader,
                m_documentation
            }
        },
        m_buttonBox
    }.attachTo(this);

    connect(m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    // Filter out characters which are not allowed in a file name
    QRegularExpressionValidator *fileNameValidator = new QRegularExpressionValidator(
                QRegularExpression("^[^\\/\\\\\\?\\>\\<\\*\\%\\:\\\"\\']*$"), m_name);
    m_name->setValidator(fileNameValidator);

    updateDocumentation();
    connect(m_name, &QLineEdit::textChanged, this, &ConfigurationDialog::updateOkButton);
    updateOkButton(); // force initial test.
    connect(m_editor, &ConfigurationEditor::documentationChanged,
            this, &ConfigurationDialog::updateDocumentation);

    // Set palette and font according to settings
    const TextEditor::FontSettings fs = TextEditor::TextEditorSettings::fontSettings();
    const QTextCharFormat tf = fs.toTextCharFormat(TextEditor::C_TEXT);
    const QTextCharFormat selectionFormat = fs.toTextCharFormat(TextEditor::C_SELECTION);

    QPalette pal;
    pal.setColor(QPalette::Base, tf.background().color());
    pal.setColor(QPalette::Text, tf.foreground().color());
    pal.setColor(QPalette::WindowText, tf.foreground().color());
    if (selectionFormat.background().style() != Qt::NoBrush)
        pal.setColor(QPalette::Highlight, selectionFormat.background().color());
    pal.setBrush(QPalette::HighlightedText, selectionFormat.foreground());
    m_documentation->setPalette(pal);
    m_editor->setPalette(pal);

    m_documentation->setFont(tf.font());
    m_editor->setFont(tf.font());

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
    m_documentation->document()->setDefaultStyleSheet(css);
}

ConfigurationDialog::~ConfigurationDialog() = default;

void ConfigurationDialog::setSettings(AbstractSettings *settings)
{
    m_settings = settings;
    m_editor->setSettings(m_settings);
}

void ConfigurationDialog::clear()
{
    m_name->clear();
    m_editor->clear();
    m_currentKey.clear();
    updateOkButton();
}

QString ConfigurationDialog::key() const
{
    return m_name->text().simplified();
}

void ConfigurationDialog::setKey(const QString &key)
{
    m_currentKey = key;
    m_name->setText(m_currentKey);
    if (m_settings)
        m_editor->setPlainText(m_settings->style(m_currentKey));
    else
        m_editor->clear();
}

QString ConfigurationDialog::value() const
{
    return m_editor->toPlainText();
}

void ConfigurationDialog::updateOkButton()
{
    const QString key = m_name->text().simplified();
    const bool exists = m_settings && key != m_currentKey && m_settings->styleExists(key);
    m_buttonBox->button(QDialogButtonBox::Ok)->setEnabled(!(key.isEmpty() || exists));
}

void ConfigurationDialog::updateDocumentation(const QString &word, const QString &docu)
{
    if (word.isEmpty())
        m_documentationHeader->setText(Tr::tr("Documentation"));
    else
        m_documentationHeader->setText(Tr::tr("Documentation for \"%1\"").arg(word));
    m_documentation->setHtml(docu);
}

} // Beautifier::Internal
