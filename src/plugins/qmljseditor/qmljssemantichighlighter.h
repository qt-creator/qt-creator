/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSSEMANTICHIGHLIGHTER_H
#define QMLJSSEMANTICHIGHLIGHTER_H

#include <texteditor/semantichighlighter.h>
#include <QFutureWatcher>
#include <QTextLayout>
#include <QVector>

namespace QmlJS {
class ScopeChain;
namespace AST { class SourceLocation; }
}

namespace TextEditor { class FontSettings; }

namespace QmlJSTools { class SemanticInfo; }

namespace QmlJSEditor {

class QmlJSEditorDocument;

namespace Internal {

class SemanticHighlighter : public QObject
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

    typedef TextEditor::HighlightingResult Use;

    SemanticHighlighter(QmlJSEditorDocument *document);

    void rerun(const QmlJSTools::SemanticInfo &scopeChain);
    void cancel();

    int startRevision() const;

    void updateFontSettings(const TextEditor::FontSettings &fontSettings);
    void reportMessagesInfo(const QVector<QTextLayout::FormatRange> &diagnosticMessages,
                            const QHash<int,QTextCharFormat> &formats);

private slots:
    void applyResults(int from, int to);
    void finished();

private:
    QFutureWatcher<Use>  m_watcher;
    QmlJSEditorDocument *m_document;
    int m_startRevision;
    QHash<int, QTextCharFormat> m_formats;
    QHash<int, QTextCharFormat> m_extraFormats;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
};

} // namespace Internal
} // namespace QmlJSEditor

#endif // QMLJSSEMANTICHIGHLIGHTER_H
