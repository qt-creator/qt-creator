// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../core_global.h"

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
    QAction *actionForContext(const Utils::Id &contextId) const;

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
