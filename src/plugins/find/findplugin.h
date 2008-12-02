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
***************************************************************************/
#ifndef FINDPLUGIN_H
#define FINDPLUGIN_H

#include "ui_findwidget.h"
#include "ifindfilter.h"
#include "findtoolbar.h"

#include <coreplugin/icore.h>
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

    Core::ICore *core();
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
    Core::ICore *m_core;
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
