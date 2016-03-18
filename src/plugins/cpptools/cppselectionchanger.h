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

#include "cpptools_global.h"

#include <cplusplus/ASTPath.h>
#include <cplusplus/CppDocument.h>
#include <cplusplus/TranslationUnit.h>

#include <QObject>
#include <QTextCursor>

namespace CppTools {

class ASTNodePositions {
public:
    ASTNodePositions() {}
    ASTNodePositions(CPlusPlus::AST *_ast) : ast(_ast) {}
    operator bool() const { return ast; }

    CPlusPlus::AST *ast = 0;
    unsigned firstTokenIndex = 0;
    unsigned lastTokenIndex = 0;
    unsigned secondToLastTokenIndex = 0;
    int astPosStart = -1;
    int astPosEnd = -1;
};

class CPPTOOLS_EXPORT CppSelectionChanger : public QObject
{
    Q_OBJECT
public:
    explicit CppSelectionChanger(QObject *parent = 0);

    enum Direction {
        ExpandSelection,
        ShrinkSelection
    };

    enum NodeIndexAndStepState {
        NodeIndexAndStepNotSet,
        NodeIndexAndStepWholeDocument,
    };

    bool changeSelection(Direction direction,
                         QTextCursor &cursorToModify,
                         const CPlusPlus::Document::Ptr doc);
    void startChangeSelection();
    void stopChangeSelection();

public slots:
    void onCursorPositionChanged(const QTextCursor &newCursor);

protected slots:
    void fineTuneForStatementPositions(unsigned firstParensTokenIndex,
                                       unsigned lastParensTokenIndex,
                                       ASTNodePositions &positions) const;

private:
    bool performSelectionChange(QTextCursor &cursorToModify);
    ASTNodePositions getASTPositions(CPlusPlus::AST *ast, const QTextCursor &cursor) const;
    void updateCursorSelection(QTextCursor &cursorToModify, ASTNodePositions positions);

    int possibleASTStepCount(CPlusPlus::AST *ast) const;
    int currentASTStep() const;
    ASTNodePositions findNextASTStepPositions(const QTextCursor &cursor);

    void fineTuneASTNodePositions(ASTNodePositions &positions) const;
    ASTNodePositions getFineTunedASTPositions(CPlusPlus::AST *ast, const QTextCursor &cursor) const;
    int getFirstCurrentStepForASTNode(CPlusPlus::AST *ast) const;
    bool isLastPossibleStepForASTNode(CPlusPlus::AST *ast) const;
    ASTNodePositions findRelevantASTPositionsFromCursor(const QList<CPlusPlus::AST *> &astPath,
                                              const QTextCursor &cursor,
                                              int startingFromNodeIndex = -1);
    ASTNodePositions findRelevantASTPositionsFromCursorWhenNodeIndexNotSet(
            const QList<CPlusPlus::AST *> astPath,
            const QTextCursor &cursor);
    ASTNodePositions findRelevantASTPositionsFromCursorWhenWholeDocumentSelected(
            const QList<CPlusPlus::AST *> astPath,
            const QTextCursor &cursor);
    ASTNodePositions findRelevantASTPositionsFromCursorFromPreviousNodeIndex(
            const QList<CPlusPlus::AST *> astPath,
            const QTextCursor &cursor);
    bool shouldSkipASTNodeBasedOnPosition(const ASTNodePositions &positions,
                                          const QTextCursor &cursor) const;
    void setNodeIndexAndStep(NodeIndexAndStepState state);
    int getTokenStartCursorPosition(unsigned tokenIndex, const QTextCursor &cursor) const;
    int getTokenEndCursorPosition(unsigned tokenIndex, const QTextCursor &cursor) const;
    void printTokenDebugInfo(unsigned tokenIndex, const QTextCursor &cursor, QString prefix) const;

    QTextCursor m_initialChangeSelectionCursor;
    QTextCursor m_workingCursor;
    CPlusPlus::Document::Ptr m_doc;
    CPlusPlus::TranslationUnit *m_unit;
    Direction m_direction;
    int m_changeSelectionNodeIndex;
    int m_nodeCurrentStep;
    bool m_inChangeSelection;
};

} // namespace CppTools
