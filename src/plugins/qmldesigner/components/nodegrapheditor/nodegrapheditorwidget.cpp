// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "nodegrapheditorwidget.h"

#include "nodegrapheditorimageprovider.h"
#include "nodegrapheditormodel.h"
#include "nodegrapheditorview.h"

#include <nodemetainfo.h>
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

#include <QuickQanava>

namespace QmlDesigner {

static QString propertyEditorResourcesPath()
{
#ifdef SHARE_QML_PATH
    if (qEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/propertyEditorQmlSources";
#endif
    return Core::ICore::resourcePath("qmldesigner/propertyEditorQmlSources").toString();
}

static Utils::FilePath materialsPath()
{
    return DocumentManager::currentResourcePath().pathAppended("materials");
}

void NodeGraphEditorWidget::doOpenNodeGraph(){
    m_model->openFile(m_filePath);
}

void NodeGraphEditorWidget::openNodeGraph(const QString &path)
{
   m_filePath = path;
    if (m_model->hasUnsavedChanges()){
        /*Pass new path to update if saved*/
        auto newFile  = QFileInfo(path).baseName();
        QMetaObject::invokeMethod(quickWidget()->rootObject(), "promptToSaveBeforeOpen",Q_ARG(QVariant, newFile));
    }
    else{
        doOpenNodeGraph();
    }
}


NodeGraphEditorWidget::NodeGraphEditorWidget(NodeGraphEditorView *nodeGraphEditorView,
                                               NodeGraphEditorModel *nodeGraphEditorModel)
    : m_editorView(nodeGraphEditorView)
    , m_model(nodeGraphEditorModel)
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
    map->setProperties({{"widget", QVariant::fromValue(this)}});
    connect(m_qmlSourceUpdateShortcut, &QShortcut::activated, this, &NodeGraphEditorWidget::reloadQmlSource);

    Theme::setupTheme(engine());

    setWindowTitle(tr("Node Graph", "Title of Editor widget"));
    setMinimumSize(QSize(256, 256));

    QuickQanava::initialize(engine());

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

static QList<QString> allowedTypes {
    "bool",
    "float",
    "double",
    "QColor",
    "QUrl",
    "Texture",
};

static QMap<QString, QString> translateTypes {
    { "float", "real" },
    { "double", "real" },
};

static QList<QString> omitNames {
};

QList<QVariantMap> NodeGraphEditorWidget::createMetaData_inPorts(QByteArray typeName)
{
    QList<QVariantMap> result;

    const auto mi = m_editorView->model()->metaInfo(typeName);
    if (mi.isValid()) {
        for (const auto& property : mi.properties()) {
            QString type = QString::fromUtf8(property.propertyType().simplifiedTypeName());
            const QString name = QString::fromUtf8(property.name());

            if (!allowedTypes.contains(type))
                continue;
            if (omitNames.contains(name))
                continue;

            if (translateTypes.contains(type))
                type = translateTypes[type];

            result.append({
                           { "id", name },
                           { "displayName", name },
                           { "type", type }
            });
        }
    }

    return result;
}

QString NodeGraphEditorWidget::generateUUID() const
{
    return QUuid::createUuid().toString();
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
