/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsbridgeqtcreatorimplementation.h"

#include "baseeditordocumentparser.h"
#include "cppmodelmanager.h"
#include "cppprojects.h"
#include "editordocumenthandle.h"

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
        return parser->projectPart();

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

} // namespace Internal

} // namespace CppTools
