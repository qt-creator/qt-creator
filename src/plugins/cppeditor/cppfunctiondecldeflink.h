/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPFUNCTIONDECLDEFLINK_H
#define CPPFUNCTIONDECLDEFLINK_H

#include "cppquickfix.h"

#include <cplusplus/CppDocument.h>
#include <cplusplus/ASTfwd.h>

#include <cpptools/cpprefactoringchanges.h>
#include <utils/changeset.h>

#include <QString>
#include <QCoreApplication>
#include <QSharedPointer>
#include <QFutureWatcher>
#include <QTextCursor>

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
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

private slots:
    void onFutureDone();

private:
    QTextCursor m_scannedSelection;
    QTextCursor m_nameSelection;
    QFutureWatcher<QSharedPointer<FunctionDeclDefLink> > m_watcher;
};

class FunctionDeclDefLink
{
    Q_DECLARE_TR_FUNCTIONS(CppEditor::Internal::FunctionDeclDefLink)
    Q_DISABLE_COPY(FunctionDeclDefLink)
public:
    ~FunctionDeclDefLink();

    class Marker {};

    bool isValid() const;
    bool isMarkerVisible() const;

    void apply(CPPEditorWidget *editor, bool jumpToMatch);
    void hideMarker(CPPEditorWidget *editor);
    void showMarker(CPPEditorWidget *editor);
    Utils::ChangeSet changes(const CPlusPlus::Snapshot &snapshot, int targetOffset = -1);

    QTextCursor linkSelection;

    // stored to allow aborting when the name is changed
    QTextCursor nameSelection;
    QString nameInitial;

    // The 'source' prefix denotes information about the original state
    // of the function before the user did any edits.
    CPlusPlus::Document::Ptr sourceDocument;
    CPlusPlus::Function *sourceFunction;
    CPlusPlus::DeclarationAST *sourceDeclaration;
    CPlusPlus::FunctionDeclaratorAST *sourceFunctionDeclarator;

    // The 'target' prefix denotes information about the remote declaration matching
    // the 'source' declaration, where we will try to apply the user changes.
    // 1-based line and column
    unsigned targetLine;
    unsigned targetColumn;
    QString targetInitial;

    CppTools::CppRefactoringFileConstPtr targetFile;
    CPlusPlus::Function *targetFunction;
    CPlusPlus::DeclarationAST *targetDeclaration;
    CPlusPlus::FunctionDeclaratorAST *targetFunctionDeclarator;

private:
    FunctionDeclDefLink();

    bool hasMarker;

    friend class FunctionDeclDefLinkFinder;
};

class ApplyDeclDefLinkChanges: public CppQuickFixFactory
{
public:
    void match(const CppQuickFixInterface &interface, TextEditor::QuickFixOperations &result);
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::FunctionDeclDefLink::Marker)

#endif // CPPFUNCTIONDECLDEFLINK_H
