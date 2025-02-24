// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "effectcomposerview.h"

#include "effectcomposermodel.h"
#include "effectcomposernodesmodel.h"
#include "effectcomposertr.h"
#include "effectcomposerwidget.h"
#include "listmodelwidthcalculator.h"
#include "studioquickwidget.h"
#include "tableheaderlengthmodel.h"

#include <designermcumanager.h>
#include <documentmanager.h>
#include <import.h>
#include <modelnodeoperations.h>
#include <qmlchangeset.h>
#include <qmldesignerconstants.h>
#include <qmldesignerplugin.h>

#include <coreplugin/icore.h>

#include <QTimer>
#include <QtQml>

namespace EffectComposer {

EffectComposerView::EffectComposerView(QmlDesigner::ExternalDependenciesInterface &externalDependencies)
    : AbstractView{externalDependencies}
    , m_componentUtils(externalDependencies)
{
}

EffectComposerView::~EffectComposerView()
{}

bool EffectComposerView::hasWidget() const
{
    return true;
}

QmlDesigner::WidgetInfo EffectComposerView::widgetInfo()
{
    if (m_widget.isNull()) {
        m_widget = new EffectComposerWidget{this};

        connect(m_widget->effectComposerModel(), &EffectComposerModel::assignToSelectedTriggered, this,
                [this] (const QString &effectPath) {
            executeInTransaction("EffectComposerView assignToSelectedTriggered", [&] {
                const QList<QmlDesigner::ModelNode> selectedNodes = selectedModelNodes();
                for (const QmlDesigner::ModelNode &node : selectedNodes)
                    QmlDesigner::ModelNodeOperations::handleItemLibraryEffectDrop(effectPath, node);
            });
        });

        connect(m_widget->effectComposerModel(), &EffectComposerModel::removePropertiesFromScene, this,
                [this] (QSet<QByteArray> props, const QString &typeName) {
            // Remove specified properties from all instances of specified type

            QmlDesigner::DesignDocument *document
                = QmlDesigner::QmlDesignerPlugin::instance()->currentDesignDocument();
            if (!document)
                return;

#ifdef QDS_USE_PROJECTSTORAGE
            auto module = model()->module(QString("%1.%2").arg(m_componentUtils.composedEffectsTypePrefix(),
                                                               typeName).toUtf8(),
                                          QmlDesigner::Storage::ModuleKind::QmlLibrary);
            auto effectMetaInfo = model()->metaInfo(module, typeName.toUtf8());
#else
            const QByteArray fullType = QString("%1.%2.%2").arg(m_componentUtils.composedEffectsTypePrefix(),
                                                             typeName).toUtf8();
#endif
            const QList<QmlDesigner::ModelNode> allNodes = allModelNodes();
            QList<QmlDesigner::ModelNode> typeNodes;
            QList<QmlDesigner::ModelNode> propertyChangeNodes;
            for (const QmlDesigner::ModelNode &node : allNodes) {
                if (QmlDesigner::QmlPropertyChanges::isValidQmlPropertyChanges(node))
                    propertyChangeNodes.append(node);
#ifdef QDS_USE_PROJECTSTORAGE
                else if (node.metaInfo() == effectMetaInfo)
#else
                else if (node.metaInfo().typeName() == fullType)
#endif
                    typeNodes.append(node);
            }
            if (!typeNodes.isEmpty()) {
                bool clearStacks = false;

                executeInTransaction("EffectComposerView removePropertiesFromScene", [&] {
                    for (QmlDesigner::ModelNode node : std::as_const(propertyChangeNodes)) {
                        QmlDesigner::ModelNode targetNode = QmlDesigner::QmlPropertyChanges(node).target();
                        if (typeNodes.contains(targetNode)) {
                            for (const QByteArray &prop : props) {
                                if (node.hasProperty(prop)) {
                                    node.removeProperty(prop);
                                    clearStacks = true;
                                }
                            }
                            QList<QmlDesigner::AbstractProperty> remainingProps = node.properties();
                            if (remainingProps.size() == 1 && remainingProps[0].name() == "target")
                                node.destroy(); // Remove empty changes node
                        }
                    }
                    for (const QmlDesigner::ModelNode &node : std::as_const(typeNodes)) {
                        for (const QByteArray &prop : props) {
                            if (node.hasProperty(prop)) {
                                node.removeProperty(prop);
                                clearStacks = true;
                            }
                        }
                    }
                });

                // Reset undo stack as changing of the actual effect cannot be undone, and thus the
                // stack will contain only unworkable states
                if (clearStacks)
                    document->clearUndoRedoStacks();
            }
        });
    }

    return createWidgetInfo(m_widget.data(),
                            "EffectComposer",
                            QmlDesigner::WidgetInfo::LeftPane,
                            tr("Effect Composer"));
}

void EffectComposerView::customNotification([[maybe_unused]] const AbstractView *view,
                                         const QString &identifier,
                                         [[maybe_unused]] const QList<QmlDesigner::ModelNode> &nodeList,
                                         const QList<QVariant> &data)
{
    if (data.size() < 1)
        return;

    if (identifier == "open_effectcomposer_composition") {
        const QString compositionPath = data[0].toString();
        m_widget->openComposition(compositionPath);
    } else if (identifier == "effectcomposer_effects_deleted") {
        if (data[0].toStringList().contains(m_widget->effectComposerModel()->currentComposition()))
            m_widget->effectComposerModel()->clear(true);
    }
}

void EffectComposerView::modelAttached(QmlDesigner::Model *model)
{
    AbstractView::modelAttached(model);


    QString currProjectPath = QmlDesigner::DocumentManager::currentProjectDirPath().toUrlishString();

    if (m_currProjectPath != currProjectPath) { // starting a new project
        m_widget->effectComposerNodesModel()->loadModel();
        m_widget->effectComposerModel()->clear(true);
        m_widget->effectComposerModel()->setEffectsTypePrefix(m_componentUtils.composedEffectsTypePrefix());
        m_widget->effectComposerModel()->setIsEnabled(
            !QmlDesigner::DesignerMcuManager::instance().isMCUProject());
        m_widget->initView();
    }

    m_currProjectPath = currProjectPath;

}

void EffectComposerView::modelAboutToBeDetached(QmlDesigner::Model *model)
{
    AbstractView::modelAboutToBeDetached(model);
    if (m_widget)
        m_widget->effectComposerModel()->clear(true);
}

void EffectComposerView::selectedNodesChanged(const QList<QmlDesigner::ModelNode> & selectedNodeList,
                                              const QList<QmlDesigner::ModelNode> & /*lastSelectedNodeList*/)
{
    bool hasValidTarget = false;

    for (const QmlDesigner::ModelNode &node : selectedNodeList) {
        if (node.metaInfo().isQtQuickItem()) {
            hasValidTarget = true;
            break;
        }
    }

    m_widget->effectComposerModel()->setHasValidTarget(hasValidTarget);
}

void EffectComposerView::highlightSupportedProperties(bool highlight, const QString &suffix)
{
    QQmlContext *ctxObj = m_widget->quickWidget()->rootContext();
    ctxObj->setContextProperty("activeDragSuffix", suffix);
    ctxObj->setContextProperty("hasActiveDrag", highlight);
}

void EffectComposerView::dragStarted(QMimeData *mimeData)
{
    if (mimeData->hasFormat(QmlDesigner::Constants::MIME_TYPE_ASSETS)
        || mimeData->hasFormat(QmlDesigner::Constants::MIME_TYPE_BUNDLE_TEXTURE)) {
        QString format = mimeData->formats()[0];
        const QString assetPath = QString::fromUtf8(mimeData->data(format)).split(',')[0];
        const QString suffix = "*." + assetPath.split('.').last().toLower();

        highlightSupportedProperties(true, suffix);
    }
}

void EffectComposerView::dragEnded()
{
    highlightSupportedProperties(false);
}

void EffectComposer::EffectComposerView::registerDeclarativeTypes()
{
    qmlRegisterType<TableHeaderLengthModel>("TableModules", 1, 0, "TableHeaderLengthModel");
    qmlRegisterType<ListModelWidthCalculator>("ModelModules", 1, 0, "ListModelWidthCalculator");
}

} // namespace EffectComposer
