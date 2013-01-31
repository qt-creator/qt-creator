/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLLIVETEXTPREVIEW_H
#define QMLLIVETEXTPREVIEW_H

#include <QObject>

#include <texteditor/basetexteditor.h>
#include <qmljs/qmljsdocument.h>

namespace Core {
class IEditor;
}

namespace QmlJS {
class ModelManagerInterface;
}

namespace Debugger {
namespace Internal {

class UpdateInspector;
class QmlInspectorAdapter;

class QmlLiveTextPreview : public QObject
{
    Q_OBJECT

public:
    QmlLiveTextPreview(const QmlJS::Document::Ptr &doc,
                       const QmlJS::Document::Ptr &initDoc,
                       QmlInspectorAdapter *inspectorAdapter,
                       QObject *parent = 0);
    ~QmlLiveTextPreview();

    void associateEditor(Core::IEditor *editor);
    void unassociateEditor(Core::IEditor *editor);
    void resetInitialDoc(const QmlJS::Document::Ptr &doc);
    const QString fileName();
    bool hasUnsynchronizableChange() { return m_changesUnsynchronizable; }

signals:
    void selectedItemsChanged(const QList<int> &debugIds);
    void fetchObjectsForLocation(const QString &file,
                                         int lineNumber, int columnNumber);
    void reloadRequest();

public slots:
    void setApplyChangesToQmlInspector(bool applyChanges);
    void updateDebugIds();
    void reloadQml();

private slots:
    void changeSelectedElements(const QList<QmlJS::AST::UiObjectMember *> offsets,
                                const QString &wordAtCursor);
    void documentChanged(QmlJS::Document::Ptr doc);
    void editorContentsChanged();
    void onAutomaticUpdateFailed();

private:
    enum UnsyncronizableChangeType {
        NoUnsyncronizableChanges,
        AttributeChangeWarning,
        ElementChangeWarning,
        JSChangeWarning,
        AutomaticUpdateFailed
    };

    bool changeSelectedElements(const QList<int> offsets, const QString &wordAtCursor);
    QList<int> objectReferencesForOffset(quint32 offset);
    void showSyncWarning(UnsyncronizableChangeType unsyncronizableChangeType,
                         const QString &elementName,
                         unsigned line, unsigned column);
    void removeOutofSyncInfo();

private:
    QHash<QmlJS::AST::UiObjectMember*, QList<int> > m_debugIds;
    QHash<QmlJS::Document::Ptr, QSet<QmlJS::AST::UiObjectMember *> > m_createdObjects;

    QmlJS::Document::Ptr m_previousDoc;
    QmlJS::Document::Ptr m_initialDoc; //the document that was loaded by the server

    QList<QPointer<TextEditor::BaseTextEditorWidget> > m_editors;

    bool m_applyChangesToQmlInspector;
    QmlJS::Document::Ptr m_docWithUnappliedChanges;
    QmlInspectorAdapter *m_inspectorAdapter;
    QList<int> m_lastOffsets;
    QmlJS::AST::UiObjectMember *m_nodeForOffset;
    bool m_updateNodeForOffset;
    bool m_changesUnsynchronizable;
    bool m_contentsChanged;

    friend class UpdateInspector;
};

} // namespace Internal
} // namespace Debugger

#endif // QMLLIVETEXTPREVIEW_H
