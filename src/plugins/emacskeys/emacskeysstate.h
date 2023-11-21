// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace EmacsKeys {
namespace Internal {

enum EmacsKeysAction {
    KeysAction3rdParty,
    KeysActionKillWord,
    KeysActionKillLine,
    KeysActionOther,
};

class EmacsKeysState : public QObject
{
public:
    EmacsKeysState(QPlainTextEdit *edit);
    ~EmacsKeysState() override;
    void setLastAction(EmacsKeysAction action);
    void beginOwnAction() { m_ignore3rdParty = true; }
    void endOwnAction(EmacsKeysAction action) {
        m_ignore3rdParty = false;
        m_lastAction = action;
    }
    EmacsKeysAction lastAction() const { return m_lastAction; }

    int mark() const { return m_mark; }
    void setMark(int mark) { m_mark = mark; }

private:
    void cursorPositionChanged();
    void textChanged();
    void selectionChanged();

    bool m_ignore3rdParty;
    int m_mark;
    EmacsKeysAction m_lastAction;
    QPlainTextEdit *m_editorWidget;
};

} // namespace Internal
} // namespace EmacsKeys
