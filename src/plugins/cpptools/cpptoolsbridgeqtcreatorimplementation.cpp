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

#include "cpptoolsbridgeqtcreatorimplementation.h"

#include "baseeditordocumentparser.h"
#include "cppmodelmanager.h"
#include "projectpart.h"
#include "editordocumenthandle.h"

#include <coreplugin/editormanager/editormanager.h>

namespace CppTools {

namespace Internal {

CppEditorDocumentHandle *
CppToolsBridgeQtCreatorImplementation::cppEditorDocument(const QString &filePath) const
{
    return CppModelManager::instance()->cppEditorDocument(filePath);
}

namespace {

CppTools::ProjectPart::Ptr projectPartForFile(const QString &filePath)
{
    if (const auto parser = BaseEditorDocumentParser::get(filePath))
        return parser->projectPartInfo().projectPart;

    return CppTools::ProjectPart::Ptr();
}

bool isProjectPartValid(const CppTools::ProjectPart::Ptr projectPart)
{
    if (projectPart)
        return CppTools::CppModelManager::instance()->projectPartForId(projectPart->id());

    return false;
}

} // anonymous namespace

QString CppToolsBridgeQtCreatorImplementation::projectPartIdForFile(const QString &filePath) const
{
    const CppTools::ProjectPart::Ptr projectPart = projectPartForFile(filePath);

    if (isProjectPartValid(projectPart))
        return projectPart->id(); // OK, Project Part is still loaded

    return QString();
}

BaseEditorDocumentProcessor *
CppToolsBridgeQtCreatorImplementation::baseEditorDocumentProcessor(const QString &filePath) const
{
    auto *document = cppEditorDocument(filePath);
    if (document)
        return document->processor();

    return 0;
}

void CppToolsBridgeQtCreatorImplementation::finishedRefreshingSourceFiles(
        const QSet<QString> &filePaths) const
{
    CppModelManager::instance()->finishedRefreshingSourceFiles(filePaths);
}

QList<Core::IEditor *> CppToolsBridgeQtCreatorImplementation::visibleEditors() const
{
    return Core::EditorManager::visibleEditors();
}

} // namespace Internal

} // namespace CppTools
