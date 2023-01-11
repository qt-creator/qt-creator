// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppquickfix.h"
#include "cpprefactoringchanges.h"

#include <QString>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QFutureWatcher>
#include <QTextCursor>

namespace CppEditor {
class CppEditorWidget;

namespace Internal {
class FunctionDeclDefLink;

class FunctionDeclDefLinkFinder : public QObject
{
    Q_OBJECT
public:
    FunctionDeclDefLinkFinder(QObject *parent = nullptr);

    void startFindLinkAt(QTextCursor cursor,
                    const CPlusPlus::Document::Ptr &doc,
                    const CPlusPlus::Snapshot &snapshot);

    QTextCursor scannedSelection() const;

signals:
    void foundLink(QSharedPointer<FunctionDeclDefLink> link);

private:
    void onFutureDone();

    QTextCursor m_scannedSelection;
    QTextCursor m_nameSelection;
    QScopedPointer<QFutureWatcher<QSharedPointer<FunctionDeclDefLink> > > m_watcher;
};

class FunctionDeclDefLink
{
    Q_DISABLE_COPY(FunctionDeclDefLink)
    FunctionDeclDefLink() = default;
public:
    bool isValid() const;
    bool isMarkerVisible() const;

    void apply(CppEditorWidget *editor, bool jumpToMatch);
    void hideMarker(CppEditorWidget *editor);
    void showMarker(CppEditorWidget *editor);
    Utils::ChangeSet changes(const CPlusPlus::Snapshot &snapshot, int targetOffset = -1);

    QTextCursor linkSelection;

    // stored to allow aborting when the name is changed
    QTextCursor nameSelection;
    QString nameInitial;

    // The 'source' prefix denotes information about the original state
    // of the function before the user did any edits.
    CPlusPlus::Document::Ptr sourceDocument;
    CPlusPlus::Function *sourceFunction = nullptr;
    CPlusPlus::DeclarationAST *sourceDeclaration = nullptr;
    CPlusPlus::FunctionDeclaratorAST *sourceFunctionDeclarator = nullptr;

    // The 'target' prefix denotes information about the remote declaration matching
    // the 'source' declaration, where we will try to apply the user changes.
    // 1-based line and column
    int targetLine = 0;
    int targetColumn = 0;
    QString targetInitial;

    CppRefactoringFileConstPtr targetFile;
    CPlusPlus::Function *targetFunction = nullptr;
    CPlusPlus::DeclarationAST *targetDeclaration = nullptr;
    CPlusPlus::FunctionDeclaratorAST *targetFunctionDeclarator = nullptr;

private:
    QString normalizedInitialName() const;

    bool hasMarker = false;

    friend class FunctionDeclDefLinkFinder;
};

} // namespace Internal
} // namespace CppEditor
