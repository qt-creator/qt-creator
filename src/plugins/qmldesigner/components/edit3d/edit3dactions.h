// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include <abstractaction.h>

#include <QAction>
#include <QWidgetAction>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QWidgetAction;
QT_END_NAMESPACE

namespace QmlDesigner {

using SelectionContextOperation = std::function<void(const SelectionContext &)>;
class Edit3DView;
class SeekerSliderAction;

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

class Edit3DWidgetActionTemplate : public PureActionInterface
{
    Q_DISABLE_COPY(Edit3DWidgetActionTemplate)

public:
    explicit Edit3DWidgetActionTemplate(QWidgetAction *widget);
    virtual void setSelectionContext(const SelectionContext &) {}
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
                 const QIcon &icon,
                 Edit3DView *view,
                 SelectionContextOperation selectionAction = nullptr,
                 const QString &toolTip = {});

    Edit3DAction(const QByteArray &menuId,
                 View3DActionType type,
                 Edit3DView *view,
                 PureActionInterface *pureInt);

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
    View3DActionType m_actionType;
};

class Edit3DParticleSeekerAction : public Edit3DAction
{
public:
    Edit3DParticleSeekerAction(const QByteArray &menuId,
                               View3DActionType type,
                               Edit3DView *view);

    SeekerSliderAction *seekerAction();

protected:
    bool isVisible(const SelectionContext &) const override;
    bool isEnabled(const SelectionContext &) const override;

private:
    SeekerSliderAction *m_seeker = nullptr;
};

class Edit3DBakeLightsAction : public Edit3DAction
{
public:
    Edit3DBakeLightsAction(const QIcon &icon,
                           Edit3DView *view,
                           SelectionContextOperation selectionAction);

protected:
    bool isVisible(const SelectionContext &) const override;
    bool isEnabled(const SelectionContext &) const override;

private:
    Edit3DView *m_view = nullptr;
};

} // namespace QmlDesigner
