// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "command.h"

#include "../icontext.h"

#include <utils/id.h>
#include <utils/proxyaction.h>

#include <QList>
#include <QMultiMap>
#include <QPointer>
#include <QMap>
#include <QKeySequence>

#include <memory>

namespace Core {
namespace Internal {

class CommandPrivate : public QObject
{
    Q_OBJECT
public:
    CommandPrivate(Command *parent);

    void setCurrentContext(const Context &context);

    void addOverrideAction(QAction *action, const Context &context, bool scriptable);
    void removeOverrideAction(QAction *action);
    bool isEmpty() const;

    void updateActiveState();
    void setActive(bool state);

    Command *m_q = nullptr;
    Context m_context;
    Command::CommandAttributes m_attributes;
    Utils::Id m_id;
    QList<QKeySequence> m_defaultKeys;
    QString m_defaultText;
    QString m_touchBarText;
    QIcon m_touchBarIcon;
    bool m_isKeyInitialized = false;

    Utils::ProxyAction *m_action = nullptr;
    mutable std::unique_ptr<Utils::ProxyAction> m_touchBarAction;
    QString m_toolTip;

    QMap<Utils::Id, QPointer<QAction> > m_contextActionMap;
    QMap<QAction*, bool> m_scriptableMap;
    bool m_active = false;
    bool m_contextInitialized = false;
};

} // namespace Internal
} // namespace Core
