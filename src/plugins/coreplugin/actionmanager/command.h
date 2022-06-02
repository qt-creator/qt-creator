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

#include <coreplugin/core_global.h>

#include <utils/hostosinfo.h>
#include <utils/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QKeySequence;
class QToolButton;
QT_END_NAMESPACE


namespace Core {
namespace Internal {
class ActionManagerPrivate;
class CommandPrivate;
} // namespace Internal

class ActionManager;
class Context;

constexpr bool useMacShortcuts = Utils::HostOsInfo::isMacHost();

class CORE_EXPORT Command : public QObject
{
    Q_OBJECT
public:
    enum CommandAttribute {
        CA_Hide = 1,
        CA_UpdateText = 2,
        CA_UpdateIcon = 4,
        CA_NonConfigurable = 8
    };
    Q_DECLARE_FLAGS(CommandAttributes, CommandAttribute)

    ~Command();

    void setDefaultKeySequence(const QKeySequence &key);
    void setDefaultKeySequences(const QList<QKeySequence> &keys);
    QList<QKeySequence> defaultKeySequences() const;
    QList<QKeySequence> keySequences() const;
    QKeySequence keySequence() const;
    // explicitly set the description (used e.g. in shortcut settings)
    // default is to use the action text for actions, or the whatsThis for shortcuts,
    // or, as a last fall back if these are empty, the command ID string
    // override the default e.g. if the text is context dependent and contains file names etc
    void setDescription(const QString &text);
    QString description() const;

    Utils::Id id() const;

    QAction *action() const;
    Context context() const;

    void setAttribute(CommandAttribute attr);
    void removeAttribute(CommandAttribute attr);
    bool hasAttribute(CommandAttribute attr) const;

    bool isActive() const;

    void setKeySequences(const QList<QKeySequence> &keys);
    QString stringWithAppendedShortcut(const QString &str) const;
    void augmentActionWithShortcutToolTip(QAction *action) const;
    static QToolButton *toolButtonWithAppendedShortcut(QAction *action, Command *cmd);

    bool isScriptable() const;
    bool isScriptable(const Context &) const;

    void setTouchBarText(const QString &text);
    QString touchBarText() const;
    void setTouchBarIcon(const QIcon &icon);
    QIcon touchBarIcon() const;
    QAction *touchBarAction() const;

signals:
    void keySequenceChanged();
    void activeStateChanged();

private:
    friend class ActionManager;
    friend class Internal::ActionManagerPrivate;

    Command(Utils::Id id);

    Internal::CommandPrivate *d;
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)
