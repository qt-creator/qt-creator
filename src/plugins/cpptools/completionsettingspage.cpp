/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
    : m_page(0)
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
    m_page = new Ui_CompletionSettingsPage;
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

    int completionTriggerIndex = 0;
    switch (settings.m_completionTrigger) {
    case TextEditor::ManualCompletion:
        completionTriggerIndex = 0;
        break;
    case TextEditor::TriggeredCompletion:
        completionTriggerIndex = 1;
        break;
    case TextEditor::AutomaticCompletion:
        completionTriggerIndex = 2;
        break;
    }

    m_page->caseSensitivity->setCurrentIndex(caseSensitivityIndex);
    m_page->completionTrigger->setCurrentIndex(completionTriggerIndex);
    m_page->autoInsertBrackets->setChecked(settings.m_autoInsertBrackets);
    m_page->partiallyComplete->setChecked(settings.m_partiallyComplete);
    m_page->spaceAfterFunctionName->setChecked(settings.m_spaceAfterFunctionName);

    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << m_page->caseSensitivityLabel->text()
                << ' ' << m_page->autoInsertBrackets->text()
                << ' ' << m_page->completionTriggerLabel->text()
                << ' ' << m_page->partiallyComplete->text()
                << ' ' << m_page->spaceAfterFunctionName->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }

    return w;
}

void CompletionSettingsPage::apply()
{
    if (!m_page) // page was never shown
        return;
    TextEditor::CompletionSettings settings;
    settings.m_caseSensitivity = caseSensitivity();
    settings.m_completionTrigger = completionTrigger();
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

TextEditor::CompletionTrigger CompletionSettingsPage::completionTrigger() const
{
    switch (m_page->completionTrigger->currentIndex()) {
    case 0:
        return TextEditor::ManualCompletion;
    case 1:
        return TextEditor::TriggeredCompletion;
    default:
        return TextEditor::AutomaticCompletion;
    }
}

void CompletionSettingsPage::finish()
{
    if (!m_page) // page was never shown
        return;
    delete m_page;
    m_page = 0;
}
