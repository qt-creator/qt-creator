/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#include "completionsettingspage.h"
#include "cppcodecompletion.h"
#include "ui_completionsettingspage.h"

#include <coreplugin/icore.h>
#include <extensionsystem/pluginmanager.h>

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

QString CompletionSettingsPage::name() const
{
    return tr("Completion");
}

QString CompletionSettingsPage::category() const
{
    return QLatin1String("TextEditor");
}

QString CompletionSettingsPage::trCategory() const
{
    return tr("Text Editor");
}

QWidget *CompletionSettingsPage::createPage(QWidget *parent)
{
    QWidget *w = new QWidget(parent);
    m_page->setupUi(w);

    m_page->caseSensitive->setChecked(m_completion->caseSensitivity() == Qt::CaseSensitive);
    m_page->autoInsertBraces->setChecked(m_completion->autoInsertBraces());
    m_page->partiallyComplete->setChecked(m_completion->isPartialCompletionEnabled());

    return w;
}

void CompletionSettingsPage::apply()
{
    m_completion->setCaseSensitivity(
            m_page->caseSensitive->isChecked() ? Qt::CaseSensitive : Qt::CaseInsensitive);
    m_completion->setAutoInsertBraces(m_page->autoInsertBraces->isChecked());
    m_completion->setPartialCompletionEnabled(m_page->partiallyComplete->isChecked());
}
