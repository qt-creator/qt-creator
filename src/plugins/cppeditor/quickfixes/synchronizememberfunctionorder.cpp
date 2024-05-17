// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "synchronizememberfunctionorder.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/ASTPath.h>

#include <QList>
#include <QHash>

#ifdef WITH_TESTS
#include "cppquickfix_test.h"
#include <projectexplorer/kitmanager.h>
#include <texteditor/textdocument.h>
#include <QtTest>
#endif

#include <memory>

using namespace Core;
using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class SynchronizeMemberFunctionOrderOp : public CppQuickFixOperation
{
public:
    SynchronizeMemberFunctionOrderOp(
        const CppQuickFixInterface &interface, const QList<Symbol *> &decls)
        : CppQuickFixOperation(interface), m_state(std::make_shared<State>())
    {
        setDescription(
            Tr::tr("Re-order Member Function Definitions According to Declaration Order"));
        m_state->decls = decls;
    }

private:
    struct DefLocation {
        Symbol *decl = nullptr;
        Link defLoc;
        bool operator==(const DefLocation &other) const
        {
            return decl == other.decl && defLoc == other.defLoc;
        }
    };
    using DefLocations = QList<DefLocation>;
    struct State {
        using Ptr = std::shared_ptr<State>;

        void insertSorted(Symbol *decl, const Link &link) {
            DefLocations &dl = defLocations[link.targetFilePath];
            DefLocation newElem{decl, link};
            const auto cmp = [](const DefLocation &elem, const DefLocation &value) {
                if (elem.defLoc.targetLine < value.defLoc.targetLine)
                    return true;
                if (elem.defLoc.targetLine > value.defLoc.targetLine)
                    return false;
                return elem.defLoc.targetColumn < value.defLoc.targetColumn;
            };
            dl.insert(std::lower_bound(dl.begin(), dl.end(), newElem, cmp), newElem);
        }

        QList<Symbol *> decls;
        QHash<FilePath, DefLocations> defLocations;
        int remainingFollowSymbolOps = 0;
    };

    void perform() override
    {
        for (Symbol * const decl : std::as_const(m_state->decls)) {
            QTextCursor cursor(currentFile()->document()->begin());
            TranslationUnit * const tu = currentFile()->cppDocument()->translationUnit();
            const int declPos = tu->getTokenPositionInDocument(decl->sourceLocation(),
                                                               currentFile()->document());
            cursor.setPosition(declPos);
            const CursorInEditor cursorInEditor(
                cursor,
                decl->filePath(),
                qobject_cast<CppEditorWidget *>(currentFile()->editor()),
                currentFile()->editor()->textDocument(),
                currentFile()->cppDocument());

            const auto callback = [decl, declPos, doc = cursor.document(), state = m_state](
                                      const Link &link) {
                class FinishedChecker
                {
                public:
                    FinishedChecker(const State::Ptr &state) : m_state(state)
                    {}
                    ~FinishedChecker()
                    {
                        if (--m_state->remainingFollowSymbolOps == 0)
                            finish(m_state);
                    };
                private:
                    const State::Ptr &m_state;
                } finishedChecker(state);

                if (!link.hasValidTarget())
                    return;
                if (decl->filePath() == link.targetFilePath) {
                    const int linkPos = Text::positionInText(doc, link.targetLine,
                                                             link.targetColumn + 1);
                    if (linkPos == declPos)
                        return;
                }
                state->insertSorted(decl, link);
            };

            ++m_state->remainingFollowSymbolOps;

            // Force queued execution, as the built-in editor can run the callback synchronously.
            const auto followSymbol = [cursorInEditor, callback] {
                CppModelManager::followSymbol(
                    cursorInEditor, callback, true, false, FollowSymbolMode::Exact);
            };
            QMetaObject::invokeMethod(CppModelManager::instance(), followSymbol, Qt::QueuedConnection);
        }
    }

    // TODO: Move to some central place for re-use
    static CppEditorWidget *getEditorWidget(const FilePath &filePath)
    {
        CppEditorWidget *editorWidget = nullptr;
        const QList<IEditor *> editors = DocumentModel::editorsForFilePath(filePath);
        for (IEditor *editor : editors) {
            const auto textEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
            if (textEditor)
                editorWidget = qobject_cast<CppEditorWidget *>(textEditor->editorWidget());
            if (editorWidget)
                return editorWidget;
        }
        return nullptr;
    }

    static void finish(const State::Ptr &state)
    {
        CppRefactoringChanges factory{CppModelManager::snapshot()};

        // TODO: Move to some central place for re-use.
        const auto createRefactoringFile = [&factory](const FilePath &filePath)
        {
            CppEditorWidget * const editorWidget = getEditorWidget(filePath);
            return editorWidget ? factory.file(editorWidget, editorWidget->semanticInfo().doc)
                                : factory.cppFile(filePath);
        };

        const auto findAstRange = [](const CppRefactoringFile &file, const Link &pos) {
            const QList<AST *> astPath = ASTPath(
                file.cppDocument())(pos.targetLine, pos.targetColumn + 1);
            for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
                if (const auto funcDef = (*it)->asFunctionDefinition()) {
                    AST *ast = funcDef;
                    for (auto next = std::next(it);
                         next != astPath.rend() && (*next)->asTemplateDeclaration();
                         ++next) {
                        ast = *next;
                    }
                    return file.range(ast);
                }
            }
            return ChangeSet::Range();
        };

        for (auto it = state->defLocations.cbegin(); it != state->defLocations.cend(); ++it) {
            const DefLocations &defLocsActualOrder = it.value();
            const DefLocations defLocsExpectedOrder = Utils::sorted(
                defLocsActualOrder, [](const DefLocation &loc1, const DefLocation &loc2) {
                    return loc1.decl->sourceLocation() < loc2.decl->sourceLocation();
                });
            if (defLocsExpectedOrder == defLocsActualOrder)
                continue;

            CppRefactoringFilePtr file = createRefactoringFile(it.key());
            ChangeSet changes;
            for (int i = 0; i < defLocsActualOrder.size(); ++i) {
                const DefLocation &actualLoc = defLocsActualOrder[i];
                int expectedPos = -1;
                for (int j = 0; j < defLocsExpectedOrder.size(); ++j) {
                    if (defLocsExpectedOrder[j].decl == actualLoc.decl) {
                        expectedPos = j;
                        break;
                    }
                }
                if (expectedPos == i)
                    continue;
                const ChangeSet::Range actualRange = findAstRange(*file, actualLoc.defLoc);
                const ChangeSet::Range expectedRange
                    = findAstRange(*file, defLocsActualOrder[expectedPos].defLoc);
                if (actualRange.end > actualRange.start && expectedRange.end > expectedRange.start)
                    changes.move(actualRange, expectedRange.start);
            }
            QTC_ASSERT(!changes.hadErrors(), continue);
            file->setChangeSet(changes);
            file->apply();
        }
    }

    const State::Ptr m_state;
};

//! Ensures relative order of member function implementations is the same as declaration order.
class SynchronizeMemberFunctionOrder : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    static QObject *createTest();
#endif

private:
    void doMatch(const CppQuickFixInterface &interface, QuickFixOperations &result) override
    {
        ClassSpecifierAST * const classAst = astForClassOperations(interface);
        if (!classAst || !classAst->symbol)
            return;

        QList<Symbol *> memberFunctions;
        const TranslationUnit * const tu
            = interface.currentFile()->cppDocument()->translationUnit();
        for (int i = 0; i < classAst->symbol->memberCount(); ++i) {
            Symbol *member = classAst->symbol->memberAt(i);

            // Skip macros
            if (tu->tokenAt(member->sourceLocation()).expanded())
                continue;

            if (const auto templ = member->asTemplate())
                member = templ->declaration();
            if (member->type()->asFunctionType() && !member->asFunction())
                memberFunctions << member;
        }
        if (!memberFunctions.isEmpty())
            result << new SynchronizeMemberFunctionOrderOp(interface, memberFunctions);
    }
};

#ifdef WITH_TESTS
using namespace Tests;

class SynchronizeMemberFunctionOrderTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QString>("projectName");
        QTest::addColumn<bool>("expectChanges");

        QTest::newRow("no out-of-line definitions") << "no-out-of-line" << false;
        QTest::newRow("already sorted") << "already-sorted" << false;
        QTest::newRow("different impl locations") << "different-locations" << true;
        QTest::newRow("templates") << "templates" << true;
    }

    void test()
    {
        QFETCH(QString, projectName);
        QFETCH(bool, expectChanges);
        using namespace CppEditor::Tests;
        using namespace ProjectExplorer;
        using namespace TextEditor;

        // Set up project.
        Kit * const kit  = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid() && !k->hasWarning() && k->value("QtSupport.QtInformation").isValid();
        });
        if (!kit)
            QSKIP("The test requires at least one valid kit with a valid Qt");
        const auto projectDir = std::make_unique<TemporaryCopiedDir>(
            ":/cppeditor/testcases/reorder-member-impls/" + projectName);
        SourceFilesRefreshGuard refreshGuard;
        ProjectOpenerAndCloser projectMgr;
        QVERIFY(projectMgr.open(projectDir->absolutePath(projectName + ".pro"), true, kit));
        QVERIFY(refreshGuard.wait());

        // Open header file and locate class.
        const auto headerFilePath = projectDir->absolutePath("header.h");
        QVERIFY2(headerFilePath.exists(), qPrintable(headerFilePath.toUserOutput()));
        const auto editor = qobject_cast<BaseTextEditor *>(EditorManager::openEditor(headerFilePath));
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        QTextCursor classCursor = doc->document()->find("struct S");
        QVERIFY(!classCursor.isNull());
        editor->setCursorPosition(classCursor.position());
        const auto editorWidget = qobject_cast<CppEditorWidget *>(editor->editorWidget());
        QVERIFY(editorWidget);
        QVERIFY(TestCase::waitForRehighlightedSemanticDocument(editorWidget));

        // Query factory.
        SynchronizeMemberFunctionOrder factory;
        CppQuickFixInterface quickFixInterface(editorWidget, ExplicitlyInvoked);
        QuickFixOperations operations;
        factory.match(quickFixInterface, operations);
        operations.first()->perform();
        if (expectChanges)
            QVERIFY(waitForSignalOrTimeout(doc, &IDocument::saved, 30000));
        QTest::qWait(1000);

        // Compare all files.
        const FileFilter filter({"*_expected"}, QDir::Files);
        const FilePaths expectedDocuments = projectDir->filePath().dirEntries(filter);
        QVERIFY(!expectedDocuments.isEmpty());
        for (const FilePath &expected : expectedDocuments) {
            static const QString suffix = "_expected";
            const FilePath actual = expected.parentDir()
                                        .pathAppended(expected.fileName().chopped(suffix.length()));
            QVERIFY(actual.exists());
            const auto actualContents = actual.fileContents();
            QVERIFY(actualContents);
            const auto expectedContents = expected.fileContents();
            const QByteArrayList actualLines = actualContents->split('\n');
            const QByteArrayList expectedLines = expectedContents->split('\n');
            if (actualLines.size() != expectedLines.size()) {
                qDebug().noquote().nospace() << "---\n" << *expectedContents << "EOF";
                qDebug().noquote().nospace() << "+++\n" << *actualContents << "EOF";
            }
            QCOMPARE(actualLines.size(), expectedLines.size());
            for (int i = 0; i < actualLines.size(); ++i) {
                const QByteArray actualLine = actualLines.at(i);
                const QByteArray expectedLine = expectedLines.at(i);
                if (actualLine != expectedLine)
                    qDebug() << "Unexpected content in line" << (i + 1) << "of file"
                             << actual.fileName();
                QCOMPARE(actualLine, expectedLine);
            }
        }
    }
};

QObject *SynchronizeMemberFunctionOrder::createTest()
{
    return new SynchronizeMemberFunctionOrderTest;
}

#endif
} // namespace

void registerSynchronizeMemberFunctionOrderQuickfix()
{
    CppQuickFixFactory::registerFactory<SynchronizeMemberFunctionOrder>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <synchronizememberfunctionorder.moc>
#endif
