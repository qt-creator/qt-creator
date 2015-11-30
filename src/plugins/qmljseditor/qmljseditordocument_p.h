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

#ifndef QMLJSEDITORDOCUMENT_P_H
#define QMLJSEDITORDOCUMENT_P_H

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

public slots:
    void invalidateFormatterCache();
    void reparseDocument();
    void onDocumentUpdated(QmlJS::Document::Ptr doc);
    void reupdateSemanticInfo();
    void acceptNewSemanticInfo(const QmlJSTools::SemanticInfo &semanticInfo);
    void updateOutlineModel();

public:
    QmlJSEditorDocument *q;
    QTimer m_updateDocumentTimer; // used to compress multiple document changes
    QTimer m_reupdateSemanticInfoTimer; // used to compress multiple libraryInfo changes
    int m_semanticInfoDocRevision; // document revision to which the semantic info is currently updated to
    SemanticInfoUpdater *m_semanticInfoUpdater;
    QmlJSTools::SemanticInfo m_semanticInfo;
    QVector<QTextLayout::FormatRange> m_diagnosticRanges;
    Internal::SemanticHighlighter *m_semanticHighlighter;
    bool m_semanticHighlightingNecessary;
    bool m_outlineModelNeedsUpdate;
    bool m_firstSementicInfo = true;
    QTimer m_updateOutlineModelTimer;
    Internal::QmlOutlineModel *m_outlineModel;
};

} // Internal
} // QmlJSEditor

#endif // QMLJSEDITORDOCUMENT_P_H
