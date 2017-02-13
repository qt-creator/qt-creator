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

#include "itestparser.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanager.h>
#include <utils/textfileformat.h>

namespace Autotest {
namespace Internal {

static CppParser *s_parserInstance = nullptr;

CppParser::CppParser()
{
    s_parserInstance = this;
}

void CppParser::init(const QStringList &filesToParse)
{
    Q_UNUSED(filesToParse);
    m_cppSnapshot = CppTools::CppModelManager::instance()->snapshot();
    m_workingCopy = CppTools::CppModelManager::instance()->workingCopy();
}

bool CppParser::selectedForBuilding(const QString &fileName)
{
    QList<CppTools::ProjectPart::Ptr> projParts =
            CppTools::CppModelManager::instance()->projectPart(fileName);

    return projParts.size() && projParts.at(0)->selectedForBuilding;
}

QByteArray CppParser::getFileContent(const QString &filePath)
{
    QByteArray fileContent;
    if (s_parserInstance->m_workingCopy.contains(filePath)) {
        fileContent = s_parserInstance->m_workingCopy.source(filePath);
    } else {
        QString error;
        const QTextCodec *codec = Core::EditorManager::defaultTextCodec();
        if (Utils::TextFileFormat::readFileUTF8(filePath, codec, &fileContent, &error)
                != Utils::TextFileFormat::ReadSuccess) {
            qDebug() << "Failed to read file" << filePath << ":" << error;
        }
    }
    return fileContent;
}

void CppParser::release()
{
    m_cppSnapshot = CPlusPlus::Snapshot();
    m_workingCopy = CppTools::WorkingCopy();
}

} // namespace Internal
} // namespace Autotest
