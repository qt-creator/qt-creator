/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008-2009 Nokia Corporation and/or its subsidiary(-ies).
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
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SNIPPETS_H
#define SNIPPETS_H

#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtGui/QShortcut>

#include <extensionsystem/iplugin.h>

namespace Snippets {
namespace Internal {

class SnippetsWindow;
class SnippetSpec;
class SnippetsCompletion;

class SnippetsPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    SnippetsPlugin();
    virtual ~SnippetsPlugin();

    static SnippetsPlugin *instance() { return m_instance; }
    static SnippetsWindow *snippetsWindow() { return m_instance->m_snippetWnd; }

    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

private slots:
    void snippetActivated();

private:
    static SnippetsPlugin *m_instance;

    QString simplifySnippetName(SnippetSpec *snippet) const;
    SnippetsCompletion *m_snippetsCompletion;
    SnippetsWindow *m_snippetWnd;

    int m_textContext;
    int m_snippetsMode;
    QShortcut *m_exitShortcut;
    QShortcut *m_modeShortcut;
    QMap<QObject*, SnippetSpec*> m_shortcuts;
};

} // namespace Internal
} // namespace Snippets

#endif // SNIPPETS_H
