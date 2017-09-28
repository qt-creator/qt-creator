/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "coreplugin/core_global.h"
#include "coreplugin/coreconstants.h"
#include "coreplugin/icontext.h"
#include <coreplugin/actionmanager/command.h>

#include <QObject>
#include <QList>

QT_BEGIN_NAMESPACE
class QAction;
class QString;
QT_END_NAMESPACE

namespace Core {

class ActionContainer;
class Command;
class Context;

namespace Internal {
class CorePlugin;
class MainWindow;
} // Internal

class CORE_EXPORT ActionManager : public QObject
{
    Q_OBJECT
public:
    static ActionManager *instance();

    static ActionContainer *createMenu(Id id);
    static ActionContainer *createMenuBar(Id id);

    static Command *registerAction(QAction *action, Id id,
                                   const Context &context = Context(Constants::C_GLOBAL),
                                   bool scriptable = false);

    static Command *command(Id id);
    static ActionContainer *actionContainer(Id id);

    static QList<Command *> commands();

    static void unregisterAction(QAction *action, Id id);

    static void setPresentationModeEnabled(bool enabled);
    static bool isPresentationModeEnabled();

    static QString withNumberAccelerator(const QString &text, const int number);

signals:
    void commandListChanged();
    void commandAdded(Core::Id id);

private:
    ActionManager(QObject *parent = 0);
    ~ActionManager();
    static void saveSettings();
    static void setContext(const Context &context);

    friend class Core::Internal::CorePlugin; // initialization
    friend class Core::Internal::MainWindow; // saving settings and setting context
};

} // namespace Core
