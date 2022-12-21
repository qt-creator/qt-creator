// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QPointer>
#include <QAction>

namespace Utils {

class QTCREATOR_UTILS_EXPORT ProxyAction : public QAction
{
    Q_OBJECT
public:
    enum Attribute {
        Hide = 0x01,
        UpdateText = 0x02,
        UpdateIcon = 0x04
    };
    Q_DECLARE_FLAGS(Attributes, Attribute)

    explicit ProxyAction(QObject *parent = nullptr);

    void initialize(QAction *action);

    void setAction(QAction *action);
    QAction *action() const;

    bool shortcutVisibleInToolTip() const;
    void setShortcutVisibleInToolTip(bool visible);

    void setAttribute(Attribute attribute);
    void removeAttribute(Attribute attribute);
    bool hasAttribute(Attribute attribute);

    static QString stringWithAppendedShortcut(const QString &str, const QKeySequence &shortcut);
    static ProxyAction *proxyActionWithIcon(QAction *original, const QIcon &newIcon);

signals:
    void currentActionChanged(QAction *action);

private:
    void actionChanged();
    void updateState();
    void updateToolTipWithKeySequence();
    void disconnectAction();
    void connectAction();
    void update(QAction *action, bool initialize);

    QPointer<QAction> m_action;
    Attributes m_attributes;
    bool m_showShortcut = false;
    QString m_toolTip;
    bool m_block = false;
};

} // namespace Utils

Q_DECLARE_OPERATORS_FOR_FLAGS(Utils::ProxyAction::Attributes)
