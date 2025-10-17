// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "crumblebar.h"

#include "componentcoretracing.h"
#include "qmldesignerplugin.h"

#include <nodeabstractproperty.h>
#include <toolbar.h>

#include <qmldesignerbase/settings/designersettings.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QVariant>
#include <QtDebug>

namespace QmlDesigner {

using ComponentCoreTracing::category;
using NanotraceHR::keyValue;

static DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

static QString componentIdForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty()) {
        if (modelNode.hasParentProperty()
                && modelNode.parentProperty().name() != "data"
                && modelNode.parentProperty().name() != "children") {
            return QString::fromUtf8(modelNode.parentProperty().name());
        } else {
            return modelNode.exportedTypeName().name.toQString();
        }
    } else {
        return modelNode.id();
    }
}

static CrumbleBarInfo createCrumbleBarInfoFromModelNode(const ModelNode &modelNode)
{
    CrumbleBarInfo crumbleBarInfo;
    crumbleBarInfo.displayName = componentIdForModelNode(modelNode);
    crumbleBarInfo.fileName = currentDesignDocument()->textEditor()->document()->filePath();
    crumbleBarInfo.modelNode = modelNode;

    return crumbleBarInfo;
}

CrumbleBar::CrumbleBar(QObject *parent) : QObject(parent)
{
    NanotraceHR::Tracer tracer{"crumble bar constructor", category()};
}

CrumbleBar::~CrumbleBar()
{
    NanotraceHR::Tracer tracer{"crumble bar destructor", category()};

    delete m_crumblePath;
}

void CrumbleBar::pushFile(const Utils::FilePath &fileName)
{
    NanotraceHR::Tracer tracer{"crumble bar push file", category()};

    if (!m_isInternalCalled) {
        crumblePath()->clear();
        m_pathes.clear();
    } else {
        // If the path already exists in crumblePath, pop up to first instance of that to avoid
        // cyclical crumblePath
        int match = -1;
        for (int i = crumblePath()->length() - 1; i >= 0; --i) {
            CrumbleBarInfo info = crumblePath()->dataForIndex(i).value<CrumbleBarInfo>();
            if (info.fileName == fileName)
                match = i;
        }

        if (match != -1) {
            for (int i = crumblePath()->length() - 1 - match; i > 0; --i)
                popElement();
        }
    }

    CrumbleBarInfo info = crumblePath()->dataForLastIndex().value<CrumbleBarInfo>();
    if (info.fileName != fileName) {
        CrumbleBarInfo crumbleBarInfo;
        crumbleBarInfo.fileName = fileName;
        crumblePath()->pushElement(fileName.fileName(), QVariant::fromValue(crumbleBarInfo));
        m_pathes.append({fileName, fileName.fileName(), {}});
    }

    m_isInternalCalled = false;
    updateVisibility();

    emit pathChanged();
}

void CrumbleBar::pushInFileComponent(const ModelNode &modelNode)
{
    NanotraceHR::Tracer tracer{"crumble bar push in file component", category()};

    CrumbleBarInfo crumbleBarInfo = createCrumbleBarInfoFromModelNode(modelNode);
    CrumbleBarInfo lastElementCrumbleBarInfo = crumblePath()->dataForLastIndex().value<CrumbleBarInfo>();

    if (lastElementCrumbleBarInfo.modelNode.isValid())
        crumblePath()->popElement();

    crumblePath()->pushElement(crumbleBarInfo.displayName, QVariant::fromValue(crumbleBarInfo));
    m_pathes.append({{}, crumbleBarInfo.displayName, modelNode});

    m_isInternalCalled = false;

    updateVisibility();

    emit pathChanged();
}

void CrumbleBar::nextFileIsCalledInternally()
{
    NanotraceHR::Tracer tracer{"crumble bar next file is called internally", category()};

    m_isInternalCalled = true;
}

Utils::CrumblePath *CrumbleBar::crumblePath()
{
    NanotraceHR::Tracer tracer{"crumble bar crumble path", category()};

    if (m_crumblePath == nullptr) {
        m_crumblePath = new Utils::CrumblePath;
        updateVisibility();
        connect(m_crumblePath, &Utils::CrumblePath::elementClicked,
            this, &CrumbleBar::onCrumblePathElementClicked);
    }

    return m_crumblePath;
}

QStringList CrumbleBar::path() const
{
    NanotraceHR::Tracer tracer{"crumble bar path", category()};

    QStringList list;
    for (auto &path : m_pathes) {
        list.append(path.displayName);
    }

    return list;
}

QList<CrumbleBarInfo> CrumbleBar::infos() const
{
    NanotraceHR::Tracer tracer{"crumble bar infos", category()};

    return m_pathes;
}

bool CrumbleBar::showSaveDialog()
{
    NanotraceHR::Tracer tracer{"crumble bar show save dialog", category()};

    bool canceled = false;
    bool alwaysSave = QmlDesignerPlugin::settings().value(DesignerSettingsKey::ALWAYS_SAVE_IN_CRUMBLEBAR).toBool();
    if (alwaysSave) {
        Core::DocumentManager::saveModifiedDocumentSilently(currentDesignDocument()->editor()->document());
    } else {
        Core::DocumentManager::saveModifiedDocument(currentDesignDocument()->editor()->document(),
                                                    tr("Save the changes to preview them correctly."),
                                                    &canceled,
                                                    tr("Always save when leaving subcomponent"),
                                                    &alwaysSave);

        QmlDesignerPlugin::settings().insert(DesignerSettingsKey::ALWAYS_SAVE_IN_CRUMBLEBAR, alwaysSave);
    }
    return !canceled;
}

void CrumbleBar::popElement()
{
    NanotraceHR::Tracer tracer{"crumble bar pop element", category()};

    crumblePath()->popElement();

    if (!m_pathes.isEmpty())
        m_pathes.removeLast();
}

void CrumbleBar::onCrumblePathElementClicked(const QVariant &data)
{
    NanotraceHR::Tracer tracer{"crumble bar on crumble path element clicked", category()};

    CrumbleBarInfo clickedCrumbleBarInfo = data.value<CrumbleBarInfo>();

    if (clickedCrumbleBarInfo == crumblePath()->dataForLastIndex().value<CrumbleBarInfo>())
        return;

    bool inlineComp = !clickedCrumbleBarInfo.modelNode.isValid()
            && clickedCrumbleBarInfo.fileName == currentDesignDocument()->fileName();

    if (!inlineComp && !showSaveDialog())
        return;

    while (clickedCrumbleBarInfo != crumblePath()->dataForLastIndex().value<CrumbleBarInfo>()
           && crumblePath()->length() > 0)
        popElement();

    if (crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isValid())
        popElement();

    m_isInternalCalled = true;
    if (inlineComp) {
        nextFileIsCalledInternally();
        currentDesignDocument()->changeToDocumentModel();
        QmlDesignerPlugin::instance()->viewManager().setComponentViewToMaster();
    } else {
        popElement();
        nextFileIsCalledInternally();
        Core::EditorManager::openEditor(clickedCrumbleBarInfo.fileName,
                                        Utils::Id(),
                                        Core::EditorManager::DoNotMakeVisible);
        if (clickedCrumbleBarInfo.modelNode.isValid()) {
            currentDesignDocument()->changeToSubComponent(clickedCrumbleBarInfo.modelNode);
            QmlDesignerPlugin::instance()->viewManager().setComponentNode(clickedCrumbleBarInfo.modelNode);
        } else {
            QmlDesignerPlugin::instance()->viewManager().setComponentViewToMaster();
        }
    }
    emit pathChanged();
    updateVisibility();
}

void CrumbleBar::updateVisibility()
{
    NanotraceHR::Tracer tracer{"crumble bar update visibility", category()};

    if (!ToolBar::isVisible())
        crumblePath()->setVisible(crumblePath()->length() > 1);
}

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second)
{
    return first.fileName == second.fileName && first.modelNode == second.modelNode;
}

bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second)
{
    return first.fileName != second.fileName || first.modelNode != second.modelNode;
}

} // namespace QmlDesigner
