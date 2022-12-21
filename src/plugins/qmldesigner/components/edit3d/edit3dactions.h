// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <abstractaction.h>

#include <QAction>
#include <QIcon>

namespace QmlDesigner {

using SelectionContextOperation = std::function<void(const SelectionContext &)>;
class Edit3DView;

class Edit3DActionTemplate : public DefaultAction
{
    Q_OBJECT

public:
    Edit3DActionTemplate(const QString &description,
                         SelectionContextOperation action,
                         Edit3DView *view,
                         View3DActionType type);

    void actionTriggered(bool b) override;

    SelectionContextOperation m_action;
    Edit3DView *m_view = nullptr;
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
                 Edit3DView *view,
                 SelectionContextOperation selectionAction = nullptr,
                 const QString &toolTip = {});

    virtual ~Edit3DAction();

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

    View3DActionType actionType() const;

protected:
    bool isVisible(const SelectionContext &selectionContext) const override;
    bool isEnabled(const SelectionContext &selectionContext) const override;

private:
    QByteArray m_menuId;
    Edit3DActionTemplate *m_actionTemplate = nullptr;
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
                       Edit3DView *view,
                       SelectionContextOperation selectionAction = nullptr);

protected:
    bool isEnabled(const SelectionContext &selectionContext) const override;
};

} // namespace QmlDesigner
