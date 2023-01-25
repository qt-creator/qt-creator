// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "toolbar.h"
#include "toolbarbackend.h"

#include <theme.h>

#include <coreplugin/icore.h>
#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QStatusBar>
#include <QToolBar>
#include <QMainWindow>
#include <QQmlEngine>

namespace QmlDesigner {

QmlDesigner::ToolBar::ToolBar()
{

}

static Utils::FilePath propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return Utils::FilePath::fromString(QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources");
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources");
}

Utils::FilePath qmlSourcesStatusBarPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return Utils::FilePath::fromString(QLatin1String(SHARE_QML_PATH) + "/statusbar");
#endif
    return Core::ICore::resourcePath("qmldesigner/statusbar");
}

Utils::FilePath qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return Utils::FilePath::fromString(QLatin1String(SHARE_QML_PATH) + "/toolbar");
#endif
    return Core::ICore::resourcePath("qmldesigner/toolbar");
}

void ToolBar::create()
{
    if (!isVisible())
        return;

    ToolBarBackend::registerDeclarativeType();

    auto window = Core::ICore::mainWindow();

    //Core::ICore::statusBar()->hide();

    auto toolBar = new QToolBar;

    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

    toolBar->setFloatable(false);
    toolBar->setMovable(false);

    auto quickWidget = new QQuickWidget;

    quickWidget->setFixedHeight(48);
    quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    quickWidget->setMinimumWidth(200);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quickWidget->engine()->addImportPath(propertyEditorResourcesPath().toString() + "/imports");

    Utils::FilePath qmlFilePath = qmlSourcesPath() / "Main.qml";
    QTC_ASSERT(qmlFilePath.exists(), return );

    Theme::setupTheme(quickWidget->engine());

    quickWidget->setSource(QUrl::fromLocalFile(qmlFilePath.toFSPathString()));

    toolBar->addWidget(quickWidget);
    window->addToolBar(toolBar);
}

void ToolBar::createStatusBar()
{
    if (!isVisible())
        return;

    ToolBarBackend::registerDeclarativeType();

    auto quickWidget = new QQuickWidget;

    quickWidget->setFixedHeight(24);
    quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    quickWidget->setMinimumWidth(200);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quickWidget->engine()->addImportPath(propertyEditorResourcesPath().toString() + "/imports");

    Utils::FilePath qmlFilePath = qmlSourcesStatusBarPath() + QStringLiteral("/Main.qml");
    QTC_ASSERT(qmlFilePath.exists(), return );

    Theme::setupTheme(quickWidget->engine());

    quickWidget->setSource(QUrl::fromLocalFile(qmlFilePath.toFSPathString()));

    for (QWidget *w : Core::ICore::statusBar()->findChildren<QWidget *>(Qt::FindDirectChildrenOnly)) {
        w->hide();
    }

    Core::ICore::statusBar()->addWidget(quickWidget);
}

bool ToolBar::isVisible()
{
    QSettings *settings = Core::ICore::settings();
    const QString qdsToolbarEntry = "QML/Designer/TopToolBar";

    return settings->value(qdsToolbarEntry, false).toBool();
}

} // namespace QmlDesigner
