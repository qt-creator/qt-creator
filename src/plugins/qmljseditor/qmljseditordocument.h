// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    QmlJSEditorDocument(Utils::Id id);
    ~QmlJSEditorDocument() override;

    bool supportsCodec(const QTextCodec *codec) const override;

    const QmlJSTools::SemanticInfo &semanticInfo() const;
    bool isSemanticInfoOutdated() const;
    QVector<QTextLayout::FormatRange> diagnosticRanges() const;
    void setDiagnosticRanges(const QVector<QTextLayout::FormatRange> &ranges);
    Internal::QmlOutlineModel *outlineModel() const;

    TextEditor::IAssistProvider *quickFixAssistProvider() const override;

    void setIsDesignModePreferred(bool value);
    bool isDesignModePreferred() const;

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
