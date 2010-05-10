/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "completionsettingspage.h"
#include "ui_completionsettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

using namespace CppTools::Internal;

CompletionSettingsPage::CompletionSettingsPage()
    : m_page(new Ui_CompletionSettingsPage)
{
}

CompletionSettingsPage::~CompletionSettingsPage()
{
    delete m_page;
}

QString CompletionSettingsPage::id() const
{
    return QLatin1String("P.Completion");
}

QString CompletionSettingsPage::displayName() const
{
    return tr("Completion");
}

QWidget *CompletionSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    const TextEditor::CompletionSettings &settings =
            TextEditor::TextEditorSettings::instance()->completionSettings();

    int caseSensitivityIndex = 0;
    switch (settings.m_caseSensitivity) {
    case TextEditor::CaseSensitive:
        caseSensitivityIndex = 0;
        break;
    case TextEditor::CaseInsensitive:
        caseSensitivityIndex = 1;
        break;
    case TextEditor::FirstLetterCaseSensitive:
        caseSensitivityIndex = 2;
        break;
    }

    m_page->caseSensitivity->setCurrentIndex(caseSensitivityIndex);
    m_page->autoInsertBrackets->setChecked(settings.m_autoInsertBrackets);
    m_page->partiallyComplete->setChecked(settings.m_partiallyComplete);
    m_page->spaceAfterFunctionName->setChecked(settings.m_spaceAfterFunctionName);

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << m_page->caseSensitivityLabel->text()
                << ' ' << m_page->autoInsertBrackets->text()
		<< ' ' << m_page->partiallyComplete->text()
		<< ' ' << m_page->spaceAfterFunctionName->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    return w;
}

void CompletionSettingsPage::apply()
{
    TextEditor::CompletionSettings settings;
    settings.m_caseSensitivity = caseSensitivity();
    settings.m_autoInsertBrackets = m_page->autoInsertBrackets->isChecked();
    settings.m_partiallyComplete = m_page->partiallyComplete->isChecked();
    settings.m_spaceAfterFunctionName = m_page->spaceAfterFunctionName->isChecked();

    TextEditor::TextEditorSettings::instance()->setCompletionSettings(settings);
}

bool CompletionSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

TextEditor::CaseSensitivity CompletionSettingsPage::caseSensitivity() const
{
    switch (m_page->caseSensitivity->currentIndex()) {
    case 0: // Full
        return TextEditor::CaseSensitive;
    case 1: // None
        return TextEditor::CaseInsensitive;
    default: // First letter
        return TextEditor::FirstLetterCaseSensitive;
    }
}
