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

#pragma once

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>
#include <cpptools/projectpart.h>
#include <utils/textfileformat.h>

#include <QStringList>

namespace Autotest {
namespace Internal {

class TestUtils
{
public:
    static QString getCMakeDisplayNameIfNecessary(const QString &filePath, const QString &proFile)
    {
        static const QString CMAKE_LISTS = QLatin1String("CMakeLists.txt");
        if (!proFile.endsWith(CMAKE_LISTS))
            return QString();

        const QList<CppTools::ProjectPart::Ptr> &projectParts
                = CppTools::CppModelManager::instance()->projectPart(filePath);
        if (projectParts.size())
            return projectParts.first()->displayName;

        return QString();
    }

    static QByteArray getFileContent(QString filePath)
    {
        QByteArray fileContent;
        CppTools::CppModelManager *cppMM = CppTools::CppModelManager::instance();
        CppTools::WorkingCopy wc = cppMM->workingCopy();
        if (wc.contains(filePath)) {
            fileContent = wc.source(filePath);
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
};

} // namespace Internal
} // namespace Autotest
