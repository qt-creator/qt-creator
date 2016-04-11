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

#include "projectpart.h"

#include <QDir>
#include <QTextStream>

namespace CppTools {

ProjectPart::ProjectPart()
    : project(0)
    , isMsvc2015Toolchain(false)
    , languageVersion(CXX14)
    , languageExtensions(NoExtensions)
    , qtVersion(UnknownQt)
    , warningFlags(ProjectExplorer::WarningFlags::Default)
    , selectedForBuilding(true)
{
}

void ProjectPart::updateLanguageFeatures()
{
    const bool hasQt = qtVersion != NoQt;
    languageFeatures.cxx11Enabled = languageVersion >= CXX11;
    languageFeatures.qtEnabled = hasQt;
    languageFeatures.qtMocRunEnabled = hasQt;
    if (!hasQt) {
        languageFeatures.qtKeywordsEnabled = false;
    } else {
        const QByteArray noKeywordsMacro = "#define QT_NO_KEYWORDS";
        const int noKeywordsIndex = projectDefines.indexOf(noKeywordsMacro);
        if (noKeywordsIndex == -1) {
            languageFeatures.qtKeywordsEnabled = true;
        } else {
            const char nextChar = projectDefines.at(noKeywordsIndex + noKeywordsMacro.length());
            // Detect "#define QT_NO_KEYWORDS" and "#define QT_NO_KEYWORDS 1", but exclude
            // "#define QT_NO_KEYWORDS_FOO"
            languageFeatures.qtKeywordsEnabled = nextChar != '\n' && nextChar != ' ';
        }
    }
}

ProjectPart::Ptr ProjectPart::copy() const
{
    return Ptr(new ProjectPart(*this));
}

QString ProjectPart::id() const
{
    QString projectPartId = QDir::fromNativeSeparators(projectFile);
    if (!displayName.isEmpty())
        projectPartId.append(QLatin1Char(' ') + displayName);
    return projectPartId;
}

QByteArray ProjectPart::readProjectConfigFile(const ProjectPart::Ptr &part)
{
    QByteArray result;

    QFile f(part->projectConfigFile);
    if (f.open(QIODevice::ReadOnly)) {
        QTextStream is(&f);
        result = is.readAll().toUtf8();
        f.close();
    }

    return result;
}

} // namespace CppTools
