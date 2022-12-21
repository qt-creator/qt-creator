// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "actioninterface.h"

#include <QAction>
#include <QMenu>
#include <QScopedPointer>

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT AbstractActionGroup : public ActionInterface
{
public:
    AbstractActionGroup(const QString &displayName);

    virtual bool isVisible(const SelectionContext &m_selectionState) const = 0;
    virtual bool isEnabled(const SelectionContext &m_selectionState) const = 0;
    ActionInterface::Type type() const override;
    QAction *action() const override;
    QMenu *menu() const;
    SelectionContext selectionContext() const;

    void currentContextChanged(const SelectionContext &selectionContext) override;
    virtual void updateContext();

private:
    const QString m_displayName;
    SelectionContext m_selectionContext;
    QScopedPointer<QMenu> m_menu;
    QAction *m_action;
};

} // namespace QmlDesigner
