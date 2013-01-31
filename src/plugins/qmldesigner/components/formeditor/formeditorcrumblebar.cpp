/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include "formeditorcrumblebar.h"

#include "qmldesignerplugin.h"

#include <QVariant>
#include <QtDebug>

namespace QmlDesigner {

FormEditorCrumbleBar::FormEditorCrumbleBar(QObject *parent) :
    QObject(parent),
    m_isInternalCalled(false),
    m_crumblePath(new Utils::CrumblePath)
{
    connect(m_crumblePath,
            SIGNAL(elementClicked(QVariant)),
            this,
            SLOT(onCrumblePathElementClicked(QVariant)));
}

void FormEditorCrumbleBar::pushFile(const QString &fileName)
{
    if (m_isInternalCalled == false)
        crumblePath()->clear();

    CrumbleBarInfo crumbleBarInfo;
    crumbleBarInfo.fileName = fileName;

    crumblePath()->pushElement(fileName.split("/").last(), QVariant::fromValue(crumbleBarInfo));

    m_isInternalCalled = false;
}

static DesignDocument *currentDesignDocument()
{
    return QmlDesignerPlugin::instance()->documentManager().currentDesignDocument();
}

void FormEditorCrumbleBar::pushInFileComponent(const QString &componentId)
{
    CrumbleBarInfo crumbleBarInfo;
    crumbleBarInfo.componentId = componentId;
    crumbleBarInfo.fileName = currentDesignDocument()->textEditor()->document()->fileName();

    CrumbleBarInfo lastElementCrumbleBarInfo = crumblePath()->dataForLastIndex().value<CrumbleBarInfo>();

    if (!lastElementCrumbleBarInfo.componentId.isEmpty())
        crumblePath()->popElement();

    crumblePath()->pushElement(componentId, QVariant::fromValue(crumbleBarInfo));
}

void FormEditorCrumbleBar::nextFileIsCalledInternally()
{
    m_isInternalCalled = true;
}

Utils::CrumblePath *FormEditorCrumbleBar::crumblePath()
{
    return m_crumblePath;
}

void FormEditorCrumbleBar::onCrumblePathElementClicked(const QVariant &data)
{
    CrumbleBarInfo crumbleBarInfo = data.value<CrumbleBarInfo>();

    if (crumbleBarInfo == crumblePath()->dataForLastIndex().value<CrumbleBarInfo>())
        return;

    while (crumbleBarInfo != crumblePath()->dataForLastIndex().value<CrumbleBarInfo>())
        crumblePath()->popElement();

    if (!crumbleBarInfo.componentId.isEmpty())
        crumblePath()->popElement();

    crumblePath()->popElement();

    m_isInternalCalled = true;
    Core::EditorManager::openEditor(crumbleBarInfo.fileName);
    if (!crumbleBarInfo.componentId.isEmpty())
        currentDesignDocument()->changeToSubComponent(currentDesignDocument()->rewriterView()->modelNodeForId(crumbleBarInfo.componentId));
}

bool operator ==(const CrumbleBarInfo &first, const CrumbleBarInfo &second)
{
    return first.fileName == second.fileName && first.componentId == second.componentId;
}

bool operator !=(const CrumbleBarInfo &first, const CrumbleBarInfo &second)
{
    return first.fileName != second.fileName || first.componentId != second.componentId;
}

} // namespace QmlDesigner
