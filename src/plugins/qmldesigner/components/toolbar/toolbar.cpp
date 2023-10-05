// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "toolbar.h"
#include "toolbarbackend.h"

#include <studioquickwidget.h>

#include <theme.h>
#include <qmldesignerconstants.h>

#include <coreplugin/icore.h>

#include <utils/filepath.h>
#include <utils/qtcassert.h>

#include <QMainWindow>
#include <QQmlEngine>
#include <QStatusBar>
#include <QToolBar>

using namespace Utils;

namespace QmlDesigner {

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

Utils::UniqueObjectPtr<QToolBar> ToolBar::create()
{
    if (!isVisible())
        return nullptr;

    ToolBarBackend::registerDeclarativeType();

    auto window = Core::ICore::mainWindow();

    //Core::ICore::statusBar()->hide();

    auto toolBar = Utils::makeUniqueObjectPtr<QToolBar>();
    toolBar->setObjectName("QDS-TOOLBAR");

    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);

    toolBar->setFloatable(false);
    toolBar->setMovable(false);
    toolBar->setProperty("_q_custom_style_skipolish", true);
    toolBar->setContentsMargins(0, 0, 0, 0);

    auto quickWidget = std::make_unique<StudioQuickWidget>();

    quickWidget->setFixedHeight(48);
    quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    quickWidget->setMinimumWidth(200);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_TOP_TOOLBAR);

    quickWidget->engine()->addImportPath(propertyEditorResourcesPath().toString() + "/imports");

    Utils::FilePath qmlFilePath = qmlSourcesPath() / "Main.qml";
    QTC_ASSERT(qmlFilePath.exists(), return nullptr);

    Theme::setupTheme(quickWidget->engine());

    quickWidget->setSource(QUrl::fromLocalFile(qmlFilePath.toFSPathString()));

    toolBar->addWidget(quickWidget.release());
    window->addToolBar(toolBar.get());

    return toolBar;
}

Utils::UniqueObjectPtr<QWidget> ToolBar::createStatusBar()
{
    if (!isVisible())
        return nullptr;

    ToolBarBackend::registerDeclarativeType();

    auto quickWidget = Utils::makeUniqueObjectPtr<StudioQuickWidget>();

    quickWidget->setFixedHeight(Theme::toolbarSize());
    quickWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    quickWidget->setMinimumWidth(200);
    quickWidget->setResizeMode(QQuickWidget::SizeRootObjectToView);

    quickWidget->quickWidget()->setObjectName(Constants::OBJECT_NAME_STATUSBAR);

    quickWidget->engine()->addImportPath(propertyEditorResourcesPath().toString() + "/imports");

    Utils::FilePath qmlFilePath = qmlSourcesStatusBarPath().pathAppended("/Main.qml");
    QTC_ASSERT(qmlFilePath.exists(), return nullptr);

    Theme::setupTheme(quickWidget->engine());

    quickWidget->setSource(QUrl::fromLocalFile(qmlFilePath.toFSPathString()));

    for (QWidget *w : Core::ICore::statusBar()->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        w->hide();
    }

    Core::ICore::statusBar()->addPermanentWidget(quickWidget.get(), 100);
    Core::ICore::statusBar()->setFixedHeight(Theme::toolbarSize());

    return quickWidget;
}

bool ToolBar::isVisible()
{
    QtcSettings *settings = Core::ICore::settings();
    const Key qdsToolbarEntry = "QML/Designer/TopToolBar";

    return settings->value(qdsToolbarEntry, false).toBool();
}

} // namespace QmlDesigner
