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

#pragma once

#include "cppquickfix.h"

#include <cpptools/cpprefactoringchanges.h>

#include <QString>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QFutureWatcher>
#include <QTextCursor>

namespace CppEditor {
namespace Internal {

class CppEditorWidget;
class FunctionDeclDefLink;

class FunctionDeclDefLinkFinder : public QObject
{
    Q_OBJECT
public:
    FunctionDeclDefLinkFinder(QObject *parent = 0);

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
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::FunctionDeclDefLink)
    Q_DISABLE_COPY(FunctionDeclDefLink)
    FunctionDeclDefLink() = default;
public:
    class Marker {};

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
    unsigned targetLine = 0;
    unsigned targetColumn = 0;
    QString targetInitial;

    CppTools::CppRefactoringFileConstPtr targetFile;
    CPlusPlus::Function *targetFunction = nullptr;
    CPlusPlus::DeclarationAST *targetDeclaration = nullptr;
    CPlusPlus::FunctionDeclaratorAST *targetFunctionDeclarator = nullptr;

private:
    bool hasMarker = false;

    friend class FunctionDeclDefLinkFinder;
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::FunctionDeclDefLink::Marker)
