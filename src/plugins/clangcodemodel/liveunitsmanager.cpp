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

#include "liveunitsmanager.h"

#include <coreplugin/idocument.h>

using namespace ClangCodeModel;
using namespace Internal;

LiveUnitsManager *LiveUnitsManager::m_instance = 0;

LiveUnitsManager::LiveUnitsManager()
{
    Q_ASSERT(!m_instance);
    m_instance = this;

    qRegisterMetaType<ClangCodeModel::Internal::Unit>();
}

LiveUnitsManager::~LiveUnitsManager()
{
    m_instance = 0;
}

void LiveUnitsManager::requestTracking(const QString &fileName)
{
    if (!fileName.isEmpty() && !isTracking(fileName))
        m_units.insert(fileName, Unit(fileName));
}

void LiveUnitsManager::cancelTrackingRequest(const QString &fileName)
{
    if (!isTracking(fileName))
        return;

    // If no one else is tracking this particular unit, we remove it.
    if (m_units[fileName].isUnique())
        m_units.remove(fileName);
}

void LiveUnitsManager::updateUnit(const QString &fileName, const Unit &unit)
{
    if (!isTracking(fileName))
        return;

    m_units[fileName] = unit;

    emit unitAvailable(unit);
}

Unit LiveUnitsManager::unit(const QString &fileName)
{
    return m_units.value(fileName);
}

void LiveUnitsManager::editorOpened(Core::IEditor *editor)
{
    requestTracking(editor->document()->filePath());
}

void LiveUnitsManager::editorAboutToClose(Core::IEditor *editor)
{
    cancelTrackingRequest(editor->document()->filePath());
}
