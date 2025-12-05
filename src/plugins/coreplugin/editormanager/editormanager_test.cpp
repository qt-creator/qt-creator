// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "../generalsettings.h"
#include "documentmodel_p.h"
#include "editormanager_p.h"

#include <utils/temporaryfile.h>

#include <QTest>

using namespace Utils;

namespace Core::Internal {

using EMP = EditorManagerPrivate;
using EM = EditorManager;

class TabbedEditorTest : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();

    void init();
    void cleanup();

    void testCloseLastTabInSplit();
    void testKeepTabForMovedEditors();
    void testKeepTabForSuspendedDocuments();
    void testKeepDocumentWhenClosingTabWithMoreSuspendedTabs();
    void testAlwaysSwitchToTab();
    void testCloseSplit();
    void testPinned();
};

QObject *createTabbedEditorTest()
{
    return new TabbedEditorTest;
}

static int fileCount = 0;

class TestFile : public TemporaryFile
{
public:
    TestFile()
        : TemporaryFile(QString("testfile%1").arg(++fileCount))
    {
        QVERIFY(open());
    }

    ~TestFile()
    {
        if (isOpen())
            close();
    }
};

QList<EditorView *> mainAreaViews()
{
    EditorView *it = EMP::mainEditorArea()->findFirstView();
    QList<EditorView *> views;
    while (it) {
        views += it;
        it = it->findNextView();
    }
    return views;
}

static void closeAll()
{
    EditorManager::closeAllEditors(false);
    EditorArea *mainArea = EMP::mainEditorArea();
    if (mainArea->hasSplits())
        mainArea->unsplit(mainArea->findFirstView());
}

void TabbedEditorTest::initTestCase()
{
    generalSettings().useTabsInEditorViews.setValue(true);
}

void TabbedEditorTest::cleanupTestCase()
{
    generalSettings().useTabsInEditorViews.setValue(false);
}

void TabbedEditorTest::init()
{
    closeAll();
}

void TabbedEditorTest::cleanup()
{
    closeAll();
}

// Test descriptions:
// A, B, C etc refer to specific files/documents
// ! designates the current document in the view
// (s) designates a tab that is not backed by an editor ("suspended tab")

/*
    When closing the last tab in a view, we want the view to show no editor anymore.
    Without tabs we show another open document there, if available.
*/
void TabbedEditorTest::testCloseLastTabInSplit()
{
    // A! in first view, B! in second view
    // close B
    // A! in first view, none in second view

    // without tabs, A is opened in the second view as well, since we do not want empy views as long
    // as there are documents
    TestFile a;
    TestFile b;
    EMP::mainEditorArea()->findFirstView()->split(Qt::Vertical);
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 2);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    QCOMPARE(views.at(0)->editors(), QList<IEditor *>{editorA});
    IEditor *editorB = EMP::openEditor(views.at(1), b.filePath());
    QCOMPARE(views.at(1)->editors(), QList<IEditor *>{editorB});

    EMP::closeEditorOrDocument(editorB);

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorA}));
    QVERIFY(views.at(1)->editors().isEmpty());
    QVERIFY(views.at(1)->tabs().isEmpty());
}

/*
    When moving editors to a different view because they are not visible in the source view,
    we have to keep the tab.
*/
void TabbedEditorTest::testKeepTabForMovedEditors()
{
    // A and B! in first view
    // open A in second view
    // A(s) und B! in first view, A! in second view
    TestFile a;
    TestFile b;
    EMP::mainEditorArea()->findFirstView()->split(Qt::Vertical);
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 2);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    IEditor *editorB = EMP::openEditor(views.at(0), b.filePath());
    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorA, editorB}));
    QCOMPARE(views.at(0)->currentEditor(), editorB);

    EMP::openEditor(views.at(1), a.filePath());

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB}));
    const QList<EditorView::TabData> view0Tabs = views.at(0)->tabs();
    QCOMPARE(view0Tabs.size(), 2);
    QCOMPARE(view0Tabs.at(0).editor, nullptr);
    QCOMPARE(view0Tabs.at(0).entry->filePath(), a.filePath());
    QCOMPARE(view0Tabs.at(1).editor, editorB);
    QCOMPARE(views.at(0)->currentEditor(), editorB);
    QCOMPARE(views.at(1)->editors(), (QList<IEditor *>{editorA}));
}

/*
    When suspending documents, the tab should stay (as a suspended tab).
    And when closing the only tab for a suspended entry, that should be removed.
*/
void TabbedEditorTest::testKeepTabForSuspendedDocuments()
{
    // A and B! in view
    // explicitly suspend A
    // A(s) and B! in view
    // close tab for A
    // B! in view, entry for A is removed
    TestFile a;
    TestFile b;
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 1);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    IEditor *editorB = EMP::openEditor(views.at(0), b.filePath());
    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorA, editorB}));
    QCOMPARE(views.at(0)->currentEditor(), editorB);

    EMP::closeEditors({editorA}, EMP::CloseFlag::Suspend);

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB}));
    const QList<EditorView::TabData> view0Tabs = views.at(0)->tabs();
    QCOMPARE(view0Tabs.size(), 2);
    QCOMPARE(view0Tabs.at(0).editor, nullptr);
    QCOMPARE(view0Tabs.at(0).entry->filePath(), a.filePath());
    QCOMPARE(view0Tabs.at(1).editor, editorB);
    DocumentModel::Entry *entryA = DocumentModel::entryForFilePath(a.filePath());
    QVERIFY(entryA != nullptr);

    views.at(0)->closeTab(entryA);

    QCOMPARE(views.at(0)->tabs().size(), 1);
    QVERIFY(DocumentModel::entryForFilePath(a.filePath()) == nullptr);
}

/*
    When closing a tab for a document, we may not close the editor if there is
    another tab for it, even if it is suspended.

    But when we close a last remaining suspended tab, the editor/document needs to be closed.
*/
void TabbedEditorTest::testKeepDocumentWhenClosingTabWithMoreSuspendedTabs()
{
    // A(s) and B! in first view, A! in second view
    // close tab for A in second view
    // A(s) and B! in first view, none in second view, editor for A still in document model
    // close tab A(s) in first view
    // B! in first view, none in second view, editor for A is closed
    TestFile a;
    TestFile b;
    EMP::mainEditorArea()->findFirstView()->split(Qt::Vertical);
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 2);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    IEditor *editorB = EMP::openEditor(views.at(0), b.filePath());
    EMP::openEditor(views.at(1), a.filePath());
    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB}));
    QCOMPARE(views.at(0)->tabs().size(), 2);
    QCOMPARE(views.at(1)->editors(), (QList<IEditor *>{editorA}));
    // now we have A(s) and B! in first view, A! in second view
    // that is the state tested in testKeepTabForMovedEditors

    EMP::closeEditorOrDocument(editorA);

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB}));
    const QList<EditorView::TabData> view0Tabs = views.at(0)->tabs();
    QCOMPARE(view0Tabs.size(), 2);
    QCOMPARE(view0Tabs.at(0).editor, nullptr);
    QCOMPARE(view0Tabs.at(0).entry->filePath(), a.filePath());
    QCOMPARE(view0Tabs.at(1).editor, editorB);
    QVERIFY(views.at(1)->editors().isEmpty());
    QVERIFY(views.at(1)->tabs().isEmpty());
    const QList<IEditor *> editorsForA = DocumentModel::editorsForFilePath(a.filePath());
    QCOMPARE(editorsForA.size(), 1);
    QCOMPARE(editorsForA.at(0), editorA);
    DocumentModel::Entry *entryForA = DocumentModel::entryForFilePath(a.filePath());
    QVERIFY(entryForA != nullptr);

    views.at(0)->closeTab(entryForA);

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB}));
    QCOMPARE(views.at(0)->tabs().size(), 1);
    QVERIFY(DocumentModel::editorsForFilePath(a.filePath()).isEmpty());
    QVERIFY(DocumentModel::entryForFilePath(a.filePath()) == nullptr);
}

/*
    When a view has only suspended tabs plus the active one,
    when closing the active tab it should switch to one of the suspended tabs.
    Without tabs it would just choose an existing non-visible editor if available.
*/
void TabbedEditorTest::testAlwaysSwitchToTab()
{
    // A and B! in first view, C(s) and D! in second view
    // close D
    // A and B! in first view, C in second view
    TestFile a;
    TestFile b;
    TestFile c;
    TestFile d;
    EMP::mainEditorArea()->findFirstView()->split(Qt::Vertical);
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 2);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    IEditor *editorB = EMP::openEditor(views.at(0), b.filePath());
    IEditor *editorC = EMP::openEditor(views.at(1), c.filePath());
    IEditor *editorD = EMP::openEditor(views.at(1), d.filePath());
    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorA, editorB}));
    QCOMPARE(views.at(1)->editors(), (QList<IEditor *>{editorC, editorD}));
    EMP::closeEditors({editorC}, EMP::CloseFlag::Suspend); // editorC is dead now
    QCOMPARE(views.at(1)->editors(), (QList<IEditor *>{editorD}));

    EMP::closeEditorOrDocument(editorD);

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorA, editorB}));
    QCOMPARE(views.at(1)->editors().size(), 1);
    QCOMPARE(views.at(1)->editors().at(0)->document()->filePath(), c.filePath());
    QCOMPARE(views.at(1)->tabs().size(), 1);
}

/*
    When closing a view, this should close editors if the view had the only tab (suspended or not),
    but not if there are other tabs for them in other views (suspended or not).
*/
void TabbedEditorTest::testCloseSplit()
{
    // A(s), C!, and B in first view, B(s), D(s), E!, A in second view, with editor for D in
    // the document model
    // close second view
    // first view unchanged, A moved to document model, D and E closed
    TestFile a;
    TestFile b;
    TestFile c;
    TestFile d;
    TestFile e;
    EMP::mainEditorArea()->findFirstView()->split(Qt::Vertical);
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 2);
    IEditor *editorA = EMP::openEditor(views.at(0), a.filePath());
    IEditor *editorB = EMP::openEditor(views.at(1), b.filePath());
    IEditor *editorC = EMP::openEditor(views.at(0), c.filePath());
    IEditor *editorD = EMP::openEditor(views.at(1), d.filePath());
    IEditor *editorE = EMP::openEditor(views.at(1), e.filePath());
    EMP::openEditor(views.at(0), b.filePath());
    EMP::openEditor(views.at(0), d.filePath());
    EMP::closeEditorOrDocument(editorD);
    views.at(0)->setCurrentEditor(editorC);
    EMP::openEditor(views.at(1), a.filePath());
    views.at(1)->setCurrentEditor(editorE);
    // verify the setup
    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB, editorC})); // not in tab order
    QCOMPARE(views.at(1)->editors(), (QList<IEditor *>{editorA, editorE})); // not in tab order
    QList<EditorView::TabData> view0Tabs = views.at(0)->tabs();
    QCOMPARE(view0Tabs.size(), 3);
    QCOMPARE(view0Tabs.at(0).entry->filePath(), a.filePath());
    QCOMPARE(view0Tabs.at(0).editor, nullptr);
    QCOMPARE(view0Tabs.at(1).editor, editorC);
    QCOMPARE(view0Tabs.at(2).editor, editorB);
    QList<EditorView::TabData> view1Tabs = views.at(1)->tabs();
    QCOMPARE(view1Tabs.size(), 4);
    QCOMPARE(view1Tabs.at(0).entry->filePath(), b.filePath());
    QCOMPARE(view1Tabs.at(0).editor, nullptr);
    QCOMPARE(view1Tabs.at(1).entry->filePath(), d.filePath());
    QCOMPARE(view1Tabs.at(1).editor, nullptr);
    QCOMPARE(view1Tabs.at(2).editor, editorE);
    QCOMPARE(view1Tabs.at(3).editor, editorA);
    QCOMPARE(DocumentModel::editorsForFilePath(d.filePath()), (QList<IEditor *>{editorD}));

    EMP::closeView(views.at(1));

    QCOMPARE(views.at(0)->editors(), (QList<IEditor *>{editorB, editorC})); // not in tab order
    QCOMPARE(views.at(0)->tabs(), view0Tabs);
    QCOMPARE(DocumentModel::editorsForFilePath(a.filePath()), (QList<IEditor *>{editorA}));
    QCOMPARE(DocumentModel::entryForFilePath(d.filePath()), nullptr);
    QCOMPARE(DocumentModel::entryForFilePath(e.filePath()), nullptr);
}

void TabbedEditorTest::testPinned()
{
    TestFile a;
    TestFile b;
    const QList<EditorView *> views = mainAreaViews();
    QCOMPARE(views.size(), 1);
    EditorView *view0 = views.at(0);
    IEditor *editorA = EMP::openEditor(view0, a.filePath());
    QVERIFY(editorA);
    QCOMPARE(view0->tabs().size(), 1);
    DocumentModel::Entry *entryA = DocumentModel::entryForDocument(editorA->document());
    QVERIFY(entryA);
    DocumentModelPrivate::setPinned(entryA, true);
    QCOMPARE(entryA->pinned, true);
    // check that clicking the close button unpins instead of closes
    emit view0->tabCloseRequested(0);
    QCOMPARE(view0->tabs().size(), 1);
    QCOMPARE(entryA->pinned, false);
    // and that after that the document is closed
    emit view0->tabCloseRequested(0);
    QCOMPARE(view0->tabs().size(), 0);
}

} // namespace Core::Internal

#include "editormanager_test.moc"
