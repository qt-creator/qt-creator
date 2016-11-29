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

#include "cppprojectfilecategorizer.h"

namespace CppTools {

ProjectFileCategorizer::ProjectFileCategorizer(const QString &projectPartName,
                                               const QStringList &filePaths,
                                               ProjectPartBuilder::FileClassifier fileClassifier)
    : m_partName(projectPartName)
{
    ProjectFiles cHeaders;
    ProjectFiles cxxHeaders;

    foreach (const QString &filePath, filePaths) {
        const ProjectFile::Kind kind = fileClassifier
                ? fileClassifier(filePath)
                : ProjectFile::classify(filePath);
        const ProjectFile projectFile(filePath, kind);

        switch (kind) {
        case ProjectFile::CSource: m_cSources += projectFile; break;
        case ProjectFile::CHeader: cHeaders += projectFile; break;
        case ProjectFile::CXXSource: m_cxxSources += projectFile; break;
        case ProjectFile::CXXHeader: cxxHeaders += projectFile; break;
        case ProjectFile::ObjCSource: m_objcSources += projectFile; break;
        case ProjectFile::ObjCXXSource: m_objcxxSources += projectFile; break;
        default:
            continue;
        }
    }

    const bool hasC = !m_cSources.isEmpty();
    const bool hasCxx = !m_cxxSources.isEmpty();
    const bool hasObjc = !m_objcSources.isEmpty();
    const bool hasObjcxx = !m_objcxxSources.isEmpty();

    if (hasObjcxx)
        m_objcxxSources += cxxHeaders + cHeaders;
    if (hasCxx)
        m_cxxSources += cxxHeaders + cHeaders;
    else if (!hasObjcxx)
        m_cxxSources += cxxHeaders;
    if (hasObjc)
        m_objcSources += cHeaders;
    if (hasC || (!hasObjc && !hasObjcxx && !hasCxx))
        m_cSources += cHeaders;

    m_partCount = (m_cSources.isEmpty() ? 0 : 1)
                + (m_cxxSources.isEmpty() ? 0 : 1)
                + (m_objcSources.isEmpty() ? 0 : 1)
                + (m_objcxxSources.isEmpty() ? 0 : 1);
}

QString ProjectFileCategorizer::partName(const QString &languageName) const
{
    if (hasMultipleParts())
        return QString::fromLatin1("%1 (%2)").arg(m_partName).arg(languageName);

    return m_partName;
}

} // namespace CppTools
