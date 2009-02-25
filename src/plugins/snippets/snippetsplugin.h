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
