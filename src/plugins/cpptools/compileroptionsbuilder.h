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

#ifndef CPPTOOLS_COMPILEROPTIONSBUILDER_H
#define CPPTOOLS_COMPILEROPTIONSBUILDER_H

#include "cpptools_global.h"

#include "projectpart.h"

namespace CppTools {

class CPPTOOLS_EXPORT CompilerOptionsBuilder
{
public:
    CompilerOptionsBuilder(const ProjectPart &projectPart);
    virtual ~CompilerOptionsBuilder() {}

    QStringList options() const;

    // Add custom options
    void add(const QString &option);
    void addDefine(const QByteArray &defineLine);

    // Add options based on project part
    void addHeaderPathOptions();
    void addToolchainAndProjectDefines();
    virtual void addLanguageOption(ProjectFile::Kind fileKind);
    virtual void addOptionsForLanguage(bool checkForBorlandExtensions = true);

protected:
    virtual bool excludeDefineLine(const QByteArray &defineLine) const;
    virtual bool excludeHeaderPath(const QString &headerPath) const;

    virtual QString defineOption() const;
    virtual QString includeOption() const;

    const ProjectPart m_projectPart;

private:
    QString defineLineToDefineOption(const QByteArray &defineLine);

    QStringList m_options;
};

} // namespace CppTools

#endif // CPPTOOLS_COMPILEROPTIONSBUILDER_H
