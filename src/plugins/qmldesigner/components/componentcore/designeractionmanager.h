// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmldesignercorelib_global.h>
#include "actioninterface.h"
#include "modelnode.h"
#include "modelnodeoperations.h"

#include <coreplugin/actionmanager/command.h>
#include <utils/styledbar.h>

#include <QToolBar>
#include <QImage>

#include <functional>

QT_BEGIN_NAMESPACE
class QGraphicsItem;
class QGraphicsWidget;
class QMimeData;
QT_END_NAMESPACE

namespace QmlDesigner {

class DesignerActionManagerView;
class DesignerIcons;

using AddResourceOperation = std::function<AddFilesResult(const QStringList &, const QString &, bool)>;
using ModelNodePreviewImageOperation = std::function<QVariant(const ModelNode &)>;
using ActionAddedInterface = std::function<void(ActionInterface*)>;

struct AddResourceHandler
{
public:
    AddResourceHandler(const QString &_category,
                       const QString &_filter,
                       AddResourceOperation _operation,
                       int _priority = 0)
        : category(_category)
        , filter(_filter)
        , operation(_operation)
        , piority(_priority)
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

class QMLDESIGNERCOMPONENTS_EXPORT DesignerActionManager
{
public:
    DesignerActionManager(DesignerActionManagerView *designerActionManagerView, ExternalDependenciesInterface &externalDependencies);
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
    bool externalDragHasSupportedAssets(const QMimeData *data) const;
    QHash<QString, QStringList> handleExternalAssetsDrop(const QMimeData *data) const;
    QIcon contextIcon(int contextId) const;
    QIcon toolbarIcon(int contextId) const;

    void addAddActionCallback(ActionAddedInterface callback);

private:
    void addTransitionEffectAction(const TypeName &typeName);
    void addCustomTransitionEffectAction();
    void setupIcons();
    QString designerIconResourcesPath() const;

    QList<QSharedPointer<ActionInterface> > m_designerActions;
    DesignerActionManagerView *m_designerActionManagerView;
    QList<AddResourceHandler> m_addResourceHandler;
    QList<ModelNodePreviewImageHandler> m_modelNodePreviewImageHandlers;
    ExternalDependenciesInterface &m_externalDependencies;
    QScopedPointer<DesignerIcons> m_designerIcons;
    QList<ActionAddedInterface> m_callBacks;
};

} //QmlDesigner
