/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "basetextmark.h"
#include "itexteditor.h"
#include "basetextdocument.h"

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QTimer>
#include <QtGui/QIcon>

namespace TextEditor {
namespace Internal {

class InternalMark : public TextEditor::ITextMark
{
public:
    explicit InternalMark(BaseTextMark *parent) : m_parent(parent) {}

    virtual QIcon icon() const
    {
        return m_parent->icon();
    }

    virtual void updateLineNumber(int lineNumber)
    {
        return m_parent->updateLineNumber(lineNumber);
    }

    virtual void updateBlock(const QTextBlock &block)
    {
        return m_parent->updateBlock(block);
    }

    virtual void removedFromEditor()
    {
        m_parent->childRemovedFromEditor(this);
    }

    virtual void documentClosing()
    {
        m_parent->documentClosingFor(this);
    }

    virtual Priority priority() const
    {
        return m_parent->priority();
    }
private:
    BaseTextMark *m_parent;
};

} // namespace Internal

BaseTextMark::BaseTextMark(const QString &filename, int line)
    : m_markableInterface(0)
    , m_internalMark(0)
    , m_fileName(filename)
    , m_line(line)
    , m_init(false)
{
    // Why is this?
    QTimer::singleShot(0, this, SLOT(init()));
}

BaseTextMark::~BaseTextMark()
{
    // oha we are deleted
    if (m_markableInterface)
        m_markableInterface->removeMark(m_internalMark);
    removeInternalMark();
}

void BaseTextMark::init()
{
    m_init = true;
    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(editorOpened(Core::IEditor *)), this, SLOT(editorOpened(Core::IEditor *)));

    foreach (Core::IEditor *editor, em->openedEditors())
        editorOpened(editor);
}

void BaseTextMark::editorOpened(Core::IEditor *editor)
{
#ifdef Q_OS_WIN
    if (m_fileName.compare(editor->file()->fileName(), Qt::CaseInsensitive))
        return;
#else
    if (editor->file()->fileName() != m_fileName)
        return;
#endif
    if (ITextEditor *textEditor = qobject_cast<ITextEditor *>(editor)) {
        if (m_markableInterface == 0) { // We aren't added to something
            m_markableInterface = textEditor->markableInterface();
            m_internalMark = new Internal::InternalMark(this);

            if (m_markableInterface->addMark(m_internalMark, m_line)) {
                // Handle reload of text documents, readding the mark as necessary
                connect(textEditor->file(), SIGNAL(reloaded()),
                        this, SLOT(documentReloaded()), Qt::UniqueConnection);
            } else {
                removeInternalMark();
            }
        }
    }
}

void BaseTextMark::documentReloaded()
{
    if (m_markableInterface)
        return;

    BaseTextDocument *doc = qobject_cast<BaseTextDocument*>(sender());
    if (!doc)
        return;

    m_markableInterface = doc->documentMarker();
    m_internalMark = new Internal::InternalMark(this);

    if (!m_markableInterface->addMark(m_internalMark, m_line))
        removeInternalMark();
}

void BaseTextMark::childRemovedFromEditor(Internal::InternalMark *mark)
{
    Q_UNUSED(mark)
    // m_internalMark was removed from the editor
    removeInternalMark();
    removedFromEditor();
}

void BaseTextMark::documentClosingFor(Internal::InternalMark *mark)
{
    Q_UNUSED(mark)
    removeInternalMark();
}

void BaseTextMark::removeInternalMark()
{
    delete m_internalMark;
    m_internalMark = 0;
    m_markableInterface = 0;
}

//#include <QDebug>

void BaseTextMark::updateMarker()
{
    //qDebug()<<"BaseTextMark::updateMarker()"<<m_markableInterface<<m_internalMark;
    if (m_markableInterface)
        m_markableInterface->updateMark(m_internalMark);
}

void BaseTextMark::moveMark(const QString & /* filename */, int /* line */)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    if (!m_init) {
        connect(em, SIGNAL(editorOpened(Core::IEditor *)), this, SLOT(editorOpened(Core::IEditor *)));
        m_init = true;
    }

    if (m_markableInterface)
        m_markableInterface->removeMark(m_internalMark);
    // This is only necessary since m_internalMark is created in editorOpened
    removeInternalMark();

    foreach (Core::IEditor *editor, em->openedEditors())
        editorOpened(editor);
}
} // namespace TextEditor
