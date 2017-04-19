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

#include <qmljs/qmljsdocument.h>
#include <qmljstools/qmljssemanticinfo.h>

#include <QObject>
#include <QTextLayout>
#include <QTimer>

namespace QmlJSEditor {

class QmlJSEditorDocument;

namespace Internal {

class QmlOutlineModel;
class SemanticHighlighter;
class SemanticInfoUpdater;

class QmlJSEditorDocumentPrivate : public QObject
{
    Q_OBJECT

public:
    QmlJSEditorDocumentPrivate(QmlJSEditorDocument *parent);
    ~QmlJSEditorDocumentPrivate();

    void invalidateFormatterCache();
    void reparseDocument();
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void reupdateSemanticInfo();
    void acceptNewSemanticInfo(const QmlJSTools::SemanticInfo &semanticInfo);
    void updateOutlineModel();

public:
    QmlJSEditorDocument *q = nullptr;
    QTimer m_updateDocumentTimer; // used to compress multiple document changes
    QTimer m_reupdateSemanticInfoTimer; // used to compress multiple libraryInfo changes
    int m_semanticInfoDocRevision = -1; // document revision to which the semantic info is currently updated to
    SemanticInfoUpdater *m_semanticInfoUpdater;
    QmlJSTools::SemanticInfo m_semanticInfo;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
    Internal::SemanticHighlighter *m_semanticHighlighter = nullptr;
    bool m_semanticHighlightingNecessary = false;
    bool m_outlineModelNeedsUpdate = false;
    bool m_firstSementicInfo = true;
    QTimer m_updateOutlineModelTimer;
    Internal::QmlOutlineModel *m_outlineModel = nullptr;
};

} // Internal
} // QmlJSEditor
