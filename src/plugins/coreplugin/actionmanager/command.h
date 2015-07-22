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
#include <coreplugin/id.h>

#include <utils/hostosinfo.h>

#include <QObject>

QT_BEGIN_NAMESPACE
class QAction;
class QKeySequence;
class QToolButton;
QT_END_NAMESPACE


namespace Core {

class Context;

enum { UseMacShortcuts = Utils::HostOsInfo::isMacHost() ? 1 : 0 };

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

    virtual void setDefaultKeySequence(const QKeySequence &key) = 0;
    virtual QKeySequence defaultKeySequence() const = 0;
    virtual QKeySequence keySequence() const = 0;
    // explicitly set the description (used e.g. in shortcut settings)
    // default is to use the action text for actions, or the whatsThis for shortcuts,
    // or, as a last fall back if these are empty, the command ID string
    // override the default e.g. if the text is context dependent and contains file names etc
    virtual void setDescription(const QString &text) = 0;
    virtual QString description() const = 0;

    virtual Id id() const = 0;

    virtual QAction *action() const = 0;
    virtual Context context() const = 0;

    virtual void setAttribute(CommandAttribute attr) = 0;
    virtual void removeAttribute(CommandAttribute attr) = 0;
    virtual bool hasAttribute(CommandAttribute attr) const = 0;

    virtual bool isActive() const = 0;

    virtual void setKeySequence(const QKeySequence &key) = 0;
    virtual QString stringWithAppendedShortcut(const QString &str) const = 0;
    void augmentActionWithShortcutToolTip(QAction *action) const;
    static QToolButton *toolButtonWithAppendedShortcut(QAction *action, Command *cmd);

    virtual bool isScriptable() const = 0;
    virtual bool isScriptable(const Context &) const = 0;

signals:
    void keySequenceChanged();
    void activeStateChanged();
};

} // namespace Core

Q_DECLARE_OPERATORS_FOR_FLAGS(Core::Command::CommandAttributes)
