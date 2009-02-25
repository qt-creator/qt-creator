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

#ifndef FINDPLUGIN_H
#define FINDPLUGIN_H

#include "ui_findwidget.h"
#include "ifindfilter.h"
#include "findtoolbar.h"

#include <extensionsystem/iplugin.h>

#include <QtCore/QHash>
#include <QtCore/QStringList>
#include <QtGui/QAction>
#include <QtGui/QTextDocument>

namespace Find {
namespace Internal {

class FindToolWindow;

class FindPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    FindPlugin();
    virtual ~FindPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    void shutdown();

    QTextDocument::FindFlags findFlags() const;
    void updateFindCompletion(const QString &text);
    void updateReplaceCompletion(const QString &text);
    QStringListModel *findCompletionModel() { return m_findCompletionModel; }
    QStringListModel *replaceCompletionModel() { return m_replaceCompletionModel; }

public slots:
    void setCaseSensitive(bool sensitive);
    void setWholeWord(bool wholeOnly);
    void setBackward(bool backward);

signals:
    void findFlagsChanged();

private slots:
    void filterChanged();
    void openFindFilter();

private:
    void setFindFlag(QTextDocument::FindFlag flag, bool enabled);
    bool hasFindFlag(QTextDocument::FindFlag flag);
    void updateCompletion(const QString &text, QStringList &completions, QStringListModel *model);
    void setupMenu();
    void setupFilterMenuItems();
    void writeSettings();
    void readSettings();

    //variables
    QHash<IFindFilter *, QAction *> m_filterActions;

    CurrentDocumentFind *m_currentDocumentFind;
    FindToolBar *m_findToolBar;
    FindToolWindow *m_findDialog;
    QTextDocument::FindFlags m_findFlags;
    QStringListModel *m_findCompletionModel;
    QStringListModel *m_replaceCompletionModel;
    QStringList m_findCompletions;
    QStringList m_replaceCompletions;
};

} // namespace Internal
} // namespace Find

#endif // FINDPLUGIN_H
