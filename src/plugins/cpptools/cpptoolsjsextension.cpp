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

#include "cpptoolsjsextension.h"

#include "cppfilesettingspage.h"

#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/session.h>

#include <utils/codegeneration.h>
#include <utils/fileutils.h>

#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

namespace CppTools {
namespace Internal {

static QString fileName(const QString &path, const QString &extension)
{
    return Utils::FilePath::fromStringWithExtension(path, extension).toString();
}

QString CppToolsJsExtension::headerGuard(const QString &in) const
{
    return Utils::headerGuard(in);
}

static QStringList parts(const QString &klass)
{
    return klass.split(QStringLiteral("::"));
}

QStringList CppToolsJsExtension::namespaces(const QString &klass) const
{
    QStringList result = parts(klass);
    result.removeLast();
    return result;
}

bool CppToolsJsExtension::hasNamespaces(const QString &klass) const
{
    return !namespaces(klass).empty();
}

QString CppToolsJsExtension::className(const QString &klass) const
{
    QStringList result = parts(klass);
    return result.last();
}

QString CppToolsJsExtension::classToFileName(const QString &klass, const QString &extension) const
{
    const QString raw = fileName(className(klass), extension);
    CppFileSettings settings;
    settings.fromSettings(Core::ICore::settings());
    if (!settings.lowerCaseFiles)
        return raw;

    QFileInfo fi = QFileInfo(raw);
    QString finalPath = fi.path();
    if (finalPath == QStringLiteral("."))
        finalPath.clear();
    if (!finalPath.isEmpty() && !finalPath.endsWith(QLatin1Char('/')))
        finalPath += QLatin1Char('/');
    QString name = fi.baseName().toLower();
    QString ext = fi.completeSuffix();
    if (!ext.isEmpty())
        ext = QString(QLatin1Char('.')) + ext;
    return finalPath + name + ext;
}

QString CppToolsJsExtension::classToHeaderGuard(const QString &klass, const QString &extension) const
{
    return Utils::headerGuard(fileName(className(klass), extension), namespaces(klass));
}

QString CppToolsJsExtension::openNamespaces(const QString &klass) const
{
    QString result;
    QTextStream str(&result);
    Utils::writeOpeningNameSpaces(namespaces(klass), QString(), str);
    return result;
}

QString CppToolsJsExtension::closeNamespaces(const QString &klass) const
{
    QString result;
    QTextStream str(&result);
    Utils::writeClosingNameSpaces(namespaces(klass), QString(), str);
    return result;
}

QString CppToolsJsExtension::includeStatement(
        const QString &fullyQualifiedClassName,
        const QString &suffix,
        const QString &specialClasses,
        const QString &pathOfIncludingFile
        )
{
    if (fullyQualifiedClassName.isEmpty())
        return {};
    const QString className = parts(fullyQualifiedClassName).last();
    if (className.isEmpty() || specialClasses.contains(className))
        return {};
    if (className.startsWith('Q') && className.length() > 2 && className.at(1).isUpper())
        return "#include <" + className + ">\n";
    const auto withUnderScores = [&className] {
        QString baseName = className;
        baseName[0] = baseName[0].toLower();
        for (int i = 1; i < baseName.length(); ++i) {
            if (baseName[i].isUpper()) {
                baseName.insert(i, '_');
                baseName[i + 1] = baseName[i + 1].toLower();
                ++i;
            }
        }
        return baseName;
    };
    QStringList candidates{className + '.' + suffix};
    bool hasUpperCase = false;
    bool hasLowerCase = false;
    for (int i = 0; i < className.length() && (!hasUpperCase || !hasLowerCase); ++i) {
        if (className.at(i).isUpper())
            hasUpperCase = true;
        if (className.at(i).isLower())
            hasLowerCase = true;
    }
    if (hasUpperCase)
        candidates << className.toLower() + '.' + suffix;
    if (hasUpperCase && hasLowerCase)
        candidates << withUnderScores()  + '.' + suffix;
    candidates.removeDuplicates();
    using namespace ProjectExplorer;
    const auto nodeMatchesFileName = [&candidates](Node *n) {
        if (const FileNode * const fileNode = n->asFileNode()) {
            if (fileNode->fileType() == FileType::Header
                && candidates.contains(fileNode->filePath().fileName())) {
                return true;
            }
        }
        return false;
    };
    for (const Project * const p : SessionManager::projects()) {
        const Node *theNode = p->rootProjectNode()->findNode(nodeMatchesFileName);
        if (theNode) {
            const bool sameDir = pathOfIncludingFile == theNode->filePath().toFileInfo().path();
            return QString("#include ")
                    .append(sameDir ? '"' : '<')
                    .append(theNode->filePath().fileName())
                    .append(sameDir ? '"' : '>')
                    .append('\n');
        }
    }
    return {};
}

} // namespace Internal
} // namespace CppTools
