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

#include "../core_global.h"
#include "../coreconstants.h"
#include "../icontext.h"
#include "command.h"

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

    static ActionContainer *createMenu(Utils::Id id);
    static ActionContainer *createMenuBar(Utils::Id id);
    static ActionContainer *createTouchBar(Utils::Id id,
                                           const QIcon &icon,
                                           const QString &text = QString());

    static Command *registerAction(QAction *action, Utils::Id id,
                                   const Context &context = Context(Constants::C_GLOBAL),
                                   bool scriptable = false);

    static Command *command(Utils::Id id);
    static ActionContainer *actionContainer(Utils::Id id);

    static QList<Command *> commands();

    static void unregisterAction(QAction *action, Utils::Id id);

    static void setPresentationModeEnabled(bool enabled);
    static bool isPresentationModeEnabled();

    static QString withNumberAccelerator(const QString &text, const int number);

signals:
    void commandListChanged();
    void commandAdded(Utils::Id id);

private:
    ActionManager(QObject *parent = nullptr);
    ~ActionManager() override;
    static void saveSettings();
    static void setContext(const Context &context);

    friend class Core::Internal::CorePlugin; // initialization
    friend class Core::Internal::MainWindow; // saving settings and setting context
};

} // namespace Core
