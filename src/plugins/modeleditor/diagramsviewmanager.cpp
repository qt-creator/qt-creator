// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "diagramsviewmanager.h"

#include "modeleditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <utils/qtcassert.h>

#include <QDebug>

namespace ModelEditor {
namespace Internal {

DiagramsViewManager::DiagramsViewManager(QObject *parent)
    : QObject(parent)
{
}

void DiagramsViewManager::openDiagram(qmt::MDiagram *diagram)
{
    emit openNewDiagram(diagram);
}

void DiagramsViewManager::closeDiagram(const qmt::MDiagram *diagram)
{
    emit closeOpenDiagram(diagram);
}

void DiagramsViewManager::closeAllDiagrams()
{
    emit closeAllOpenDiagrams();
}

void DiagramsViewManager::onDiagramRenamed(const qmt::MDiagram *diagram)
{
    emit diagramRenamed(diagram);
}

} // namespace Internal
} // namespace ModelEditor


