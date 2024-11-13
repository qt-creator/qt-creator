// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditorwidget.h"

#include "nodegrapheditorimageprovider.h"
#include "nodegrapheditormodel.h"
#include "nodegrapheditorview.h"

#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>
#include <theme.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/messagebox.h>

#include <QFileInfo>
#include <QKeySequence>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlPropertyMap>
#include <QQuickItem>
#include <QShortcut>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

NodeGraphEditorWidget::NodeGraphEditorWidget(NodeGraphEditorView *nodeGraphEditorView,
                                               NodeGraphEditorModel *nodeGraphEditorModel)
    : m_editorView(nodeGraphEditorView)
    , m_imageProvider(nullptr)
    , m_qmlSourceUpdateShortcut(nullptr)
{
    m_imageProvider = new Internal::NodeGraphEditorImageProvider;

    engine()->addImageProvider(QStringLiteral("qmldesigner_nodegrapheditor"), m_imageProvider);
    engine()->addImportPath(propertyEditorResourcesPath() + "/imports");
    engine()->addImportPath(qmlSourcesPath());
    engine()->addImportPath(qmlSourcesPath() + "/imports");

    m_qmlSourceUpdateShortcut = new QShortcut(QKeySequence(Qt::ALT | Qt::Key_F9), this);
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &NodeGraphEditorWidget::reloadQmlSource);

    quickWidget()->setObjectName(Constants::OBJECT_NAME_STATES_EDITOR);
    setResizeMode(QQuickWidget::SizeRootObjectToView);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    auto map = registerPropertyMap("NodeGraphEditorBackend");
    map->setProperties({{"nodeGraphEditorModel", QVariant::fromValue(nodeGraphEditorModel)}});

    Theme::setupTheme(engine());

    setWindowTitle(tr("Node Graph", "Title of Editor widget"));
    setMinimumSize(QSize(256, 256));

    // init the first load of the QML UI elements
    reloadQmlSource();
}

QString NodeGraphEditorWidget::qmlSourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/nodegrapheditor";
#endif
    return Core::ICore::resourcePath("qmldesigner/nodegrapheditor").toString();
}

void NodeGraphEditorWidget::showEvent(QShowEvent *event)
{
    StudioQuickWidget::showEvent(event);
    update();
    QMetaObject::invokeMethod(rootObject(), "showEvent");
}

void NodeGraphEditorWidget::focusOutEvent(QFocusEvent *focusEvent)
{
    QmlDesignerPlugin::emitUsageStatisticsTime(Constants::EVENT_NODEGRAPHEDITOR_TIME,
                                               m_usageTimer.elapsed());
    StudioQuickWidget::focusOutEvent(focusEvent);
}

void NodeGraphEditorWidget::focusInEvent(QFocusEvent *focusEvent)
{
    m_usageTimer.restart();
    StudioQuickWidget::focusInEvent(focusEvent);
}

void NodeGraphEditorWidget::reloadQmlSource()
{
    QString qmlFilePath = qmlSourcesPath() + QStringLiteral("/Main.qml");
    QTC_ASSERT(QFileInfo::exists(qmlFilePath), return );
    setSource(QUrl::fromLocalFile(qmlFilePath));

    if (!rootObject()) {
        QString errorString;
        for (const QQmlError &error : errors())
            errorString += "\n" + error.toString();

        Core::AsynchronousMessageBox::warning(tr("Cannot Create QtQuick View"),
                                              tr("NodeGraphEditorWidget: %1 cannot be created.%2")
                                                  .arg(qmlSourcesPath(), errorString));
        return;
    }
}

} // namespace QmlDesigner
