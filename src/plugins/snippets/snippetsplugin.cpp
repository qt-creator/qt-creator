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
#include "snippetswindow.h"
#include "snippetscompletion.h"
#include "snippetsplugin.h"
#include "snippetspec.h"

#include <QtCore/qplugin.h>
#include <QtCore/QDebug>
#include <QtGui/QShortcut>
#include <QtGui/QApplication>

#include <extensionsystem/pluginmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/CoreTools>
#include <texteditor/itexteditable.h>
#include <texteditor/texteditorconstants.h>

using namespace Snippets::Internal;

SnippetsPlugin *SnippetsPlugin::m_instance = 0;

SnippetsPlugin::SnippetsPlugin()
{
    m_instance = this;
}

SnippetsPlugin::~SnippetsPlugin()
{
    removeObject(m_snippetsCompletion);
    delete m_snippetsCompletion;
}

void SnippetsPlugin::extensionsInitialized()
{
}

bool SnippetsPlugin::initialize(const QStringList & /*arguments*/, QString *)
{
    m_core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    Core::ActionManagerInterface *am = m_core->actionManager();

    QList<int> context;
    context << m_core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);

    m_snippetWnd = new SnippetsWindow();
    addAutoReleasedObject(new Core::BaseView("Snippets.SnippetsTree",
        m_snippetWnd,
        QList<int>() << m_core->uniqueIDManager()->uniqueIdentifier(QLatin1String("Snippets Window"))
                     << m_core->uniqueIDManager()->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR),
        Qt::RightDockWidgetArea));
    m_snippetsCompletion = new SnippetsCompletion(this);
    addObject(m_snippetsCompletion);

    foreach (SnippetSpec *snippet, m_snippetWnd->snippets()) {
        QShortcut *sc = new QShortcut(m_snippetWnd);
        Core::ICommand *cmd = am->registerShortcut(sc, simplifySnippetName(snippet), context);
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
    SnippetSpec *snippet = m_shortcuts.value(sender());
    if (snippet && m_core->editorManager()->currentEditor()) {
        TextEditor::ITextEditable *te =
            qobject_cast<TextEditor::ITextEditable *>(
                    m_core->editorManager()->currentEditor());
        m_snippetWnd->insertSnippet(te, snippet);
    }
}

Q_EXPORT_PLUGIN(SnippetsPlugin)
