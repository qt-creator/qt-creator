/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "crumblebar.h"

#include "qmldesignerplugin.h"

#include <nodeabstractproperty.h>

#include <coreplugin/documentmanager.h>

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
            return modelNode.parentProperty().name();
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

CrumbleBar::CrumbleBar(QObject *parent) :
    QObject(parent),
    m_isInternalCalled(false),
    m_crumblePath(new Utils::CrumblePath)
{
    connect(m_crumblePath,
            SIGNAL(elementClicked(QVariant)),
            this,
            SLOT(onCrumblePathElementClicked(QVariant)));

    updateVisibility();
}

void CrumbleBar::pushFile(const QString &fileName)
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

    crumblePath()->pushElement(fileName.split("/").last(), QVariant::fromValue(crumbleBarInfo));

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
    return m_crumblePath;
}

void CrumbleBar::showSaveDialog()
{
    DesignerSettings settings = QmlDesignerPlugin::instance()->settings();

    if (settings.alwaysSaveInCrumbleBar) {
        Core::DocumentManager::saveModifiedDocumentSilently(currentDesignDocument()->editor()->document());
    } else {
        bool alwaysSave;
        bool canceled;

        Core::DocumentManager::saveModifiedDocument(currentDesignDocument()->editor()->document(),
                                                    tr("Save the changes to preview them correctly."),
                                                    &canceled,
                                                    tr("Always save when leaving subcomponent"),
                                                    &alwaysSave);

        settings.alwaysSaveInCrumbleBar = alwaysSave;
        QmlDesignerPlugin::instance()->setSettings(settings);
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
        Core::EditorManager::openEditor(clickedCrumbleBarInfo.fileName, Core::Id(),
                                        Core::EditorManager::DoNotMakeVisible);
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
