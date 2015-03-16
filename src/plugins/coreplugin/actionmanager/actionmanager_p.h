/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ACTIONMANAGERPRIVATE_H
#define ACTIONMANAGERPRIVATE_H

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

    explicit ActionManagerPrivate();
    ~ActionManagerPrivate();

    void setContext(const Context &context);
    bool hasContext(int context) const;

    void saveSettings();
    void saveSettings(Action *cmd);

    void showShortcutPopup(const QString &shortcut);
    bool hasContext(const Context &context) const;
    Action *overridableAction(Id id);

    void readUserSettings(Id id, Action *cmd);

public slots:
    void containerDestroyed();

    void actionTriggered();

public:
    IdCmdMap m_idCmdMap;

    IdContainerMap m_idContainerMap;

    Context m_context;

    bool m_presentationModeEnabled;
};

} // namespace Internal
} // namespace Core

#endif // ACTIONMANAGERPRIVATE_H
