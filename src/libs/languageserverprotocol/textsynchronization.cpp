/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "textsynchronization.h"

namespace LanguageServerProtocol {

constexpr const char DidOpenTextDocumentNotification::methodName[];
constexpr const char DidChangeTextDocumentNotification::methodName[];
constexpr const char WillSaveTextDocumentNotification::methodName[];
constexpr const char WillSaveWaitUntilTextDocumentRequest::methodName[];
constexpr const char DidSaveTextDocumentNotification::methodName[];
constexpr const char DidCloseTextDocumentNotification::methodName[];

DidOpenTextDocumentNotification::DidOpenTextDocumentNotification(
        const DidOpenTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidChangeTextDocumentNotification::DidChangeTextDocumentNotification(
        const DidChangeTextDocumentParams &params)
    : DidChangeTextDocumentNotification(methodName, params)
{ }

WillSaveTextDocumentNotification::WillSaveTextDocumentNotification(
        const WillSaveTextDocumentParams &params)
    : Notification(methodName, params)
{ }

WillSaveWaitUntilTextDocumentRequest::WillSaveWaitUntilTextDocumentRequest(const WillSaveTextDocumentParams &params)
    : Request(methodName, params)
{ }

DidSaveTextDocumentNotification::DidSaveTextDocumentNotification(
        const DidSaveTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidCloseTextDocumentNotification::DidCloseTextDocumentNotification(
        const DidCloseTextDocumentParams &params)
    : Notification(methodName, params)
{ }

DidChangeTextDocumentParams::DidChangeTextDocumentParams()
    : DidChangeTextDocumentParams(VersionedTextDocumentIdentifier())
{ }

DidChangeTextDocumentParams::DidChangeTextDocumentParams(
        const VersionedTextDocumentIdentifier &docId, const QString &text)
{
    setTextDocument(docId);
    setContentChanges({text});
}

bool DidChangeTextDocumentParams::isValid(QStringList *error) const
{
    return check<VersionedTextDocumentIdentifier>(error, textDocumentKey)
            && checkArray<TextDocumentContentChangeEvent>(error, contentChangesKey);
}

DidOpenTextDocumentParams::DidOpenTextDocumentParams(const TextDocumentItem &document)
{
    setTextDocument(document);
}

DidCloseTextDocumentParams::DidCloseTextDocumentParams(const TextDocumentIdentifier &document)
{
    setTextDocument(document);
}

DidChangeTextDocumentParams::TextDocumentContentChangeEvent::TextDocumentContentChangeEvent(
        const QString text)
{
    setText(text);
}

bool DidChangeTextDocumentParams::TextDocumentContentChangeEvent::isValid(QStringList *error) const
{
    return checkOptional<Range>(error, rangeKey)
            && checkOptional<int>(error, rangeLengthKey)
            && check<QString>(error, textKey);
}

DidSaveTextDocumentParams::DidSaveTextDocumentParams(const TextDocumentIdentifier &document)
{
    setTextDocument(document);
}

bool DidSaveTextDocumentParams::isValid(QStringList *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && checkOptional<QString>(error, textKey);
}

WillSaveTextDocumentParams::WillSaveTextDocumentParams(
        const TextDocumentIdentifier &document,
        const WillSaveTextDocumentParams::TextDocumentSaveReason &reason)
{
    setTextDocument(document);
    setReason(reason);
}

bool WillSaveTextDocumentParams::isValid(QStringList *error) const
{
    return check<TextDocumentIdentifier>(error, textDocumentKey)
            && check<int>(error, reasonKey);
}

bool TextDocumentSaveRegistrationOptions::isValid(QStringList *error) const
{
    return TextDocumentRegistrationOptions::isValid(error)
            && checkOptional<bool>(error, includeTextKey);
}

} // namespace LanguageServerProtocol
