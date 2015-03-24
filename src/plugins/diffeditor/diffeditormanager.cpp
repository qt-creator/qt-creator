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

#include "diffeditormanager.h"
#include "diffeditor.h"
#include "diffeditorconstants.h"
#include "diffeditordocument.h"
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/id.h>
#include <coreplugin/idocument.h>
#include <coreplugin/editormanager/ieditor.h>

#include <utils/qtcassert.h>

namespace DiffEditor {

using namespace Internal;

static DiffEditorManager *m_instance = 0;

DiffEditorManager::DiffEditorManager(QObject *parent)
    : QObject(parent)
{
    QTC_ASSERT(!m_instance, return);
    m_instance = this;
}

DiffEditorManager::~DiffEditorManager()
{
    m_instance = 0;
}

Core::IDocument *DiffEditorManager::find(const QString &vcsId)
{
    Core::IDocument *document = m_instance->m_idToDocument.value(vcsId);
    QTC_ASSERT(!document || document->isTemporary(), return 0);
    return document;
}

Core::IDocument *DiffEditorManager::findOrCreate(const QString &vcsId, const QString &displayName)
{
    auto document = static_cast<Internal::DiffEditorDocument *>(find(vcsId));
    if (document)
        return document;

    DiffEditor *diffEditor = qobject_cast<DiffEditor *>(
                Core::EditorManager::openEditorWithContents(Constants::DIFF_EDITOR_ID, 0));
    QTC_ASSERT(diffEditor, return 0);

    document = qobject_cast<Internal::DiffEditorDocument *>(diffEditor->document());
    QTC_ASSERT(diffEditor, return 0);

    document->setPreferredDisplayName(displayName);

    m_instance->m_idToDocument.insert(vcsId, document);

    return document;
}

DiffEditorController *DiffEditorManager::controller(Core::IDocument *document)
{
    auto doc = qobject_cast<DiffEditorDocument *>(document);
    return doc ? doc->controller() : 0;
}

void DiffEditorManager::removeDocument(Core::IDocument *document)
{
    DiffEditorDocument *doc = qobject_cast<DiffEditorDocument *>(document);
    QTC_ASSERT(doc, return);
    for (auto it = m_instance->m_idToDocument.constBegin(); it != m_instance->m_idToDocument.constEnd(); ++it) {
        if (it.value() == doc) {
            m_instance->m_idToDocument.remove(it.key());
            break;
        }
    }
}

} // namespace DiffEditor
