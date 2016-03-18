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

#include <coreplugin/actionmanager/command_p.h>
#include <coreplugin/actionmanager/actioncontainer_p.h>
#include <coreplugin/icontext.h>

#include <QMap>
#include <QHash>
#include <QMultiHash>
#include <QTimer>

QT_BEGIN_NAMESPACE
class QLabel;
class QSettings;
QT_END_NAMESPACE

namespace Core {

namespace Internal {

class Action;
class ActionContainerPrivate;
class MainWindow;

class ActionManagerPrivate : public QObject
{
    Q_OBJECT

public:
    typedef QHash<Id, Action *> IdCmdMap;
    typedef QHash<Id, ActionContainerPrivate *> IdContainerMap;

    ~ActionManagerPrivate();

    void setContext(const Context &context);
    bool hasContext(int context) const;

    void saveSettings();
    void saveSettings(Action *cmd);

    void showShortcutPopup(const QString &shortcut);
    bool hasContext(const Context &context) const;
    Action *overridableAction(Id id);

    void readUserSettings(Id id, Action *cmd);

    void containerDestroyed();
    void actionTriggered();

    IdCmdMap m_idCmdMap;

    IdContainerMap m_idContainerMap;

    Context m_context;

    bool m_presentationModeEnabled = false;
};

} // namespace Internal
} // namespace Core
