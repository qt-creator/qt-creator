// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljseditor/qmljseditor_global.h>
#include <texteditor/semantichighlighter.h>
#include <utils/futuresynchronizer.h>
#include <QFutureWatcher>
#include <QTextLayout>
#include <QVector>

namespace QmlJS {
class SourceLocation;
}

namespace TextEditor { class FontSettings; }

namespace QmlJSTools { class SemanticInfo; }

namespace QmlJSEditor {

class QmlJSEditorDocument;

class QMLJSEDITOR_EXPORT SemanticHighlighter : public QObject
{
    Q_OBJECT
public:
    enum UseType
    {
        UnknownType,
        LocalIdType, // ids in the same file
        ExternalIdType, // ids from instantiating files
        QmlTypeType, // qml types
        RootObjectPropertyType, // property in root object
        ScopeObjectPropertyType, // property in scope object
        ExternalObjectPropertyType, // property in root object of instantiating file
        JsScopeType, // var or function in local js scope
        JsImportType, // name of js import
        JsGlobalType, // in global scope
        LocalStateNameType, // name of a state in the current file
        BindingNameType, // name on the left hand side of a binding
        FieldType, // member of an object
        Max // number of the last used value (to generate the warning formats)
    };

    using Use = TextEditor::HighlightingResult;

    SemanticHighlighter(QmlJSEditorDocument *document);

    void rerun(const QmlJSTools::SemanticInfo &scopeChain);
    void cancel();

    int startRevision() const;
    void setEnableWarnings(bool e);
    void setEnableHighlighting(bool e);

    void updateFontSettings(const TextEditor::FontSettings &fontSettings);
    void reportMessagesInfo(const QVector<QTextLayout::FormatRange> &diagnosticMessages,
                            const QHash<int,QTextCharFormat> &formats);

private:
    void applyResults(int from, int to);
    void finished();
    void run(QPromise<Use> &promise,
             const QmlJSTools::SemanticInfo &semanticInfo,
             const TextEditor::FontSettings &fontSettings);

    QFutureWatcher<Use>  m_watcher;
    QmlJSEditorDocument *m_document;
    int m_startRevision;
    QHash<int, QTextCharFormat> m_formats;
    QHash<int, QTextCharFormat> m_extraFormats;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
    Utils::FutureSynchronizer m_futureSynchronizer;
    bool m_enableWarnings = true;
    bool m_enableHighlighting = true;
};

} // namespace QmlJSEditor
