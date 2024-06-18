// Copyright (C) 2016 Orgad Shaneh <orgads@gmail.com>.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppheadersource.h"

#include "cpptoolsreuse.h"
#include "cppfilesettingspage.h"
#include "cppmodelmanager.h"
#include "cppfilesettingspage.h"

#include <projectexplorer/project.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projecttree.h>

#include <utils/fileutils.h>
#include <utils/mimeconstants.h>
#include <utils/mimeutils.h>
#include <utils/temporarydirectory.h>

#include <QDir>

#ifdef WITH_TESTS
#include "cpptoolstestcase.h"
#include <QtTest>
#endif

using namespace CppEditor::Internal;
using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

enum { debug = 0 };

static QHash<FilePath, FilePath> m_headerSourceMapping;

void Internal::clearHeaderSourceCache()
{
    m_headerSourceMapping.clear();
}

static FilePaths findFilesInProject(const QStringList &names, const Project *project,
                                    FileType fileType)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << names << project;

    if (!project)
        return {};

    const auto filter = [&](const Node *n) {
        const auto fn = n->asFileNode();
        return fn && fn->fileType() == fileType && names.contains(fn->filePath().fileName());
    };
    return project->files(filter);
}

// Return the suffixes that should be checked when trying to find a
// source belonging to a header and vice versa
static QStringList matchingCandidateSuffixes(ProjectFile::Kind kind)
{
    using namespace Utils::Constants;
    switch (kind) {
    case ProjectFile::AmbiguousHeader:
    case ProjectFile::CHeader:
    case ProjectFile::CXXHeader:
    case ProjectFile::ObjCHeader:
    case ProjectFile::ObjCXXHeader:
        return mimeTypeForName(C_SOURCE_MIMETYPE).suffixes()
             + mimeTypeForName(CPP_SOURCE_MIMETYPE).suffixes()
             + mimeTypeForName(OBJECTIVE_C_SOURCE_MIMETYPE).suffixes()
             + mimeTypeForName(OBJECTIVE_CPP_SOURCE_MIMETYPE).suffixes()
             + mimeTypeForName(CUDA_SOURCE_MIMETYPE).suffixes();
    case ProjectFile::CSource:
    case ProjectFile::ObjCSource:
        return mimeTypeForName(C_HEADER_MIMETYPE).suffixes();
    case ProjectFile::CXXSource:
    case ProjectFile::ObjCXXSource:
    case ProjectFile::CudaSource:
    case ProjectFile::OpenCLSource:
        return mimeTypeForName(CPP_HEADER_MIMETYPE).suffixes();
    default:
        return {};
    }
}

static QStringList baseNameWithAllSuffixes(const QString &baseName, const QStringList &suffixes)
{
    QStringList result;
    const QChar dot = QLatin1Char('.');
    for (const QString &suffix : suffixes) {
        QString fileName = baseName;
        fileName += dot;
        fileName += suffix;
        result += fileName;
    }
    return result;
}

static QStringList baseNamesWithAllPrefixes(const CppFileSettings &settings,
                                            const QStringList &baseNames, bool isHeader)
{
    QStringList result;
    const QStringList &sourcePrefixes = settings.sourcePrefixes;
    const QStringList &headerPrefixes = settings.headerPrefixes;

    for (const QString &name : baseNames) {
        for (const QString &prefix : isHeader ? headerPrefixes : sourcePrefixes) {
            if (name.startsWith(prefix)) {
                QString nameWithoutPrefix = name.mid(prefix.size());
                result += nameWithoutPrefix;
                for (const QString &prefix : isHeader ? sourcePrefixes : headerPrefixes)
                    result += prefix + nameWithoutPrefix;
            }
        }
        for (const QString &prefix : isHeader ? sourcePrefixes : headerPrefixes)
            result += prefix + name;

    }
    return result;
}

static QStringList baseDirWithAllDirectories(const QDir &baseDir, const QStringList &directories)
{
    QStringList result;
    for (const QString &dir : directories)
        result << QDir::cleanPath(baseDir.absoluteFilePath(dir));
    return result;
}

static int commonFilePathLength(const QString &s1, const QString &s2)
{
    int length = qMin(s1.length(), s2.length());
    for (int i = 0; i < length; ++i)
        if (HostOsInfo::fileNameCaseSensitivity() == Qt::CaseSensitive) {
            if (s1[i] != s2[i])
                return i;
        } else {
            if (s1[i].toLower() != s2[i].toLower())
                return i;
        }
    return length;
}

static FilePath correspondingHeaderOrSourceInProject(const FilePath &filePath,
                                                     const QStringList &candidateFileNames,
                                                     const Project *project,
                                                     FileType fileType,
                                                     CacheUsage cacheUsage)
{
    const FilePaths projectFiles = findFilesInProject(candidateFileNames, project, fileType);

    // Find the file having the most common path with fileName
    FilePath bestFilePath;
    int compareValue = 0;
    for (const FilePath &projectFile : projectFiles) {
        int value = commonFilePathLength(filePath.toString(), projectFile.toString());
        if (value > compareValue) {
            compareValue = value;
            bestFilePath = projectFile;
        }
    }
    if (!bestFilePath.isEmpty()) {
        QTC_ASSERT(bestFilePath.isFile(), return {});
        if (cacheUsage == CacheUsage::ReadWrite) {
            m_headerSourceMapping[filePath] = bestFilePath;
            m_headerSourceMapping[bestFilePath] = filePath;
        }
        return bestFilePath;
    }

    return {};
}

FilePath correspondingHeaderOrSource(const FilePath &filePath, bool *wasHeader, CacheUsage cacheUsage)
{
    ProjectFile::Kind kind = ProjectFile::classify(filePath.fileName());
    const bool isHeader = ProjectFile::isHeader(kind);
    if (wasHeader)
        *wasHeader = isHeader;
    if (const auto it = m_headerSourceMapping.constFind(filePath);
        it != m_headerSourceMapping.constEnd()) {
        return it.value();
    }

    Project * const projectForFile = ProjectManager::projectForFile(filePath);
    const CppFileSettings settings = cppFileSettingsForProject(projectForFile);

    if (debug)
        qDebug() << Q_FUNC_INFO << filePath.fileName() <<  kind;

    if (kind == ProjectFile::Unsupported)
        return {};

    const QString baseName = filePath.completeBaseName();
    const QString privateHeaderSuffix = QLatin1String("_p");
    const QStringList suffixes = matchingCandidateSuffixes(kind);

    QStringList candidateFileNames = baseNameWithAllSuffixes(baseName, suffixes);
    if (isHeader) {
        if (baseName.endsWith(privateHeaderSuffix)) {
            QString sourceBaseName = baseName;
            sourceBaseName.truncate(sourceBaseName.size() - privateHeaderSuffix.size());
            candidateFileNames += baseNameWithAllSuffixes(sourceBaseName, suffixes);
        }
    } else {
        QString privateHeaderBaseName = baseName;
        privateHeaderBaseName.append(privateHeaderSuffix);
        candidateFileNames += baseNameWithAllSuffixes(privateHeaderBaseName, suffixes);
    }

    const QDir absoluteDir = filePath.toFileInfo().absoluteDir();
    QStringList candidateDirs(absoluteDir.absolutePath());
    // If directory is not root, try matching against its siblings
    const QStringList searchPaths = isHeader ? settings.sourceSearchPaths
                                             : settings.headerSearchPaths;
    candidateDirs += baseDirWithAllDirectories(absoluteDir, searchPaths);

    candidateFileNames += baseNamesWithAllPrefixes(settings, candidateFileNames, isHeader);

    // Try to find a file in the same or sibling directories first
    for (const QString &candidateDir : std::as_const(candidateDirs)) {
        for (const QString &candidateFileName : std::as_const(candidateFileNames)) {
            const FilePath candidateFilePath
                = FilePath::fromString(candidateDir + '/' + candidateFileName).normalizedPathName();
            if (candidateFilePath.isFile()) {
                if (cacheUsage == CacheUsage::ReadWrite) {
                    m_headerSourceMapping[filePath] = candidateFilePath;
                    if (!isHeader || !baseName.endsWith(privateHeaderSuffix))
                        m_headerSourceMapping[candidateFilePath] = filePath;
                }
                return candidateFilePath;
            }
        }
    }

    // Find files in the current project
    Project *currentProject = projectForFile;
    if (!projectForFile)
        currentProject = ProjectTree::currentProject();
    const FileType requestedFileType = isHeader ? FileType::Source : FileType::Header;
    if (currentProject) {
        const FilePath path = correspondingHeaderOrSourceInProject(
            filePath, candidateFileNames, currentProject, requestedFileType, cacheUsage);
        if (!path.isEmpty())
            return path;

    // Find files in other projects
    } else {
        const QList<ProjectInfo::ConstPtr> projectInfos = CppModelManager::projectInfos();
        for (const ProjectInfo::ConstPtr &projectInfo : projectInfos) {
            const Project *project = projectInfo->project();
            if (project == currentProject)
                continue; // We have already checked the current project.

            const FilePath path = correspondingHeaderOrSourceInProject(
                filePath, candidateFileNames, project, requestedFileType, cacheUsage);
            if (!path.isEmpty())
                return path;
        }
    }

    return {};
}

} // CppEditor

#ifdef WITH_TESTS

namespace CppEditor::Internal {

static inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

static void createTempFile(const FilePath &filePath)
{
    QString fileName = filePath.toString();
    QFile file(fileName);
    QDir(QFileInfo(fileName).absolutePath()).mkpath(_("."));
    file.open(QFile::WriteOnly);
    file.close();
}

static QString baseTestDir()
{
    return Utils::TemporaryDirectory::masterDirectoryPath() + "/qtc_cppheadersource/";
}

class HeaderSourceTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void test_data();
    void test();
};

void HeaderSourceTest::test()
{
    QFETCH(QString, sourceFileName);
    QFETCH(QString, headerFileName);

    CppEditor::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    const QDir path = QDir(temporaryDir.path() + QLatin1Char('/') + _(QTest::currentDataTag()));
    const FilePath sourcePath = FilePath::fromString(path.absoluteFilePath(sourceFileName));
    const FilePath headerPath = FilePath::fromString(path.absoluteFilePath(headerFileName));
    createTempFile(sourcePath);
    createTempFile(headerPath);

    bool wasHeader;
    clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(sourcePath, &wasHeader, CacheUsage::ReadWrite), headerPath);
    QVERIFY(!wasHeader);
    clearHeaderSourceCache();
    QCOMPARE(correspondingHeaderOrSource(headerPath, &wasHeader, CacheUsage::ReadWrite), sourcePath);
    QVERIFY(wasHeader);
}

void HeaderSourceTest::test_data()
{
    QTest::addColumn<QString>("sourceFileName");
    QTest::addColumn<QString>("headerFileName");
    QTest::newRow("samedir") << _("foo.cpp") << _("foo.h");
    QTest::newRow("includesub") << _("foo.cpp") << _("include/foo.h");
    QTest::newRow("headerprefix") << _("foo.cpp") << _("testh_foo.h");
    QTest::newRow("sourceprefixwsub") << _("testc_foo.cpp") << _("include/foo.h");
    QTest::newRow("sourceAndHeaderPrefixWithBothsub") << _("src/testc_foo.cpp") << _("include/testh_foo.h");
}

void HeaderSourceTest::initTestCase()
{
    QDir(baseTestDir()).mkpath(_("."));
    CppFileSettings &fs = globalCppFileSettings();
    fs.headerSearchPaths.append(QLatin1String("include"));
    fs.headerSearchPaths.append(QLatin1String("../include"));
    fs.sourceSearchPaths.append(QLatin1String("src"));
    fs.sourceSearchPaths.append(QLatin1String("../src"));
    fs.headerPrefixes.append(QLatin1String("testh_"));
    fs.sourcePrefixes.append(QLatin1String("testc_"));
}

void HeaderSourceTest::cleanupTestCase()
{
    Utils::FilePath::fromString(baseTestDir()).removeRecursively();
    CppFileSettings &fs = globalCppFileSettings();
    fs.headerSearchPaths.removeLast();
    fs.headerSearchPaths.removeLast();
    fs.sourceSearchPaths.removeLast();
    fs.sourceSearchPaths.removeLast();
    fs.headerPrefixes.removeLast();
    fs.sourcePrefixes.removeLast();
}

QObject *createCppHeaderSourceTest()
{
    return new HeaderSourceTest;
}

} // namespace CppEditor::Internal

#include "cppheadersource.moc"

#endif // WITH_TESTS
