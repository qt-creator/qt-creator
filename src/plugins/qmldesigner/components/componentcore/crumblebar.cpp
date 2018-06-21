/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "crumblebar.h"

#include "qmldesignerplugin.h"

#include <nodeabstractproperty.h>

#include <coreplugin/documentmanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/imode.h>
#include <coreplugin/editormanager/editormanager.h>

#include <QVariant>
#include <QtDebug>

namespace QmlDesigner {

static DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

static inline QString componentIdForModelNode(const ModelNode &modelNode)
{
    if (modelNode.id().isEmpty()) {
        if (modelNode.hasParentProperty()
                && modelNode.parentProperty().name() != "data"
                && modelNode.parentProperty().name() != "children") {
            return QString::fromUtf8(modelNode.parentProperty().name());
        } else {
            return modelNode.simplifiedTypeName();
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
}

CrumbleBar::~CrumbleBar()
{
    delete m_crumblePath;
}

void CrumbleBar::pushFile(const Utils::FileName &fileName)
{
    if (m_isInternalCalled == false) {
        crumblePath()->clear();
    } else {
        CrumbleBarInfo lastElementCrumbleBarInfo = crumblePath()->dataForLastIndex().value<CrumbleBarInfo>();

        if (!lastElementCrumbleBarInfo.displayName.isEmpty()
                && lastElementCrumbleBarInfo.fileName == fileName)
            crumblePath()->popElement();
    }

    CrumbleBarInfo crumbleBarInfo;
    crumbleBarInfo.fileName = fileName;

    crumblePath()->pushElement(fileName.fileName(), QVariant::fromValue(crumbleBarInfo));

    m_isInternalCalled = false;

    updateVisibility();
}

void CrumbleBar::pushInFileComponent(const ModelNode &modelNode)
{

    CrumbleBarInfo crumbleBarInfo = createCrumbleBarInfoFromModelNode(modelNode);
    CrumbleBarInfo lastElementCrumbleBarInfo = crumblePath()->dataForLastIndex().value<CrumbleBarInfo>();

    if (lastElementCrumbleBarInfo.modelNode.isValid())
        crumblePath()->popElement();

    crumblePath()->pushElement(crumbleBarInfo.displayName, QVariant::fromValue(crumbleBarInfo));

    m_isInternalCalled = false;

    updateVisibility();
}

void CrumbleBar::nextFileIsCalledInternally()
{
    m_isInternalCalled = true;
}

Utils::CrumblePath *CrumbleBar::crumblePath()
{
    if (m_crumblePath == nullptr) {
        m_crumblePath = new Utils::CrumblePath;
        updateVisibility();
        connect(m_crumblePath, &Utils::CrumblePath::elementClicked,
            this, &CrumbleBar::onCrumblePathElementClicked);
    }

    return m_crumblePath;
}

void CrumbleBar::showSaveDialog()
{
    if (DesignerSettings::getValue(DesignerSettingsKey::ALWAYS_SAFE_IN_CRUMBLEBAR).toBool()) {
        Core::DocumentManager::saveModifiedDocumentSilently(currentDesignDocument()->editor()->document());
    } else {
        bool alwaysSave;
        bool canceled;

        Core::DocumentManager::saveModifiedDocument(currentDesignDocument()->editor()->document(),
                                                    tr("Save the changes to preview them correctly."),
                                                    &canceled,
                                                    tr("Always save when leaving subcomponent"),
                                                    &alwaysSave);

        DesignerSettings::setValue(DesignerSettingsKey::ALWAYS_SAFE_IN_CRUMBLEBAR, alwaysSave);
    }
}

void CrumbleBar::onCrumblePathElementClicked(const QVariant &data)
{
    CrumbleBarInfo clickedCrumbleBarInfo = data.value<CrumbleBarInfo>();

    if (clickedCrumbleBarInfo ==  crumblePath()->dataForLastIndex().value<CrumbleBarInfo>())
        return;

    while (clickedCrumbleBarInfo != crumblePath()->dataForLastIndex().value<CrumbleBarInfo>())
        crumblePath()->popElement();

    if (crumblePath()->dataForLastIndex().value<CrumbleBarInfo>().modelNode.isValid())
        crumblePath()->popElement();

    m_isInternalCalled = true;
    if (!clickedCrumbleBarInfo.modelNode.isValid()
            && clickedCrumbleBarInfo.fileName == currentDesignDocument()->fileName()) {
        nextFileIsCalledInternally();
        currentDesignDocument()->changeToDocumentModel();
        QmlDesignerPlugin::instance()->viewManager().setComponentViewToMaster();
    } else {
        showSaveDialog();
        crumblePath()->popElement();
        nextFileIsCalledInternally();
        Core::EditorManager::openEditor(clickedCrumbleBarInfo.fileName.toString(),
            Core::Id(), Core::EditorManager::DoNotMakeVisible);
        if (clickedCrumbleBarInfo.modelNode.isValid()) {
            currentDesignDocument()->changeToSubComponent(clickedCrumbleBarInfo.modelNode);
            QmlDesignerPlugin::instance()->viewManager().setComponentNode(clickedCrumbleBarInfo.modelNode);
        } else {
            QmlDesignerPlugin::instance()->viewManager().setComponentViewToMaster();
        }
    }
    updateVisibility();
}

void CrumbleBar::updateVisibility()
{
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
