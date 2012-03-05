/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basetextmark.h"
#include "itexteditor.h"
#include "basetextdocument.h"
#include "texteditorplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

using namespace TextEditor;
using namespace TextEditor::Internal;

BaseTextMarkRegistry::BaseTextMarkRegistry(QObject *parent)
    : QObject(parent)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(editorOpened(Core::IEditor*)),
        SLOT(editorOpened(Core::IEditor*)));
}

void BaseTextMarkRegistry::add(BaseTextMark *mark)
{
    m_marks[Utils::FileName::fromString(mark->fileName())].append(mark);
    Core::EditorManager *em = Core::EditorManager::instance();
    foreach (Core::IEditor *editor, em->editorsForFileName(mark->fileName())) {
        if (ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor)) {
            if (mark->m_markableInterface == 0) { // We aren't added to something
                ITextMarkable *markableInterface = textEditor->markableInterface();
                if (markableInterface->addMark(mark)) {
                    mark->m_markableInterface = markableInterface;
                    // Handle reload of text documents, readding the mark as necessary
                    connect(textEditor->document(), SIGNAL(reloaded()),
                            this, SLOT(documentReloaded()), Qt::UniqueConnection);
                    break;
                }
            }
        }
    }
}

void BaseTextMarkRegistry::remove(BaseTextMark *mark)
{
    m_marks[Utils::FileName::fromString(mark->fileName())].removeOne(mark);
}

void BaseTextMarkRegistry::editorOpened(Core::IEditor *editor)
{
    ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor);
    if (!textEditor)
        return;
    if (!m_marks.contains(Utils::FileName::fromString(editor->document()->fileName())))
        return;

    // Handle reload of text documents, readding the mark as necessary
    connect(textEditor->document(), SIGNAL(reloaded()),
            this, SLOT(documentReloaded()), Qt::UniqueConnection);

    foreach (BaseTextMark *mark, m_marks.value(Utils::FileName::fromString(editor->document()->fileName()))) {
        if (mark->m_markableInterface == 0) { // We aren't added to something
            ITextMarkable *markableInterface = textEditor->markableInterface();
            if (markableInterface->addMark(mark))
                mark->m_markableInterface = markableInterface;
        }
    }
}

void BaseTextMarkRegistry::documentReloaded()
{
    BaseTextDocument *doc = qobject_cast<BaseTextDocument*>(sender());
    if (!doc)
        return;

    if (!m_marks.contains(Utils::FileName::fromString(doc->fileName())))
        return;

    foreach (BaseTextMark *mark, m_marks.value(Utils::FileName::fromString(doc->fileName()))) {
        if (mark->m_markableInterface)
            return;
        ITextMarkable *markableInterface = doc->documentMarker();
        if (markableInterface->addMark(mark))
            mark->m_markableInterface = markableInterface;
    }
}

BaseTextMark::BaseTextMark(const QString &fileName, int lineNumber)
    : ITextMark(lineNumber), m_fileName(fileName)
{
    Internal::TextEditorPlugin::instance()->baseTextMarkRegistry()->add(this);
}

BaseTextMark::~BaseTextMark()
{
    // oha we are deleted
    if (m_markableInterface)
        m_markableInterface.data()->removeMark(this);
    m_markableInterface.clear();
    Internal::TextEditorPlugin::instance()->baseTextMarkRegistry()->remove(this);
}

void BaseTextMark::updateMarker()
{
    if (m_markableInterface)
        m_markableInterface.data()->updateMark(this);
}
