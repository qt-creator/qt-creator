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

#ifndef FINDPLUGIN_H
#define FINDPLUGIN_H

#include "find_global.h"

#include <extensionsystem/iplugin.h>

#include <QtGui/QTextDocument>

QT_BEGIN_NAMESPACE
class QStringListModel;
QT_END_NAMESPACE

namespace Find {
class IFindFilter;
struct FindPluginPrivate;

namespace Internal {
class FindToolBar;
class FindToolWindow;
class CurrentDocumentFind;
} // namespace Internal

class FIND_EXPORT FindPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    FindPlugin();
    virtual ~FindPlugin();
    static FindPlugin *instance();

    enum FindDirection {
        FindForward,
        FindBackward
    };

    // IPlugin
    bool initialize(const QStringList &arguments, QString *error_message);
    void extensionsInitialized();
    void shutdown();

    QTextDocument::FindFlags findFlags() const;
    void updateFindCompletion(const QString &text);
    void updateReplaceCompletion(const QString &text);
    QStringListModel *findCompletionModel() const;
    QStringListModel *replaceCompletionModel() const;
    void setUseFakeVim(bool on);
    void openFindToolBar(FindDirection direction);

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
    FindPluginPrivate *d;
};

} // namespace Find

#endif // FINDPLUGIN_H
