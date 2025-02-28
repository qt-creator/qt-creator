// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "designsystemview.h"
#include "coreplugin/messagebox.h"
#include "designsystemwidget.h"
#include "dsstore.h"

#include <qmldesignertr.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <QPushButton>
#include <QQuickWidget>
#include <QtQml/qqmlcontext.h>
#include <QtQml/qqmlengine.h>

namespace QmlDesigner {

DesignSystemView::DesignSystemView(ExternalDependenciesInterface &externalDependencies,
                                   ProjectStorageDependencies projectStorageDependencies)
    : AbstractView(externalDependencies)
    , m_externalDependencies(externalDependencies)
    , m_dsStore(std::make_unique<DSStore>(m_externalDependencies, projectStorageDependencies))
    , m_dsInterface(m_dsStore.get())
{
    connect(ProjectExplorer::ProjectManager::instance(),
            &ProjectExplorer::ProjectManager::startupProjectChanged,
            this,
            [this](ProjectExplorer::Project *) { loadDesignSystem(); });

    connect(Core::EditorManager::instance(),
            &Core::EditorManager::saved,
            this,
            [this](Core::IDocument *document) {
                if (document->filePath().contains("Generated/DesignSystem")) {
                    loadDesignSystem();
                }
            });
}

DesignSystemView::~DesignSystemView() {}

WidgetInfo DesignSystemView::widgetInfo()
{
    if (!m_designSystemWidget)
        m_designSystemWidget = new DesignSystemWidget(this, &m_dsInterface);

    return createWidgetInfo(m_designSystemWidget,
                            "DesignSystemView",
                            WidgetInfo::RightPane,
                            Tr::tr("Design System"),
                            Tr::tr("Design System view"),
                            DesignerWidgetFlags::IgnoreErrors);
}

bool DesignSystemView::hasWidget() const
{
    return true;
}

void DesignSystemView::loadDesignSystem()
{
    /*This is only used to load internally - when saving we have to take care of reflection.
     * Saving should not trigger a load again.
    */
    m_dsInterface.loadDesignSystem();
}

} // namespace QmlDesigner
