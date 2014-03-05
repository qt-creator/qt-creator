/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "diffeditormanager.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/id.h>
#include <utils/qtcassert.h>

namespace DiffEditor {

static DiffEditorManager *m_instance = 0;

DiffEditorManager::DiffEditorManager(QObject *parent)
    : QObject(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;

    Core::EditorManager *editorManager = Core::EditorManager::instance();
    connect(editorManager, SIGNAL(editorsClosed(QList<Core::IEditor*>)),
            this, SLOT(slotEditorsClosed(QList<Core::IEditor*>)));
}

DiffEditorManager::~DiffEditorManager()
{
    m_instance = 0;
}

DiffEditorManager *DiffEditorManager::instance()
{
    return m_instance;
}

void DiffEditorManager::slotEditorsClosed(const QList<Core::IEditor *> &editors)
{
    QMap<Core::IDocument *, int> editorsForDocument;
    for (int i = 0; i < editors.count(); i++) {
        DiffEditor *diffEditor = qobject_cast<DiffEditor *>(editors.at(i));
        if (diffEditor) {
            Core::IDocument *document = diffEditor->document();
            editorsForDocument[document]++;
        }
    }
    Core::DocumentModel *documentModel = Core::EditorManager::documentModel();
    QMapIterator<Core::IDocument *, int> it(editorsForDocument);
    while (it.hasNext()) {
        it.next();
        if (documentModel->editorsForDocument(it.key()).count() == 0) { // no other editors use that document
            DiffEditorDocument *document
                    = qobject_cast<DiffEditorDocument *>(it.key());
            if (document) {
                const QString documentId = documentToId.value(document);
                documentToId.remove(document);
                idToDocument.remove(documentId);
            }
        }
    }
}

DiffEditorDocument *DiffEditorManager::find(const QString &documentId)
{
    return instance()->idToDocument.value(documentId);
}

DiffEditorDocument *DiffEditorManager::findOrCreate(const QString &documentId, const QString &displayName)
{
    DiffEditorDocument *document = find(documentId);
    if (document)
        return document;

    const QString msgWait = tr("Waiting for data...");
    DiffEditor *diffEditor = qobject_cast<DiffEditor *>(
                Core::EditorManager::openEditorWithContents(Constants::DIFF_EDITOR_ID,
                                                            0, msgWait.toUtf8(),
                                                            (Core::EditorManager::OpenInOtherSplit
                                                             | Core::EditorManager::NoNewSplits)));
    QTC_ASSERT(diffEditor, return 0);

    document = qobject_cast<DiffEditorDocument *>(diffEditor->document());
    QTC_ASSERT(diffEditor, return 0);

    document->setDisplayName(displayName);

    instance()->idToDocument.insert(documentId, document);
    instance()->documentToId.insert(document, documentId);

    return document;
}


} // namespace DiffEditor
