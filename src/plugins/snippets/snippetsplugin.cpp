/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "snippetswindow.h"
#include "snippetscompletion.h"
#include "snippetsplugin.h"
#include "snippetspec.h"

#include <QtCore/QDebug>
#include <QtCore/QtPlugin>
#include <QtGui/QApplication>
#include <QtGui/QShortcut>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/baseview.h>
#include <coreplugin/icore.h>
#include <coreplugin/iview.h>
#include <texteditor/itexteditable.h>
#include <texteditor/texteditorconstants.h>

using namespace Snippets::Internal;

SnippetsPlugin *SnippetsPlugin::m_instance = 0;

SnippetsPlugin::SnippetsPlugin()
{
    m_instance = this;
    m_snippetsCompletion = 0;
}

SnippetsPlugin::~SnippetsPlugin()
{
    removeObject(m_snippetsCompletion);
    delete m_snippetsCompletion;
}

void SnippetsPlugin::extensionsInitialized()
{
}

bool SnippetsPlugin::initialize(const QStringList &arguments, QString *)
{
    Q_UNUSED(arguments);
    Core::ICore *core = Core::ICore::instance();
    Core::ActionManager *am = core->actionManager();

    QList<int> context;
    context << core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);

    m_snippetWnd = new SnippetsWindow();
    Core::BaseView *view = new Core::BaseView;
    view->setUniqueViewName("Snippets");
    view->setWidget(m_snippetWnd);
    view->setContext(QList<int>()
        << core->uniqueIDManager()->uniqueIdentifier(QLatin1String("Snippets Window"))
        << core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR));
    //view->setDefaultPosition(Qt::RightDockWidgetArea));
    addAutoReleasedObject(view);
    m_snippetsCompletion = new SnippetsCompletion(this);
    addObject(m_snippetsCompletion);

    foreach (SnippetSpec *snippet, m_snippetWnd->snippets()) {
        QShortcut *sc = new QShortcut(m_snippetWnd);
        Core::Command *cmd = am->registerShortcut(sc, simplifySnippetName(snippet), context);
        cmd->setCategory(tr("Snippets"));
        connect(sc, SIGNAL(activated()), this, SLOT(snippetActivated()));
        m_shortcuts.insert(sc, snippet);
    }

    return true;
}

QString SnippetsPlugin::simplifySnippetName(SnippetSpec *snippet) const
{
    return QLatin1String("Snippets.")
        + snippet->category().simplified().replace(QLatin1String(" "), QLatin1String(""))
        + QLatin1Char('.')
        + snippet->name().simplified().replace(QLatin1String(" "), QLatin1String(""));
}

void SnippetsPlugin::snippetActivated()
{
    Core::ICore *core = Core::ICore::instance();
    SnippetSpec *snippet = m_shortcuts.value(sender());
    if (snippet && core->editorManager()->currentEditor()) {
        TextEditor::ITextEditable *te =
            qobject_cast<TextEditor::ITextEditable *>(
                    core->editorManager()->currentEditor());
        m_snippetWnd->insertSnippet(te, snippet);
    }
}

Q_EXPORT_PLUGIN(SnippetsPlugin)
