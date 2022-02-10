/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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

#pragma once

#include <cppeditor/cpptoolstestcase.h>
#include <coreplugin/find/searchresultitem.h>
#include <texteditor/blockrange.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/semantichighlighter.h>
#include <utils/fileutils.h>

#include <QHash>
#include <QObject>
#include <QSet>
#include <QStringList>

namespace ProjectExplorer {
class Kit;
class Project;
}
namespace TextEditor { class TextDocument; }

namespace ClangCodeModel {
namespace Internal {
class ClangdClient;
namespace Tests {

class ClangdTest : public QObject
{
    Q_OBJECT
public:
    ~ClangdTest();

protected:
    // Convention: base bame == name of parent dir
    void setProjectFileName(const QString &fileName) { m_projectFileName = fileName; }

    void setSourceFileNames(const QStringList &fileNames) { m_sourceFileNames = fileNames; }
    void setMinimumVersion(int version) { m_minVersion = version; }

    ClangdClient *client() const { return m_client; }
    Utils::FilePath filePath(const QString &fileName) const;
    TextEditor::TextDocument *document(const QString &fileName) const {
        return m_sourceDocuments.value(fileName);
    }
    ProjectExplorer::Project *project() const { return m_project; }
    void waitForNewClient(bool withIndex = true);

protected slots:
    virtual void initTestCase();

private:
    CppEditor::Tests::TemporaryCopiedDir *m_projectDir = nullptr;
    QString m_projectFileName;
    QStringList m_sourceFileNames;
    QHash<QString, TextEditor::TextDocument *> m_sourceDocuments;
    ProjectExplorer::Kit *m_kit = nullptr;
    ProjectExplorer::Project *m_project = nullptr;
    ClangdClient *m_client = nullptr;
    int m_minVersion = -1;
};

class ClangdTestFindReferences : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestFindReferences();

private slots:
    void initTestCase() override;
    void init() { m_actualResults.clear(); }
    void test_data();
    void test();

private:
    QList<Core::SearchResultItem> m_actualResults;
};

class ClangdTestFollowSymbol : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestFollowSymbol();

private slots:
    void test_data();
    void test();
};

class ClangdTestLocalReferences : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestLocalReferences();

private slots:
    void test_data();
    void test();
};

class ClangdTestTooltips : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestTooltips();

private slots:
    void test_data();
    void test();
};

class ClangdTestHighlighting : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestHighlighting();

private slots:
    void initTestCase() override;
    void test_data();
    void test();
    void testIfdefedOutBlocks();

private:
    TextEditor::HighlightingResults m_results;
    QList<TextEditor::BlockRange> m_ifdefedOutBlocks;
};

class ClangdTestCompletion : public ClangdTest
{
    Q_OBJECT
public:
    ClangdTestCompletion();

private slots:
    void initTestCase() override;

    void testCompleteDoxygenKeywords();
    void testCompletePreprocessorKeywords();
    void testCompleteIncludeDirective();

    void testCompleteGlobals();
    void testCompleteMembers();
    void testCompleteMembersFromInside();
    void testCompleteMembersFromOutside();
    void testCompleteMembersFromFriend();
    void testFunctionAddress();
    void testFunctionHints();
    void testFunctionHintsFiltered();
    void testFunctionHintConstructor();
    void testCompleteClassAndConstructor();
    void testCompletePrivateFunctionDefinition();

    void testCompleteWithDotToArrowCorrection();
    void testDontCompleteWithDotToArrowCorrectionForFloats();

    void testCompleteCodeInGeneratedUiFile();

    void testSignalCompletion_data();
    void testSignalCompletion();

    void testCompleteAfterProjectChange();

private:
    void startCollectingHighlightingInfo();
    void getProposal(const QString &fileName, TextEditor::ProposalModelPtr &proposalModel,
                     const QString &insertString = {}, int *cursorPos = nullptr);
    static bool hasItem(TextEditor::ProposalModelPtr model, const QString &text,
                        const QString &detail = {});
    static bool hasSnippet(TextEditor::ProposalModelPtr model, const QString &text);
    static int itemsWithText(TextEditor::ProposalModelPtr model, const QString &text);
    static TextEditor::AssistProposalItemInterface *getItem(
            TextEditor::ProposalModelPtr model, const QString &text, const QString &detail = {});

    QSet<Utils::FilePath> m_documentsWithHighlighting;
};

class ClangdTestExternalChanges : public ClangdTest
{
    Q_OBJECT

public:
    ClangdTestExternalChanges();

private slots:
    void test();
};

} // namespace Tests
} // namespace Internal
} // namespace ClangCodeModel

