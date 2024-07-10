// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "moveclasstoownfile.h"

#include "../cppeditortr.h"
#include "../cppeditorwidget.h"
#include "../cppfilesettingspage.h"
#include "../cpprefactoringchanges.h"
#include "cppquickfix.h"
#include "cppquickfixhelpers.h"

#include <coreplugin/messagemanager.h>
#include <cplusplus/ASTPath.h>
#include <cplusplus/Overview.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectnodes.h>
#include <utils/codegeneration.h>
#include <utils/layoutbuilder.h>
#include <utils/treemodel.h>
#include <utils/treeviewcombobox.h>

#include <QCheckBox>
#include <QDialog>
#include <QDialogButtonBox>

#ifdef WITH_TESTS
#include "../cpptoolstestcase.h"
#include <coreplugin/editormanager/editormanager.h>
#include <projectexplorer/kitmanager.h>
#include <texteditor/textdocument.h>
#include <QtTest>
#endif // WITH_TESTS

using namespace CPlusPlus;
using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor::Internal {
namespace {

class MoveClassToOwnFileOp : public CppQuickFixOperation
{
public:
    MoveClassToOwnFileOp(
        const CppQuickFixInterface &interface,
        AST *fullDecl,
        ClassSpecifierAST *classAst,
        const QList<Namespace *> &namespacePath,
        bool interactive)
        : CppQuickFixOperation(interface)
        , m_state(std::make_shared<State>())
    {
        setDescription(Tr::tr("Move Class to a Dedicated Set of Source Files"));
        m_state->originalFilePath = interface.currentFile()->filePath();
        m_state->classAst = classAst;
        m_state->namespacePath = namespacePath;
        m_state->interactive = interactive;
        PerFileState &perFileState = m_state->perFileState[interface.currentFile()->filePath()];
        perFileState.refactoringFile = interface.currentFile();
        perFileState.declarationsToMove << fullDecl;
    }

private:
    struct PerFileState {
        // We want to keep the relative order of moved code.
        void insertSorted(AST *decl) {
            declarationsToMove.insert(std::lower_bound(
                                          declarationsToMove.begin(),
                                          declarationsToMove.end(),
                                          decl,
                                          [](const AST *elem, const AST *value) {
                                              return elem->firstToken() < value->firstToken();
                                          }), decl);
        }

        CppRefactoringFilePtr refactoringFile;
        QList<AST *> declarationsToMove;
    };
    struct State {
        using Ptr = std::shared_ptr<State>;

        FilePath originalFilePath;
        AST *fullDecl = nullptr;
        ClassSpecifierAST *classAst = nullptr;
        QList<Namespace *> namespacePath;
        Links lookupResults;
        QMap<FilePath, PerFileState> perFileState; // A map for deterministic order of moved code.
        CppRefactoringChanges factory{CppModelManager::snapshot()};
        int remainingFollowSymbolOps = 0;
        bool interactive = true;
    };
    class Dialog : public QDialog {
    public:
        Dialog(const FilePath &defaultHeaderFilePath, const FilePath &defaultSourceFilePath,
               ProjectNode *defaultProjectNode)
            : m_buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel)
        {
            ProjectNode * const rootNode = defaultProjectNode
                                              ? defaultProjectNode->getProject()->rootProjectNode()
                                              : nullptr;
            if (rootNode) {
                const auto projectRootItem = new NodeItem(rootNode);
                buildTree(projectRootItem);
                m_projectModel.rootItem()->appendChild(projectRootItem);
            }
            m_projectNodeComboBox.setModel(&m_projectModel);
            if (defaultProjectNode) {
                const auto matcher = [defaultProjectNode](TreeItem *item) {
                    return static_cast<NodeItem *>(item)->node == defaultProjectNode;
                };
                TreeItem * const defaultItem = m_projectModel.rootItem()->findAnyChild(matcher);
                if (defaultItem ) {
                    QModelIndex index = m_projectModel.indexForItem(defaultItem);
                    m_projectNodeComboBox.setCurrentIndex(index);
                    while (index.isValid()) {
                        m_projectNodeComboBox.view()->expand(index);
                        index = index.parent();
                    }
                }

            }
            connect(&m_projectNodeComboBox, &QComboBox::currentIndexChanged,
                    this, [this] {
                        if (m_filesEdited)
                            return;
                        const auto newProjectNode = projectNode();
                        QTC_ASSERT(newProjectNode, return);
                        const FilePath baseDir = newProjectNode->directory();
                        m_sourcePathChooser.setFilePath(
                            baseDir.pathAppended(sourceFilePath().fileName()));
                        m_headerPathChooser.setFilePath(
                            baseDir.pathAppended(headerFilePath().fileName()));
                        m_filesEdited = false;
                    });

            m_headerOnlyCheckBox.setText(Tr::tr("Header file only"));
            m_headerOnlyCheckBox.setChecked(false);
            connect(&m_headerOnlyCheckBox, &QCheckBox::toggled,
                    this, [this](bool checked) { m_sourcePathChooser.setEnabled(!checked); });

            m_headerPathChooser.setExpectedKind(PathChooser::SaveFile);
            m_sourcePathChooser.setExpectedKind(PathChooser::SaveFile);
            m_headerPathChooser.setFilePath(defaultHeaderFilePath);
            m_sourcePathChooser.setFilePath(defaultSourceFilePath);
            connect(&m_headerPathChooser, &PathChooser::textChanged,
                    this, [this] { m_filesEdited = true; });
            connect(&m_sourcePathChooser, &PathChooser::textChanged,
                    this, [this] { m_filesEdited = true; });

            connect(&m_buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
            connect(&m_buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

            using namespace Layouting;
            Column {
                Form {
                    Tr::tr("Project:"), &m_projectNodeComboBox, br,
                    &m_headerOnlyCheckBox, br,
                    Tr::tr("Header file:"), &m_headerPathChooser, br,
                    Tr::tr("Implementation file:"), &m_sourcePathChooser, br,
                },
                &m_buttonBox
            }.attachTo(this);
        }

        ProjectNode *projectNode() const
        {
            const QVariant v = m_projectNodeComboBox.currentData(Qt::UserRole);
            return v.isNull() ? nullptr : static_cast<ProjectNode *>(v.value<void *>());
        }
        bool createSourceFile() const { return !m_headerOnlyCheckBox.isChecked(); }
        FilePath headerFilePath() const { return m_headerPathChooser.absoluteFilePath(); }
        FilePath sourceFilePath() const { return m_sourcePathChooser.absoluteFilePath(); }

    private:
        struct NodeItem : public StaticTreeItem {
            NodeItem(ProjectNode *node)
                : StaticTreeItem({node->displayName()}, {node->directory().toUserOutput()})
                , node(node)
            {}
            Qt::ItemFlags flags(int) const override
            {
                return Qt::ItemIsEnabled | Qt::ItemIsSelectable;
            }
            QVariant data(int column, int role) const override
            {
                if (role == Qt::UserRole)
                    return QVariant::fromValue(static_cast<void *>(node));
                return StaticTreeItem::data(column, role);
            }

            ProjectNode * const node;
        };

        void buildTree(NodeItem *parent)
        {
            for (Node * const node : parent->node->nodes()) {
                if (const auto projNode = node->asProjectNode()) {
                    const auto child = new NodeItem(projNode);
                    buildTree(child);
                    parent->appendChild(child);
                }
            }
        }

        TreeViewComboBox m_projectNodeComboBox;
        QCheckBox m_headerOnlyCheckBox;
        PathChooser m_headerPathChooser;
        PathChooser m_sourcePathChooser;
        QDialogButtonBox m_buttonBox;
        TreeModel<> m_projectModel;
        bool m_filesEdited = false;
    };

    void perform() override
    {
        collectImplementations(m_state->classAst->symbol, m_state);
        if (m_state->remainingFollowSymbolOps == 0)
            finish(m_state);
    }

    static CppRefactoringFilePtr getRefactoringFile(const FilePath &filePath, const State::Ptr &state)
    {
        CppRefactoringFilePtr &refactoringFile = state->perFileState[filePath].refactoringFile;
        if (!refactoringFile)
            refactoringFile = state->factory.cppFile(filePath);
        return refactoringFile;
    }

    static void lookupSymbol(Symbol *symbol, const State::Ptr &state)
    {
        const CppRefactoringFilePtr refactoringFile = getRefactoringFile(symbol->filePath(), state);
        const auto editorWidget = qobject_cast<CppEditorWidget *>(refactoringFile->editor());
        QTextCursor cursor(refactoringFile->document()->begin());
        TranslationUnit * const tu = refactoringFile->cppDocument()->translationUnit();
        const int symbolPos = tu->getTokenPositionInDocument(symbol->sourceLocation(),
                                                             refactoringFile->document());
        cursor.setPosition(symbolPos);
        const CursorInEditor cursorInEditor(
            cursor,
            symbol->filePath(),
            editorWidget,
            editorWidget ? editorWidget->textDocument() : nullptr,
            refactoringFile->cppDocument());
        const auto callback = [symbol, symbolPos, doc = cursor.document(), state](const Link &link) {
            class FinishedChecker {
            public:
                FinishedChecker(const State::Ptr &state) : m_state(state) {}
                ~FinishedChecker() {
                    if (--m_state->remainingFollowSymbolOps == 0)
                        finish(m_state);
                };
            private:
                const State::Ptr &m_state;
            } finishedChecker(state);
            if (!link.hasValidTarget())
                return;
            if (symbol->filePath() == link.targetFilePath) {
                const int linkPos = Text::positionInText(doc, link.targetLine,
                                                         link.targetColumn + 1);
                if (linkPos == symbolPos)
                    return;
            }
            const CppRefactoringFilePtr refactoringFile
                = getRefactoringFile(link.targetFilePath, state);
            const QList<AST *> astPath = ASTPath(
                refactoringFile->cppDocument())(link.targetLine, link.targetColumn);
            const bool isTemplate = symbol->asTemplate();
            const bool isFunction = symbol->type()->asFunctionType();
            for (auto it = astPath.rbegin(); it != astPath.rend(); ++it) {
                const bool match = isTemplate ? bool((*it)->asTemplateDeclaration())
                                   : isFunction ? bool((*it)->asFunctionDefinition())
                                                : bool((*it)->asSimpleDeclaration());
                if (match) {
                    // For member functions of class templates.
                    if (isFunction) {
                        const auto next = std::next(it);
                        if (next != astPath.rend() && (*next)->asTemplateDeclaration())
                            it = next;
                    }
                    state->perFileState[link.targetFilePath].insertSorted(*it);
                    if (symbol->asForwardClassDeclaration()) {
                        if (const auto classSpec = (*(it - 1))->asClassSpecifier();
                            classSpec && classSpec->symbol) {
                            collectImplementations(classSpec->symbol, state);
                        }
                    }
                    break;
                }
            }
        };
        ++state->remainingFollowSymbolOps;

        // Force queued execution, as the built-in editor can run the callback synchronously.
        const auto followSymbol = [cursorInEditor, callback] {
            NonInteractiveFollowSymbolMarker niMarker;
            CppModelManager::followSymbol(
                cursorInEditor, callback, true, false, FollowSymbolMode::Exact);
        };
        QMetaObject::invokeMethod(CppModelManager::instance(), followSymbol, Qt::QueuedConnection);
    }

    static void collectImplementations(Class *klass, const State::Ptr &state)
    {
        for (int i = 0; i < klass->memberCount(); ++i) {
            Symbol * const member = klass->memberAt(i);
            if (member->asForwardClassDeclaration() || member->asTemplate()) {
                lookupSymbol(member, state);
                continue;
            }
            const auto decl = member->asDeclaration();
            if (!decl)
                continue;
            if (decl->type().type()->asFunctionType()) {
                if (!decl->asFunction())
                    lookupSymbol(member, state);
            } else if (decl->isStatic() && !decl->type().isInline()) {
                lookupSymbol(member, state);
            }
        }
    }

    static void finish(const State::Ptr &state)
    {
        Overview ov;
        Project * const project = ProjectManager::projectForFile(state->originalFilePath);
        const CppFileSettings fileSettings = cppFileSettingsForProject(project);
        const auto constructDefaultFilePaths = [&] {
            const QString className = ov.prettyName(state->classAst->symbol->name());
            const QString baseFileName = fileSettings.lowerCaseFiles ? className.toLower() : className;
            const QString headerFileName = baseFileName + '.' + fileSettings.headerSuffix;
            const FilePath baseDir = state->originalFilePath.parentDir();
            const FilePath headerFilePath = baseDir.pathAppended(headerFileName);
            const QString sourceFileName = baseFileName + '.' + fileSettings.sourceSuffix;
            const FilePath sourceFilePath = baseDir.pathAppended(sourceFileName);
            return std::make_pair(headerFilePath, sourceFilePath);
        };
        auto [headerFilePath, sourceFilePath] = constructDefaultFilePaths();
        bool mustCreateSourceFile = false;
        bool mustNotCreateSourceFile = false;
        ProjectNode *projectNode = nullptr;
        if (project && project->rootProjectNode()) {
            const Node * const origNode = project->nodeForFilePath(state->originalFilePath);
            if (origNode)
                projectNode = const_cast<Node *>(origNode)->managingProject();
        }
        if (state->interactive) {
            Dialog dlg(headerFilePath, sourceFilePath, projectNode);
            if (dlg.exec() != QDialog::Accepted)
                return;
            projectNode = dlg.projectNode();
            headerFilePath = dlg.headerFilePath();
            sourceFilePath = dlg.sourceFilePath();
            mustCreateSourceFile = dlg.createSourceFile();
            mustNotCreateSourceFile = !dlg.createSourceFile();
        }
        const auto fileListForDisplay = [](const FilePaths &files) {
            return Utils::transform<QStringList>(files, [](const FilePath &fp) {
                       return '"' + fp.toUserOutput() + '"';
                   }).join(", ");
        };
        FilePaths existingFiles;
        if (headerFilePath.exists())
            existingFiles << headerFilePath;
        if (!mustNotCreateSourceFile && sourceFilePath.exists())
            existingFiles << sourceFilePath;
        if (!existingFiles.isEmpty()) {
            MessageManager::writeDisrupting(
                Tr::tr("Refusing to overwrite the following files: %1\n")
                    .arg(fileListForDisplay(existingFiles)));
            return;
        }
        const QString headerFileName = headerFilePath.fileName();

        QString headerContent;
        QString sourceContent;
        QList<QString *> commonContent{&headerContent};
        if (!mustNotCreateSourceFile)
            commonContent << &sourceContent;
        for (QString *const content : std::as_const(commonContent)) {
            content->append(fileSettings.licenseTemplate());
            if (!content->isEmpty())
                content->append('\n');
        }
        sourceContent.append('\n').append("#include \"").append(headerFileName).append("\"\n");
        const QStringList namespaceNames
            = Utils::transform<QStringList>(state->namespacePath, [&](const Namespace *ns) {
                  return ov.prettyName(ns->name());
              });
        const QString headerGuard = fileSettings.headerGuard(headerFilePath);
        if (fileSettings.headerPragmaOnce) {
            headerContent.append("#pragma once\n");
        } else {
            headerContent.append("#ifndef " + headerGuard + "\n");
            headerContent.append("#define " + headerGuard + "\n");
        }
        if (!namespaceNames.isEmpty()) {
            for (QString *const content : std::as_const(commonContent)) {
                content->append('\n');
                for (const QString &ns : namespaceNames)
                    content->append("namespace " + ns + " {\n");
            }
        }
        bool hasSourceContent = false;
        for (auto it = state->perFileState.begin(); it != state->perFileState.end(); ++it) {
            if (it->declarationsToMove.isEmpty())
                continue;
            const CppRefactoringFilePtr refactoringFile = it->refactoringFile;
            QTC_ASSERT(refactoringFile, continue);
            const bool isDeclFile = refactoringFile->filePath() == state->originalFilePath;
            ChangeSet changes;
            if (isDeclFile) {
                QString relInclude = headerFilePath.relativePathFrom(
                                                       refactoringFile->filePath().parentDir()).toString();
                if (!relInclude.isEmpty())
                    relInclude.append('/');
                relInclude.append('"').append(headerFileName).append('"');
                insertNewIncludeDirective(relInclude, refactoringFile,
                                          refactoringFile->cppDocument(), changes);
            }
            for (AST * const declToMove : std::as_const(it->declarationsToMove)) {
                const ChangeSet::Range rangeToMove = refactoringFile->range(declToMove);
                QString &content = isDeclFile || mustNotCreateSourceFile ? headerContent
                                                                         : sourceContent;
                if (&content == &sourceContent)
                    hasSourceContent = true;
                content.append('\n')
                    .append(refactoringFile->textOf(rangeToMove))
                    .append('\n');
                changes.remove(rangeToMove);
            }
            refactoringFile->apply(changes);
        }

        if (!namespaceNames.isEmpty()) {
            for (QString *const content : std::as_const(commonContent)) {
                content->append('\n');
                for (auto it = namespaceNames.rbegin(); it != namespaceNames.rend(); ++it)
                    content->append("} // namespace " + *it + '\n');
            }
        }
        if (!fileSettings.headerPragmaOnce)
            headerContent.append("\n#endif // " + headerGuard + '\n');

        headerFilePath.ensureExistingFile();
        state->factory.cppFile(headerFilePath)->apply(ChangeSet::makeInsert(0, headerContent));
        if (hasSourceContent || mustCreateSourceFile) {
            sourceFilePath.ensureExistingFile();
            state->factory.cppFile(sourceFilePath)->apply(ChangeSet::makeInsert(0, sourceContent));
        }

        if (!projectNode)
            return;
        FilePaths toAdd{headerFilePath};
        if (hasSourceContent)
            toAdd << sourceFilePath;
        FilePaths notAdded;
        projectNode->addFiles(toAdd, &notAdded);
        if (!notAdded.isEmpty()) {
            MessageManager::writeDisrupting(
                Tr::tr("Failed to add to project file \"%1\": %2")
                    .arg(projectNode->filePath().toUserOutput(), fileListForDisplay(notAdded)));
        }

        if (state->interactive)
            EditorManager::openEditor(headerFilePath);
    }

    const State::Ptr m_state;
};

//! Move a class into a dedicates set of files.
class MoveClassToOwnFile : public CppQuickFixFactory
{
#ifdef WITH_TESTS
public:
    void setNonInteractive() { m_interactive = false; }
    static QObject *createTest();
#endif

private:
    // Applies if and only if:
    //   - Class is not a nested class.
    //   - Class name does not match file name via any of the usual transformations.
    //   - There are other declarations in the same file.
    void doMatch(const CppQuickFixInterface &interface,
                 TextEditor::QuickFixOperations &result) override
    {
        ClassSpecifierAST * const classAst = astForClassOperations(interface);
        if (!classAst || !classAst->symbol)
            return;
        AST *fullDecl = nullptr;
        for (auto it = interface.path().rbegin(); it != interface.path().rend() && !fullDecl; ++it) {
            if (*it == classAst && it != interface.path().rend() - 1) {
                auto next = std::next(it);
                fullDecl = (*next)->asSimpleDeclaration();
                if (next != interface.path().rend() - 1) {
                    next = std::next(next);
                    if (const auto templ = (*next)->asTemplateDeclaration())
                        fullDecl = templ;
                }
            }
        }
        if (!fullDecl)
            return;

        // Check file name.
        const QString className = Overview().prettyName(classAst->symbol->name());
        if (className.isEmpty())
            return;
        const QString lowerFileBaseName = interface.filePath().baseName().toLower();
        if (lowerFileBaseName.contains(className.toLower()))
            return;
        QString underscoredClassName = className;
        QChar curChar = underscoredClassName.at(0);
        for (int i = 1; i < underscoredClassName.size(); ++i) {
            const QChar prevChar = curChar;
            curChar = underscoredClassName.at(i);
            if (curChar.isUpper() && prevChar.isLetterOrNumber() && !prevChar.isUpper()) {
                underscoredClassName.insert(i, '_');
                ++i;
            }
        }
        if (lowerFileBaseName.contains(underscoredClassName.toLower()))
            return;

        // Is there more than one class definition in the file?
        AST * const ast = interface.currentFile()->cppDocument()->translationUnit()->ast();
        if (!ast)
            return;
        DeclarationListAST * const topLevelDecls = ast->asTranslationUnit()->declaration_list;
        if (!topLevelDecls)
            return;
        QList<Namespace *> namespacePath;
        QList<Namespace *> currentNamespacePath;
        bool foundOtherDecls = false;
        bool foundSelf = false;
        std::function<void(Namespace *)> collectSymbolsFromNamespace;
        const auto handleSymbol = [&](Symbol *symbol) {
            if (!symbol)
                return;
            if (const auto nsMember = symbol->asNamespace()) {
                collectSymbolsFromNamespace(nsMember);
                return;
            }
            if (symbol != classAst->symbol) {
                if (!symbol->asForwardClassDeclaration())
                    foundOtherDecls = true;
                return;
            }
            QTC_ASSERT(symbol->asClass(), return);
            foundSelf = true;
            namespacePath = currentNamespacePath;
        };
        collectSymbolsFromNamespace = [&](Namespace *ns) {
            currentNamespacePath << ns;
            for (int i = 0; i < ns->memberCount() && (!foundSelf || !foundOtherDecls); ++i)
                handleSymbol(ns->memberAt(i));
            currentNamespacePath.removeLast();
        };
        for (DeclarationListAST *it = topLevelDecls; it && (!foundSelf || !foundOtherDecls);
             it = it->next) {
            DeclarationAST *decl = it->value;
            if (!decl)
                continue;
            if (const auto templ = decl->asTemplateDeclaration())
                decl = templ->declaration;
            if (!decl)
                continue;
            if (const auto ns = decl->asNamespace(); ns && ns->symbol) {
                collectSymbolsFromNamespace(ns->symbol);
                continue;
            }
            if (const auto simpleDecl = decl->asSimpleDeclaration()) {
                if (!simpleDecl->decl_specifier_list)
                    continue;
                for (SpecifierListAST *spec = simpleDecl->decl_specifier_list; spec; spec = spec->next) {
                    if (!spec->value)
                        continue;
                    if (const auto klass = spec->value->asClassSpecifier())
                        handleSymbol(klass->symbol);
                    else if (!spec->value->asElaboratedTypeSpecifier()) // forward decl
                        foundOtherDecls = true;
                }
            } else if (decl->asDeclaration()) {
                foundOtherDecls = true;
            }
        }

        if (foundSelf && foundOtherDecls) {
            result << new MoveClassToOwnFileOp(
                interface, fullDecl, classAst, namespacePath, m_interactive);
        }
    }

    bool m_interactive = true;
};

#ifdef WITH_TESTS
class MoveClassToOwnFileTest : public QObject
{
    Q_OBJECT

private slots:
    void test_data()
    {
        QTest::addColumn<QString>("projectName");
        QTest::addColumn<QString>("fileName");
        QTest::addColumn<QString>("className");
        QTest::addColumn<bool>("applicable");

        QTest::newRow("nested") << "nested" << "main.cpp" << "Inner" << false;
        QTest::newRow("file name match 1") << "match1" << "TheClass.h" << "TheClass" << false;
        QTest::newRow("file name match 2") << "match2" << "theclass.h" << "TheClass" << false;
        QTest::newRow("file name match 3") << "match3" << "the_class.h" << "TheClass" << false;
        QTest::newRow("single") << "single" << "theheader.h" << "TheClass" << false;
        QTest::newRow("complex") << "complex" << "theheader.h" << "TheClass" << true;
        QTest::newRow("header only") << "header-only" << "theheader.h" << "TheClass" << true;
        QTest::newRow("decl in source file") << "decl-in-source" << "thesource.cpp" << "TheClass" << true;
        QTest::newRow("template") << "template" << "theheader.h" << "TheClass" << true;
    }

    void test()
    {
        QFETCH(QString, projectName);
        QFETCH(QString, fileName);
        QFETCH(QString, className);
        QFETCH(bool, applicable);
        using namespace CppEditor::Tests;
        using namespace TextEditor;

        // Set up project.
        Kit * const kit  = Utils::findOr(KitManager::kits(), nullptr, [](const Kit *k) {
            return k->isValid() && !k->hasWarning() && k->value("QtSupport.QtInformation").isValid();
        });
        if (!kit)
            QSKIP("The test requires at least one valid kit with a valid Qt");
        const auto projectDir = std::make_unique<TemporaryCopiedDir>(
            ":/cppeditor/testcases/move-class/" + projectName);
        SourceFilesRefreshGuard refreshGuard;
        ProjectOpenerAndCloser projectMgr;
        QVERIFY(projectMgr.open(projectDir->absolutePath(projectName + ".pro"), true, kit));
        QVERIFY(refreshGuard.wait());

        // Open header file and locate class.
        const auto headerFilePath = projectDir->absolutePath(fileName);
        QVERIFY2(headerFilePath.exists(), qPrintable(headerFilePath.toUserOutput()));
        const auto editor = qobject_cast<BaseTextEditor *>(EditorManager::openEditor(headerFilePath));
        QVERIFY(editor);
        const auto doc = qobject_cast<TextEditor::TextDocument *>(editor->document());
        QVERIFY(doc);
        QTextCursor classCursor = doc->document()->find("class " + className);
        QVERIFY(!classCursor.isNull());
        editor->setCursorPosition(classCursor.position());
        const auto editorWidget = qobject_cast<CppEditorWidget *>(editor->editorWidget());
        QVERIFY(editorWidget);
        QVERIFY(TestCase::waitForRehighlightedSemanticDocument(editorWidget));

        // Query factory.
        MoveClassToOwnFile factory;
        factory.setNonInteractive();
        CppQuickFixInterface quickFixInterface(editorWidget, ExplicitlyInvoked);
        QuickFixOperations operations;
        factory.match(quickFixInterface, operations);
        QCOMPARE(operations.isEmpty(), !applicable);
        if (!applicable)
            return;
        operations.first()->perform();
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

QObject *MoveClassToOwnFile::createTest()
{
    return new MoveClassToOwnFileTest;
}

#endif // WITH_TESTS

} // namespace

void registerMoveClassToOwnFileQuickfix()
{
    CppQuickFixFactory::registerFactory<MoveClassToOwnFile>();
}

} // namespace CppEditor::Internal

#ifdef WITH_TESTS
#include <moveclasstoownfile.moc>
#endif
