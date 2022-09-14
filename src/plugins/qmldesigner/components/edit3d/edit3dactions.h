// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0
#pragma once

#include <abstractaction.h>

#include <QAction>
#include <QIcon>

namespace QmlDesigner {

using SelectionContextOperation = std::function<void(const SelectionContext &)>;

class Edit3DActionTemplate : public DefaultAction
{
public:
    Edit3DActionTemplate(const QString &description,
                         SelectionContextOperation action,
                         AbstractView *view,
                         View3DActionType type);

    void actionTriggered(bool b) override;

    SelectionContextOperation m_action;
    AbstractView *m_view;
    View3DActionType m_type;
};

class Edit3DAction : public AbstractAction
{
public:
    Edit3DAction(const QByteArray &menuId,
                 View3DActionType type,
                 const QString &description,
                 const QKeySequence &key,
                 bool checkable,
                 bool checked,
                 const QIcon &iconOff,
                 const QIcon &iconOn,
                 AbstractView *view,
                 SelectionContextOperation selectionAction = nullptr,
                 const QString &toolTip = {});

    QByteArray category() const override;

    int priority() const override
    {
        return CustomActionsPriority;
    }

    Type type() const override
    {
        return ActionInterface::Edit3DAction;
    }

    QByteArray menuId() const override
    {
        return m_menuId;
    }

protected:
    bool isVisible(const SelectionContext &selectionContext) const override;
    bool isEnabled(const SelectionContext &selectionContext) const override;

private:
    QByteArray m_menuId;
};

class Edit3DCameraAction : public Edit3DAction
{
public:
    Edit3DCameraAction(const QByteArray &menuId,
                       View3DActionType type,
                       const QString &description,
                       const QKeySequence &key,
                       bool checkable,
                       bool checked,
                       const QIcon &iconOff,
                       const QIcon &iconOn,
                       AbstractView *view,
                       SelectionContextOperation selectionAction = nullptr);

protected:
    bool isEnabled(const SelectionContext &selectionContext) const override;
};

} // namespace QmlDesigner
