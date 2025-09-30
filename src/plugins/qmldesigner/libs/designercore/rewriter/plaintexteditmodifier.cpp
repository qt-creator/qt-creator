// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "plaintexteditmodifier.h"
#include "rewritertracing.h"

#include <qmljs/qmljsmodelmanagerinterface.h>

#include <utils/changeset.h>

#include <QDebug>
#include <QPlainTextEdit>

namespace QmlDesigner {

using NanotraceHR::keyValue;
using RewriterTracing::category;

PlainTextEditModifier::PlainTextEditModifier(QTextDocument *document)
    : m_textDocument{document}
    , m_textCursor{document}
{
    NanotraceHR::Tracer tracer{"plain text edit modifier constructor", category()};

    connect(document, &QTextDocument::contentsChanged, this, &PlainTextEditModifier::textEditChanged);
}

PlainTextEditModifier::~PlainTextEditModifier()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier destructor", category()};
}

void PlainTextEditModifier::replace(int offset, int length, const QString &replacement)
{
    NanotraceHR::Tracer tracer{"plain text edit modifier replace",
                               category(),
                               keyValue("offset", offset),
                               keyValue("length", length),
                               keyValue("replacement", replacement)};

    Q_ASSERT(offset >= 0);
    Q_ASSERT(length >= 0);

    const int replacementLength = replacement.length();

    if (m_changeSet) {
        m_changeSet->replace(offset, offset + length, replacement);
        emit replaced(offset, length, replacementLength);
    } else {
        Utils::ChangeSet changeSet;
        changeSet.replace(offset, offset + length, replacement);
        emit replaced(offset, length, replacementLength);
        runRewriting(&changeSet);
    }
}

void PlainTextEditModifier::move(const MoveInfo &moveInfo)
{
    NanotraceHR::Tracer tracer{"plain text edit modifier move",
                               category(),
                               keyValue("move info", moveInfo)};

    Q_ASSERT(moveInfo.objectStart >= 0);
    Q_ASSERT(moveInfo.objectEnd > moveInfo.objectStart);
    Q_ASSERT(moveInfo.destination >= 0);
    Q_ASSERT(moveInfo.leadingCharsToRemove >= 0);
    Q_ASSERT(moveInfo.trailingCharsToRemove >= 0);
    Q_ASSERT(moveInfo.objectStart - moveInfo.leadingCharsToRemove >= 0);

    if (m_changeSet) {
        m_changeSet->insert(moveInfo.destination, moveInfo.prefixToInsert);
        m_changeSet->move(moveInfo.objectStart, moveInfo.objectEnd, moveInfo.destination);
        m_changeSet->insert(moveInfo.destination, moveInfo.suffixToInsert);
        m_changeSet->remove(moveInfo.objectStart - moveInfo.leadingCharsToRemove, moveInfo.objectStart);
        m_changeSet->remove(moveInfo.objectEnd, moveInfo.objectEnd + moveInfo.trailingCharsToRemove);
        emit moved(moveInfo);
    } else {
        Utils::ChangeSet changeSet;
        changeSet.insert(moveInfo.destination, moveInfo.prefixToInsert);
        changeSet.move(moveInfo.objectStart, moveInfo.objectEnd, moveInfo.destination);
        changeSet.insert(moveInfo.destination, moveInfo.suffixToInsert);
        changeSet.remove(moveInfo.objectStart - moveInfo.leadingCharsToRemove, moveInfo.objectStart);
        changeSet.remove(moveInfo.objectEnd, moveInfo.objectEnd + moveInfo.trailingCharsToRemove);
        emit moved(moveInfo);
        runRewriting(&changeSet);
    }
}

void PlainTextEditModifier::startGroup()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier start group", category()};

    if (!m_changeSet)
        m_changeSet = new Utils::ChangeSet;

    textCursor().beginEditBlock();
}

void PlainTextEditModifier::flushGroup()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier flush group", category()};

    if (m_changeSet)
        runRewriting(m_changeSet);
}

void PlainTextEditModifier::commitGroup()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier commit group", category()};

    if (m_changeSet) {
        runRewriting(m_changeSet);
        delete m_changeSet;
        m_changeSet = nullptr;
    }

    textCursor().endEditBlock();
}

void PlainTextEditModifier::textEditChanged()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier text edit changed", category()};

    if (!m_ongoingTextChange && m_changeSignalsEnabled)
        emit textChanged();
    else
        m_pendingChangeSignal = true;
}

void PlainTextEditModifier::runRewriting(Utils::ChangeSet *changeSet)
{
    NanotraceHR::Tracer tracer{"plain text edit modifier run rewriting", category()};

    m_ongoingTextChange = true;
    QTextCursor cursor = textCursor();
    changeSet->apply(&cursor);
    m_ongoingTextChange = false;
    textEditChanged();
}

QTextDocument *PlainTextEditModifier::textDocument() const
{
    NanotraceHR::Tracer tracer{"plain text edit modifier text document", category()};

    return m_textDocument;
}

QString PlainTextEditModifier::text() const
{
    NanotraceHR::Tracer tracer{"plain text edit modifier text", category()};

    return m_textDocument->toPlainText();
}

QTextCursor PlainTextEditModifier::textCursor() const
{
    NanotraceHR::Tracer tracer{"plain text edit modifier text cursor", category()};

    return m_textCursor;
}

void PlainTextEditModifier::deactivateChangeSignals()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier deactivate change signals", category()};

    m_changeSignalsEnabled = false;
}

void PlainTextEditModifier::reactivateChangeSignals()
{
    NanotraceHR::Tracer tracer{"plain text edit modifier reactivate change signals", category()};

    m_changeSignalsEnabled = true;

    if (m_pendingChangeSignal) {
        m_pendingChangeSignal = false;
        emit textChanged();
    }
}

} // namespace QmlDesigner
