// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "actioninterface.h"

#include <QAction>
#include <QScopedPointer>

namespace QmlDesigner {

class QMLDESIGNERCOMPONENTS_EXPORT DefaultAction : public QAction
{
    Q_OBJECT

public:
    DefaultAction(const QString &description);

    // virtual function instead of slot
    virtual void actionTriggered([[maybe_unused]] bool enable) {}
    void setSelectionContext(const SelectionContext &selectionContext);

protected:
    SelectionContext m_selectionContext;
};

class QMLDESIGNERCOMPONENTS_EXPORT AbstractAction : public ActionInterface
{
public:
    AbstractAction(const QString &description = QString());
    AbstractAction(DefaultAction *action);

    QAction *action() const override final;
    DefaultAction *defaultAction() const;

    void currentContextChanged(const SelectionContext &selectionContext) override;

protected:
    virtual void updateContext();
    virtual bool isChecked(const SelectionContext &selectionContext) const;
    virtual bool isVisible(const SelectionContext &selectionContext) const = 0;
    virtual bool isEnabled(const SelectionContext &selectionContext) const = 0;

    void setCheckable(bool checkable);
    SelectionContext selectionContext() const;

private:
    QScopedPointer<DefaultAction> m_defaultAction;
    SelectionContext m_selectionContext;
};

} // namespace QmlDesigner
