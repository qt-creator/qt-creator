// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmldesignercorelib_global.h"

#include <qmljs/qmljsdocument.h>
#include <texteditor/tabsettings.h>

#include <QObject>
#include <QTextCursor>
#include <QTextDocument>

#include <optional>

namespace TextEditor { class TabSettings; }

namespace QmlDesigner {

class QMLDESIGNERCORE_EXPORT TextModifier: public QObject
{
    Q_OBJECT

private:
    TextModifier(const TextModifier &);
    TextModifier &operator=(const TextModifier &);

public:
    struct MoveInfo {
        int objectStart;
        int objectEnd;
        int leadingCharsToRemove;
        int trailingCharsToRemove;

        int destination;
        QString prefixToInsert;
        QString suffixToInsert;

        MoveInfo(): objectStart(-1), objectEnd(-1), leadingCharsToRemove(0), trailingCharsToRemove(0), destination(-1) {}
    };

public:
    TextModifier() = default;
    ~TextModifier() override = 0;

    virtual void replace(int offset, int length, const QString& replacement) = 0;
    virtual void move(const MoveInfo &moveInfo) = 0;
    virtual void indent(int offset, int length) = 0;
    virtual void indentLines(int startLine, int endLine) = 0;

    virtual TextEditor::TabSettings tabSettings() const = 0;

    virtual void startGroup() = 0;
    virtual void flushGroup() = 0;
    virtual void commitGroup() = 0;

    virtual QTextDocument *textDocument() const = 0;
    virtual QString text() const = 0;
    virtual QTextCursor textCursor() const = 0;
    static int getLineInDocument(QTextDocument* document, int offset);

    virtual void deactivateChangeSignals() = 0;
    virtual void reactivateChangeSignals() = 0;

    static QmlJS::Snapshot qmljsSnapshot();

    virtual bool renameId(const QString &oldId, const QString &newId) = 0;
    virtual QStringList autoComplete(QTextDocument * /*textDocument*/, int /*position*/, bool explicitComplete = true) = 0;

    virtual bool moveToComponent(int nodeOffset, const QString &importData) = 0;

signals:
    void textChanged();

    void replaced(int offset, int oldLength, int newLength);
    void moved(const TextModifier::MoveInfo &moveInfo);
};

}
