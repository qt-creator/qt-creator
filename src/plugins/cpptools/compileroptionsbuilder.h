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
    virtual void addTargetTriple();
    virtual void enableExceptions();
    void addHeaderPathOptions();
    void addToolchainAndProjectDefines();
    virtual void addLanguageOption(ProjectFile::Kind fileKind);
    virtual void addOptionsForLanguage(bool checkForBorlandExtensions = true);

    void addMsvcCompatibilityVersion();
    void undefineCppLanguageFeatureMacrosForMsvc2015();

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
