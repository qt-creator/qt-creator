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

#include "qmlprofilerdetailsrewriter_test.h"

#include <projectexplorer/customexecutablerunconfiguration.h>
#include <projectexplorer/project.h>
#include <projectexplorer/target.h>
#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectnodes.h>
#include <projectexplorer/session.h>
#include <projectexplorer/kitinformation.h>

#include <QLibraryInfo>
#include <QTest>

namespace QmlProfiler {
namespace Internal {

class DummyProjectNode : public ProjectExplorer::ProjectNode
{
public:
    DummyProjectNode(const Utils::FileName &file) : ProjectExplorer::ProjectNode(file)
    {}
};

class DummyProject : public ProjectExplorer::Project
{
    Q_OBJECT
public:
    DummyProject(const Utils::FileName &file) :
        ProjectExplorer::Project(QString(), file, {})
    {
        ProjectExplorer::FileNode *fileNode
                = new ProjectExplorer::FileNode(file, ProjectExplorer::FileType::Source, false);
        DummyProjectNode *root = new DummyProjectNode(file);
        root->addNode(fileNode);
        fileNode = new ProjectExplorer::FileNode(
                    Utils::FileName::fromLatin1(
                        ":/qmlprofiler/tests/qmlprofilerdetailsrewriter_test.cpp"),
                    ProjectExplorer::FileType::Source, false);
        root->addNode(fileNode);
        setRootProjectNode(root);
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
    seedRewriter();
    delete m_modelManager;
    m_modelManager = nullptr;

    // Verify it doesn't crash if model manager is missing.
    QVERIFY(!m_rewriterDone);
    auto rewriteConnection = connect(&m_rewriter, &QmlProfilerDetailsRewriter::rewriteDetailsString,
            this, [&](int typeId, const QString &string) {
        Q_UNUSED(typeId);
        Q_UNUSED(string);
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
    seedRewriter();
    QCOMPARE(m_rewriter.getLocalFile("notthere.qml"), QString());
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"),
             QString::fromLatin1(":/qmlprofiler/tests/Test.qml"));
    QCOMPARE(m_rewriter.getLocalFile("qmlprofilerdetailsrewriter_test.cpp"), QString());
}

void QmlProfilerDetailsRewriterTest::testPopulateFileFinder()
{
    m_rewriter.populateFileFinder(nullptr);
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"), QString());

    // Test that the rewriter will populate from available projects if given nullptr as parameter.
    DummyProject *project1 = new DummyProject(Utils::FileName::fromString(":/nix.nix"));
    ProjectExplorer::SessionManager::addProject(project1);
    DummyProject *project2 = new DummyProject(
                Utils::FileName::fromString(":/qmlprofiler/tests/Test.qml"));
    ProjectExplorer::SessionManager::addProject(project2);
    m_rewriter.populateFileFinder(nullptr);
    QCOMPARE(m_rewriter.getLocalFile("Test.qml"),
             QString::fromLatin1(":/qmlprofiler/tests/Test.qml"));

    ProjectExplorer::SessionManager::removeProject(project1);
    ProjectExplorer::SessionManager::removeProject(project2);
}

void QmlProfilerDetailsRewriterTest::seedRewriter()
{
    delete m_modelManager;
    m_modelManager = new QmlJS::ModelManagerInterface(this);
    QString filename = ":/qmlprofiler/tests/Test.qml";

    QFutureInterface<void> result;
    QmlJS::PathsAndLanguages lPaths;
    for (auto p : QLibraryInfo::location(QLibraryInfo::Qml2ImportsPath))
        lPaths.maybeInsert(Utils::FileName::fromString(p), QmlJS::Dialect::Qml);
    QmlJS::ModelManagerInterface::importScan(result, QmlJS::ModelManagerInterface::workingCopy(),
                                             lPaths, m_modelManager, false);

    QFile file(filename);
    file.open(QFile::ReadOnly | QFile::Text);
    const QString content = QString::fromUtf8(file.readAll());
    file.close();

    QmlJS::Document::MutablePtr doc = QmlJS::Document::create(filename, QmlJS::Dialect::Qml);
    doc->setSource(content);
    doc->parse();
    QVERIFY(!doc->source().isEmpty());

    ProjectExplorer::Kit *kit = new ProjectExplorer::Kit;
    ProjectExplorer::SysRootKitInformation::setSysRoot(
                kit, Utils::FileName::fromLatin1("/nowhere"));

    DummyProject *project = new DummyProject(Utils::FileName::fromString(filename));
    ProjectExplorer::SessionManager::addProject(project);
    ProjectExplorer::Target *target = project->createTarget(kit);

    m_rewriter.populateFileFinder(target);
    ProjectExplorer::SessionManager::removeProject(project);
    ProjectExplorer::KitManager::deleteKit(kit);
}

} // namespace Internal
} // namespace QmlProfiler

#include "qmlprofilerdetailsrewriter_test.moc"
