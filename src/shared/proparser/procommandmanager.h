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

#ifndef PROCOMMANDMANAGER_H
#define PROCOMMANDMANAGER_H

#include "namespace_global.h"

#include <QtCore/QModelIndex>

QT_BEGIN_NAMESPACE
class ProItem;
QT_END_NAMESPACE

namespace Qt4ProjectManager {
namespace Internal {
    
class ProCommand
{
public:
    virtual ~ProCommand() {}
    virtual bool redo() = 0;
    virtual void undo() = 0;
};

class ProCommandGroup
{
public:
    ProCommandGroup(const QString &name);
    ~ProCommandGroup();

    void appendCommand(ProCommand *cmd);

    void undo();
    void redo();

private:
    QString m_name;
    QList<ProCommand *> m_commands;
};

class ProCommandManager : public QObject
{
    Q_OBJECT

public:
    ProCommandManager(QObject *parent);
    ~ProCommandManager();

    void beginGroup(const QString &name);
    void endGroup();

    // excutes the Command and adds it to the open group
    bool command(ProCommand *cmd);

    bool hasGroup() const;
    bool isDirty() const;

    void notifySave();

    bool canUndo() const;
    bool canRedo() const;

public slots:
    void undo();
    void redo();

signals:
    void modified();

private:
    ProCommandGroup *m_group;
    QList<ProCommandGroup *> m_groups;
    
    int m_pos;
    ProCommandGroup *m_savepoint;
};

} //namespace Internal
} //namespace Qt4ProjectManager

#endif // PROCOMMANDMANAGER_H
