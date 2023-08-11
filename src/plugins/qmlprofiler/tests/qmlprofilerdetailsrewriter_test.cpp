// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmlprofilerdetailsrewriter_test.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/target.h>

#include <utils/filepath.h>

#include <QLibraryInfo>
#include <QTest>

using namespace ProjectExplorer;
using namespace Utils;

namespace QmlProfiler {
namespace Internal {

class DummyProject : public ProjectExplorer::Project
{
public:
    DummyProject(const Utils::FilePath &file)
        : ProjectExplorer::Project(QString(), file)
    {
        auto fileNode
                = std::make_unique<ProjectExplorer::FileNode>(file, ProjectExplorer::FileType::Source);
        auto root = std::make_unique<ProjectExplorer::ProjectNode>(file);
        root->addNode(std::move(fileNode));
        fileNode = std::make_unique<ProjectExplorer::FileNode>(
                    Utils::FilePath::fromString(
                        ":/qmlprofiler/tests/qmlprofilerdetailsrewriter_test.cpp"),
                    ProjectExplorer::FileType::Source);
        root->addNode(std::move(fileNode));
        setRootProjectNode(std::move(root));
        setDisplayName(file.toString());
        setId("QmlProfilerDetailsRewriterTest.DummyProject");
    }

    bool needsConfiguration() const final { return false; }
};

class DummyBuildConfigurationFactory : public BuildConfigurationFactory
{
public:
    DummyBuildConfigurationFactory()
    {
        setBuildGenerator([](const Kit *, const FilePath &, bool) { return QList<BuildInfo>{}; });
    }
};

QmlProfilerDetailsRewriterTest::QmlProfilerDetailsRewriterTest(QObject *parent) : QObject(parent)
{
    connect(&m_rewriter, &QmlProfilerDetailsRewriter::eventDetailsChanged,
                this, [this]() {
        m_rewriterDone = true;
    });
}

void QmlProfilerDetailsRewriterTest::testMissingModelManager()
{
    DummyBuildConfigurationFactory factory;
    Q_UNUSED(factory)

    seedRewriter();
    delete m_modelManager;
    m_modelManager = nullptr;

    // Verify it doesn't crash if model manager is missing.
    QVERIFY(!m_rewriterDone);
    auto rewriteConnection = connect(&m_rewriter, &QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, [&](int typeId, const QString &string) {
        Q_UNUSED(typeId)
        Q_UNUSED(string)
        QFAIL("found nonexisting file in nonexisting model manager");
    });
    m_rewriter.requestDetailsForLocation(44, QmlEventLocation("Test.qml", 12, 12));
    m_rewriter.reloadDocuments();
    QVERIFY(m_rewriterDone);
    m_rewriterDone = false;
    disconnect(rewriteConnection);
}

void QmlProfilerDetailsRewriterTest::testRequestDetailsForLocation()
{
    DummyBuildConfigurationFactory factory;
    Q_UNUSED(factory)

    seedRewriter();
    QVERIFY(!m_rewriterDone);
    bool found1 = false;
    bool found2 = false;
    auto rewriteConnection = connect(&m_rewriter, &QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, [&](int typeId, const QString &string) {
        if (typeId == 1) {
            QCOMPARE(string, QString::fromLatin1("property int dings: 12 + 12 + 12"));
            found1 = true;
        } else if (typeId == 2) {
            QCOMPARE(string, QString::fromLatin1("objectName: { return no + Math.random(); }"));
            found2 = true;
        } else {
            QFAIL("invalid typeId");
        }
    });

    m_rewriter.requestDetailsForLocation(12, QmlEventLocation("Test.qml", 5, 25));
    // simulate unrelated document update: Shouldn't send any events our way.
    emit m_modelManager->documentUpdated(QmlJS::Document::create("gibtsnich.qml",
                                                                 QmlJS::Dialect::Qml));
    m_rewriter.clear();
    QVERIFY(!found1);
    QVERIFY(!found2);
    QVERIFY(!m_rewriterDone);

    m_rewriter.requestDetailsForLocation(1, QmlEventLocation("Test.qml", 4, 25));
    m_rewriter.requestDetailsForLocation(2, QmlEventLocation("Test.qml", 6, 17));
    m_rewriter.requestDetailsForLocation(42, QmlEventLocation("gibtsnich.qml", 6, 17));
    m_rewriter.requestDetailsForLocation(14, QmlEventLocation("Test.qml", 55, 4));
    m_rewriter.reloadDocuments();

    QTRY_VERIFY(found1);
    QTRY_VERIFY(found2);
    QTRY_VERIFY(m_rewriterDone);

    found1 = found2 = m_rewriterDone = false;
    m_rewriter.reloadDocuments();
    QVERIFY(!found1);
    QVERIFY(!found2);
    QVERIFY(m_rewriterDone);
    m_rewriterDone = false;

    disconnect(rewriteConnection);
}

void QmlProfilerDetailsRewriterTest::testGetLocalFile()
{
    DummyBuildConfigurationFactory factory;
    Q_UNUSED(factory)

    seedRewriter();
    QCOMPARE(m_rewriter.getLocalFile("notthere.qml"), Utils::FilePath());
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"),
             Utils::FilePath::fromString(":/qmlprofiler/tests/Test.qml"));
    QCOMPARE(m_rewriter.getLocalFile("qmlprofilerdetailsrewriter_test.cpp"), Utils::FilePath());
}

void QmlProfilerDetailsRewriterTest::testPopulateFileFinder()
{
    m_rewriter.populateFileFinder(nullptr);
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"), Utils::FilePath());

    // Test that the rewriter will populate from available projects if given nullptr as parameter.
    DummyProject *project1 = new DummyProject(":/nix.nix");
    ProjectExplorer::ProjectManager::addProject(project1);
    DummyProject *project2 = new DummyProject(":/qmlprofiler/tests/Test.qml");
    ProjectExplorer::ProjectManager::addProject(project2);
    m_rewriter.populateFileFinder(nullptr);
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"),
             Utils::FilePath::fromString(":/qmlprofiler/tests/Test.qml"));

    ProjectExplorer::ProjectManager::removeProject(project1);
    ProjectExplorer::ProjectManager::removeProject(project2);
}

void QmlProfilerDetailsRewriterTest::seedRewriter()
{
    delete m_modelManager;
    m_modelManager = new QmlJS::ModelManagerInterface(this);
    QString filename = ":/qmlprofiler/tests/Test.qml";

    QmlJS::PathsAndLanguages lPaths;
    lPaths.maybeInsert(
                Utils::FilePath::fromString(QLibraryInfo::path(QLibraryInfo::Qml2ImportsPath)),
                QmlJS::Dialect::Qml);
    QmlJS::ModelManagerInterface::importScan(QmlJS::ModelManagerInterface::workingCopy(),
                                             lPaths, m_modelManager, false);

    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    QmlJS::Document::MutablePtr doc = QmlJS::Document::create(Utils::FilePath::fromString(filename), QmlJS::Dialect::Qml);
    doc->setSource(content);
    doc->parse();
    QVERIFY(!doc->source().isEmpty());

    auto kit = std::make_unique<ProjectExplorer::Kit>();
    ProjectExplorer::SysRootKitAspect::setSysRoot(kit.get(), "/nowhere");

    DummyProject *project = new DummyProject(Utils::FilePath::fromString(filename));
    ProjectExplorer::ProjectManager::addProject(project);

    m_rewriter.populateFileFinder(project->addTargetForKit(kit.get()));

    ProjectExplorer::ProjectManager::removeProject(project);
}

} // namespace Internal
} // namespace QmlProfiler
