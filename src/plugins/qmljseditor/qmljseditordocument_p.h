// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <languageserverprotocol/servercapabilities.h>
#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljssemanticinfo.h>

#include <QObject>
#include <QTextLayout>
#include <QTimer>

namespace TextEditor { class TextMark; }

namespace QmlJSEditor {

class QmlJSEditorDocument;
class SemanticHighlighter;

namespace Internal {

class QmlOutlineModel;

class SemanticInfoUpdater;

class QmllsStatus
{
public:
    enum class Source { Qmlls, EmbeddedCodeModel };

    Source semanticWarningsSource;
    Source semanticHighlightSource;
    Source completionSource;
    Utils::FilePath qmllsPath;
};

class QmlJSEditorDocumentPrivate : public QObject
{
    Q_OBJECT

public:
    QmlJSEditorDocumentPrivate(QmlJSEditorDocument *parent);
    ~QmlJSEditorDocumentPrivate() override;

    void invalidateFormatterCache();
    void reparseDocument();
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void reupdateSemanticInfo();
    void acceptNewSemanticInfo(const QmlJSTools::SemanticInfo &semanticInfo);
    bool isSemanticInfoOutdated() const;
    void updateOutlineModel();

    void createTextMarks(const QList<QmlJS::DiagnosticMessage> &diagnostics);
    void cleanDiagnosticMarks();
    void createTextMarks(const QmlJSTools::SemanticInfo &info);
    void cleanSemanticMarks();

    // qmlls change source
    void setSemanticWarningSource(QmllsStatus::Source newSource);
    void setSemanticHighlightSource(QmllsStatus::Source newSource);
    void setCompletionSource(QmllsStatus::Source newSource);
public slots:
    void setSourcesWithCapabilities(const LanguageServerProtocol::ServerCapabilities &);
    void settingsChanged();

public:
    QmlJSEditorDocument *q = nullptr;
    QTimer m_updateDocumentTimer; // used to compress multiple document changes
    QTimer m_reupdateSemanticInfoTimer; // used to compress multiple libraryInfo changes
    int m_semanticInfoDocRevision = -1; // document revision to which the semantic info is currently updated to
    SemanticInfoUpdater *m_semanticInfoUpdater;
    QmlJSTools::SemanticInfo m_semanticInfo;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
    SemanticHighlighter *m_semanticHighlighter = nullptr;
    bool m_semanticHighlightingNecessary = false;
    bool m_outlineModelNeedsUpdate = false;
    QTimer m_updateOutlineModelTimer;
    Internal::QmlOutlineModel *m_outlineModel = nullptr;
    QVector<TextEditor::TextMark *> m_diagnosticMarks;
    QVector<TextEditor::TextMark *> m_semanticMarks;
    bool m_isDesignModePreferred = false;
    QmllsStatus m_qmllsStatus = {QmllsStatus::Source::EmbeddedCodeModel,
                                 QmllsStatus::Source::EmbeddedCodeModel,
                                 QmllsStatus::Source::EmbeddedCodeModel,
                                 {}};
};

} // Internal
} // QmlJSEditor
