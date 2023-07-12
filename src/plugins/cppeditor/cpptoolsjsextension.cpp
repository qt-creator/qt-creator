// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpptoolsjsextension.h"

#include "cppeditorplugin.h"
#include "cppfilesettingspage.h"
#include "cpplocatordata.h"
#include "cppworkingcopy.h"

#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projecttree.h>

#include <cplusplus/AST.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/Overview.h>

#include <utils/codegeneration.h>
#include <utils/fileutils.h>

#include <QElapsedTimer>
#include <QFileInfo>
#include <QStringList>
#include <QTextStream>

namespace CppEditor::Internal {

static CppFileSettings fileSettings()
{
    // Note that the user can set a different project in the wizard *after* the file names
    // have been determined. There's nothing we can do about that here.
    return CppEditorPlugin::fileSettings(ProjectExplorer::ProjectTree::currentProject());
}

static QString fileName(const QString &path, const QString &extension)
{
    return Utils::FilePath::fromStringWithExtension(path, extension).toString();
}

QString CppToolsJsExtension::headerGuard(const QString &in) const
{
    return Utils::headerGuard(in);
}

QString CppToolsJsExtension::licenseTemplate() const
{
    return fileSettings().licenseTemplate();
}

bool CppToolsJsExtension::usePragmaOnce() const
{
    return fileSettings().headerPragmaOnce;
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
    const CppFileSettings &settings = fileSettings();
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

bool CppToolsJsExtension::hasQObjectParent(const QString &klassName) const
{
    // This is a synchronous function, but the look-up is potentially expensive.
    // Since it's not crucial information, we just abort if retrieving it takes too long,
    // in order not to freeze the UI.
    // TODO: Any chance to at least cache between successive invocations for the same dialog?
    //       I don't see it atm...
    QElapsedTimer timer;
    timer.start();
    static const int timeout = 5000;

    // Find symbol.
    QList<IndexItem::Ptr> candidates;
    m_locatorData->filterAllFiles([&](const IndexItem::Ptr &item) {
        if (timer.elapsed() > timeout)
            return IndexItem::VisitorResult::Break;
        if (item->scopedSymbolName() == klassName) {
            candidates = {item};
            return IndexItem::VisitorResult::Break;
        }
        if (item->symbolName() == klassName)
            candidates << item;
        return IndexItem::VisitorResult::Recurse;
    });
    if (timer.elapsed() > timeout)
        return false;
    if (candidates.isEmpty())
        return false;
    const IndexItem::Ptr item = candidates.first();

    // Find class in AST.
    const CPlusPlus::Snapshot snapshot = CppModelManager::snapshot();
    const WorkingCopy workingCopy = CppModelManager::workingCopy();
    std::optional<QByteArray> source = workingCopy.source(item->filePath());
    if (!source) {
        const Utils::expected_str<QByteArray> contents = item->filePath().fileContents();
        QTC_ASSERT_EXPECTED(contents, return false);
        source = *contents;
    }
    const auto doc = snapshot.preprocessedDocument(*source, item->filePath());
    if (!doc)
        return false;
    doc->check();
    if (!doc->translationUnit())
        return false;
    if (timer.elapsed() > timeout)
        return false;
    CPlusPlus::ClassSpecifierAST *classSpec = nullptr;
    const QList<CPlusPlus::AST *> astPath = CPlusPlus::ASTPath(doc)(item->line(), item->column());
    for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
        if ((classSpec = (*it)->asClassSpecifier()))
            break;
    }
    if (!classSpec)
        return false;

    // Check whether constructor has a QObject parent parameter.
    CPlusPlus::Overview overview;
    const CPlusPlus::Class * const klass = classSpec->symbol;
    if (!klass)
        return false;
    for (auto it = klass->memberBegin(); it != klass->memberEnd(); ++it) {
        const CPlusPlus::Symbol * const member = *it;
        if (overview.prettyName(member->name()) != item->symbolName())
            continue;
        const CPlusPlus::Function *function = (*it)->asFunction();
        if (!function)
            function = member->type().type()->asFunctionType();
        if (!function)
            continue;
        for (int i = 0; i < function->argumentCount(); ++i) {
            const CPlusPlus::Symbol * const arg = function->argumentAt(i);
            const QString argName = overview.prettyName(arg->name());
            const QString argType = overview.prettyType(arg->type())
                    .split("::", Qt::SkipEmptyParts).last();
            if (argName == "parent" && argType == "QObject *")
                return true;
        }
    }

    return false;
}

QString CppToolsJsExtension::includeStatement(
        const QString &fullyQualifiedClassName,
        const QString &suffix,
        const QStringList &specialClasses,
        const QString &pathOfIncludingFile
        )
{
    if (fullyQualifiedClassName.isEmpty())
        return {};
    const QString className = parts(fullyQualifiedClassName).constLast();
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
    for (const Project * const p : ProjectManager::projects()) {
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

QString CppToolsJsExtension::cxxHeaderSuffix() const
{
    return fileSettings().headerSuffix;
}

QString CppToolsJsExtension::cxxSourceSuffix() const
{
    return fileSettings().sourceSuffix;
}

} // namespace CppEditor::Internal
