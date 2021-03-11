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

#include <qmldesignercorelib_global.h>
#include "actioninterface.h"
#include "modelnode.h"

#include <coreplugin/actionmanager/command.h>
#include <utils/styledbar.h>

#include <QToolBar>
#include <QImage>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QGraphicsWidget;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignerActionManagerView;

using AddResourceOperation = std::function<bool (const QStringList &, const QString &)>;
using ModelNodePreviewImageOperation = std::function<QVariant (const ModelNode &)>;

struct AddResourceHandler
{
public:
    AddResourceHandler( const QString &_category,
                        const QString &_filter,
                        AddResourceOperation _operation,
                        int _priority = 0)
        : category(_category)
        ,filter(_filter)
        ,operation(_operation)
        ,piority(_priority)
    {
    }

    QString category;
    QString filter;
    AddResourceOperation operation;
    int piority;
};

struct ModelNodePreviewImageHandler
{
public:
    ModelNodePreviewImageHandler(const TypeName &t,
                                 ModelNodePreviewImageOperation op,
                                 bool compOnly = false,
                                 int prio = 0)
        : type(t)
        , operation(op)
        , componentOnly(compOnly)
        , priority(prio)
    {
    }

    TypeName type;
    ModelNodePreviewImageOperation operation = nullptr;
    bool componentOnly = false;
    int priority = 0;
};

class DesignerActionToolBar : public Utils::StyledBar
{
public:
    DesignerActionToolBar(QWidget *parentWidget);
    void registerAction(ActionInterface *action);
    void addSeparator();

private:
    QToolBar *m_toolBar;
};

class QMLDESIGNERCORE_EXPORT DesignerActionManager {
public:
    DesignerActionManager(DesignerActionManagerView *designerActionManagerView);
    ~DesignerActionManager();

    void addDesignerAction(ActionInterface *newAction);
    void addCreatorCommand(Core::Command *command, const QByteArray &category, int priority,
                           const QIcon &overrideIcon = QIcon());

    QList<QSharedPointer<ActionInterface>> actionsForTargetView(const ActionInterface::TargetView &target);

    QList<ActionInterface* > designerActions() const;
    ActionInterface *actionByMenuId(const QByteArray &id);

    void createDefaultDesignerActions();
    void createDefaultAddResourceHandler();
    void createDefaultModelNodePreviewImageHandlers();

    DesignerActionManagerView *view();

    DesignerActionToolBar *createToolBar(QWidget *parent = nullptr) const;
    void polishActions() const;
    QGraphicsWidget *createFormEditorToolBar(QGraphicsItem *parent);

    static DesignerActionManager &instance();
    void setupContext();

    DesignerActionManager(const DesignerActionManager&) = delete;
    DesignerActionManager & operator=(const DesignerActionManager&) = delete;

    QList<AddResourceHandler> addResourceHandler() const;
    void registerAddResourceHandler(const AddResourceHandler &handler);
    void unregisterAddResourceHandlers(const QString &category);

    void registerModelNodePreviewHandler(const ModelNodePreviewImageHandler &handler);
    bool hasModelNodePreviewHandler(const ModelNode &node) const;
    ModelNodePreviewImageOperation modelNodePreviewOperation(const ModelNode &node) const;

private:
    void addTransitionEffectAction(const TypeName &typeName);
    void addCustomTransitionEffectAction();

    QList<QSharedPointer<ActionInterface> > m_designerActions;
    DesignerActionManagerView *m_designerActionManagerView;
    QList<AddResourceHandler> m_addResourceHandler;
    QList<ModelNodePreviewImageHandler> m_modelNodePreviewImageHandlers;
};

} //QmlDesigner
