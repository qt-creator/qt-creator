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
#include <texteditor/texteditorconstants.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QTextStream>
#include <QtCore/QCoreApplication>

using namespace CppTools::Internal;

CompletionSettingsPage::CompletionSettingsPage(CppCodeCompletion *completion)
    : m_completion(completion)
    , m_page(new Ui_CompletionSettingsPage)
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

QString CompletionSettingsPage::category() const
{
    return QLatin1String(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
}

QString CompletionSettingsPage::displayCategory() const
{
    return QCoreApplication::translate("Text Editor", TextEditor::Constants::TEXT_EDITOR_SETTINGS_TR_CATEGORY);
}

QIcon CompletionSettingsPage::categoryIcon() const
{
    return QIcon(QLatin1String(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON));
}

QWidget *CompletionSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    int caseSensitivityIndex = 0;
    switch (m_completion->caseSensitivity()) {
    case CppCodeCompletion::CaseSensitive:
        caseSensitivityIndex = 0;
        break;
    case CppCodeCompletion::CaseInsensitive:
        caseSensitivityIndex = 1;
        break;
    case CppCodeCompletion::FirstLetterCaseSensitive:
        caseSensitivityIndex = 2;
        break;
    }

    m_page->caseSensitivity->setCurrentIndex(caseSensitivityIndex);
    m_page->autoInsertBrackets->setChecked(m_completion->autoInsertBrackets());
    m_page->partiallyComplete->setChecked(m_completion->isPartialCompletionEnabled());
    if (m_searchKeywords.isEmpty()) {
        QTextStream(&m_searchKeywords) << m_page->caseSensitivityLabel->text()
                << ' ' << m_page->autoInsertBrackets->text()
                << ' ' << m_page->partiallyComplete->text();
        m_searchKeywords.remove(QLatin1Char('&'));
    }
    return w;
}

void CompletionSettingsPage::apply()
{
    m_completion->setCaseSensitivity(caseSensitivity());
    m_completion->setAutoInsertBrackets(m_page->autoInsertBrackets->isChecked());
    m_completion->setPartialCompletionEnabled(m_page->partiallyComplete->isChecked());
}

bool CompletionSettingsPage::matches(const QString &s) const
{
    return m_searchKeywords.contains(s, Qt::CaseInsensitive);
}

CppCodeCompletion::CaseSensitivity CompletionSettingsPage::caseSensitivity() const
{
    switch (m_page->caseSensitivity->currentIndex()) {
    case 0: // Full
        return CppCodeCompletion::CaseSensitive;
    case 1: // None
        return CppCodeCompletion::CaseInsensitive;
    default: // First letter
        return CppCodeCompletion::FirstLetterCaseSensitive;
    }
}
