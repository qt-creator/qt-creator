// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "itemlibraryview.h"
#include "itemlibrarytracing.h"
#include "itemlibrarywidget.h"

#include <asynchronousimagecache.h>
#include <bindingproperty.h>
#include <componentcore_constants.h>
#include <coreplugin/icore.h>
#include <customnotifications.h>
#include <import.h>
#include <metainfo.h>
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

using ItemLibraryTracing::category;

ItemLibraryView::ItemLibraryView(AsynchronousImageCache &imageCache,
                                 ExternalDependenciesInterface &externalDependencies)
    : AbstractView(externalDependencies)
    , m_imageCache(imageCache)
{
    NanotraceHR::Tracer tracer{"item library view constructor", category()};
}

ItemLibraryView::~ItemLibraryView()
{
    NanotraceHR::Tracer tracer{"item library view destructor", category()};
}

bool ItemLibraryView::hasWidget() const
{
    NanotraceHR::Tracer tracer{"item library view has widget", category()};

    return true;
}

WidgetInfo ItemLibraryView::widgetInfo()
{
    NanotraceHR::Tracer tracer{"item library view widget info", category()};

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
    NanotraceHR::Tracer tracer{"item library view model attached", category()};

    AbstractView::modelAttached(model);

    m_widget->clearSearchFilter();
    m_widget->switchToComponentsView();
    m_widget->setModel(model);
    updateImports();
    if (model)
        m_widget->updatePossibleImports(set_difference(model->possibleImports(), model->imports()));
    m_hasErrors = !rewriterView()->errors().isEmpty();
}

void ItemLibraryView::modelAboutToBeDetached(Model *model)
{
    NanotraceHR::Tracer tracer{"item library view model about to be detached", category()};

    AbstractView::modelAboutToBeDetached(model);

    m_widget->setModel(nullptr);
}

void ItemLibraryView::importsChanged(const Imports &, const Imports &)
{
    NanotraceHR::Tracer tracer{"item library view imports changed", category()};

    updateImports();
    m_widget->updatePossibleImports(model()->possibleImports());
}

void ItemLibraryView::possibleImportsChanged(const Imports &possibleImports)
{
    NanotraceHR::Tracer tracer{"item library view possible imports changed", category()};

    m_widget->updatePossibleImports(possibleImports);
}

void ItemLibraryView::usedImportsChanged(const Imports &usedImports)
{
    NanotraceHR::Tracer tracer{"item library view used imports changed", category()};

    m_widget->updateUsedImports(usedImports);
}

void ItemLibraryView::documentMessagesChanged(const QList<DocumentMessage> &errors, const QList<DocumentMessage> &)
{
    NanotraceHR::Tracer tracer{"item library view document messages changed", category()};

    if (m_hasErrors && errors.isEmpty())
        updateImports();

     m_hasErrors = !errors.isEmpty();
}

void ItemLibraryView::updateImports()
{
    NanotraceHR::Tracer tracer{"item library view update imports", category()};

    m_widget->delayedUpdateModel();
}

void ItemLibraryView::customNotification(const AbstractView *view,
                                         const QString &identifier,
                                         const QList<ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    NanotraceHR::Tracer tracer{"item library view custom notification", category()};

    if (identifier == UpdateItemlibrary)
        updateImports();
    else
        AbstractView::customNotification(view, identifier, nodeList, data);
}

void ItemLibraryView::exportedTypeNamesChanged(const ExportedTypeNames &, const ExportedTypeNames &)
{
    m_widget->updatePossibleImports(model()->possibleImports());
}

} // namespace QmlDesigner
