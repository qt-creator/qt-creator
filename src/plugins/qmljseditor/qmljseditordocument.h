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

#include "qmljseditor_global.h"

#include <qmljstools/qmljssemanticinfo.h>
#include <texteditor/textdocument.h>

#include <QTextLayout>

namespace QmlJSEditor {

namespace Internal {
class QmlJSEditorDocumentPrivate;
class QmlOutlineModel;
} // Internal

class QMLJSEDITOR_EXPORT QmlJSEditorDocument : public TextEditor::TextDocument
{
    Q_OBJECT
public:
    QmlJSEditorDocument();
    ~QmlJSEditorDocument() override;

    const QmlJSTools::SemanticInfo &semanticInfo() const;
    bool isSemanticInfoOutdated() const;
    QVector<QTextLayout::FormatRange> diagnosticRanges() const;
    void setDiagnosticRanges(const QVector<QTextLayout::FormatRange> &ranges);
    Internal::QmlOutlineModel *outlineModel() const;

    TextEditor::QuickFixAssistProvider *quickFixAssistProvider() const override;

signals:
    void updateCodeWarnings(QmlJS::Document::Ptr doc);
    void semanticInfoUpdated(const QmlJSTools::SemanticInfo &semanticInfo);

protected:
    void applyFontSettings() override;
    void triggerPendingUpdates() override;

private:
    friend class Internal::QmlJSEditorDocumentPrivate; // sending signals
    Internal::QmlJSEditorDocumentPrivate *d;
};

} // QmlJSEditor
