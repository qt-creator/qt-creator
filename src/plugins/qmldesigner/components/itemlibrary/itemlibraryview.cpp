// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryview.h"
#include "itemlibrarywidget.h"
#include "metainfo.h"
#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <componentcore_constants.h>
#include <coreplugin/icore.h>
#include <customnotifications.h>
#include <import.h>
#include <nodelistproperty.h>
#include <projectexplorer/kit.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <qmlitemnode.h>
#include <rewriterview.h>
#include <sqlitedatabase.h>
#include <utils/algorithm.h>

namespace QmlDesigner {

ItemLibraryView::ItemLibraryView(AsynchronousImageCache &imageCache,
                                 ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
{}

ItemLibraryView::~ItemLibraryView()
{
}

bool ItemLibraryView::hasWidget() const
{
    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    if (m_widget.isNull())
        m_widget = new ItemLibraryWidget{m_imageCache};

    return createWidgetInfo(m_widget.data(),
                            "Components",
                            WidgetInfo::LeftPane,
                            tr("Components"),
                            tr("Components view"));
}

void ItemLibraryView::modelAttached(Model *model)
{
    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->switchToComponentsView();
    m_widget->setModel(model);
    updateImports();
    if (model)
        m_widget->updatePossibleImports(set_difference(model->possibleImports(), model->imports()));
    m_hasErrors = !rewriterView()->errors().isEmpty();
    m_widget->setFlowMode(QmlItemNode(rootModelNode()).isFlowView());
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    AbstractView::modelAboutToBeDetached(model);

    m_widget->setModel(nullptr);
}

void ItemLibraryView::importsChanged(const Imports &, const Imports &)
{
    updateImports();
    m_widget->updatePossibleImports(model()->possibleImports());
}

void ItemLibraryView::possibleImportsChanged(const Imports &possibleImports)
{
    m_widget->updatePossibleImports(possibleImports);
}

void ItemLibraryView::usedImportsChanged(const Imports &usedImports)
{
    m_widget->updateUsedImports(usedImports);
}

void ItemLibraryView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    if (m_hasErrors && errors.isEmpty())
        updateImports();

     m_hasErrors = !errors.isEmpty();
}

void ItemLibraryView::updateImports()
{
    m_widget->delayedUpdateModel();
}

void ItemLibraryView::customNotification(const AbstractView *view,
                                         const QString &identifier,
                                         const QList<ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    if (identifier == UpdateItemlibrary)
        updateImports();
    else
        AbstractView::customNotification(view, identifier, nodeList, data);
}

} // namespace QmlDesigner
