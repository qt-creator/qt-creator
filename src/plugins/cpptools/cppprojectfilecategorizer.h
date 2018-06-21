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

#include "cppprojectfile.h"
#include "cpprawprojectpart.h"

#include <QString>
#include <QVector>

namespace CppTools {

class ProjectFileCategorizer
{
public:
    using FileClassifier = RawProjectPart::FileClassifier;

public:
    ProjectFileCategorizer(const QString &projectPartName,
                           const QStringList &filePaths,
                           FileClassifier fileClassifier = FileClassifier());

    bool hasCSources() const { return !m_cSources.isEmpty(); }
    bool hasCxxSources() const { return !m_cxxSources.isEmpty(); }
    bool hasObjcSources() const { return !m_objcSources.isEmpty(); }
    bool hasObjcxxSources() const { return !m_objcxxSources.isEmpty(); }

    ProjectFiles cSources() const { return m_cSources; }
    ProjectFiles cxxSources() const { return m_cxxSources; }
    ProjectFiles objcSources() const { return m_objcSources; }
    ProjectFiles objcxxSources() const { return m_objcxxSources; }

    bool hasMultipleParts() const { return m_partCount > 1; }
    bool hasParts() const { return m_partCount > 0; }

    QString partName(const QString &languageName) const;

private:
    QStringList classifyFiles(const QStringList &filePaths, FileClassifier fileClassifier);
    void expandSourcesWithAmbiguousHeaders(const QStringList &ambiguousHeaders);

private:
    QString m_partName;
    ProjectFiles m_cSources;
    ProjectFiles m_cxxSources;
    ProjectFiles m_objcSources;
    ProjectFiles m_objcxxSources;
    int m_partCount;
};

} // namespace CppTools
