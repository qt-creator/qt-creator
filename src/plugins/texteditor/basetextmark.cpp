/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#include <coreplugin/editormanager/editormanager.h>
#include <extensionsystem/pluginmanager.h>

#include <QtCore/QTimer>
#include <QtGui/QIcon>

namespace TextEditor {

BaseTextMark::BaseTextMark()
    : m_markableInterface(0), m_init(false)
{}

BaseTextMark::~BaseTextMark()
{
    // oha we are deleted
    if (m_markableInterface)
        m_markableInterface->removeMark(this);
    removeInternalMark();
}

void BaseTextMark::setLocation(const QString &fileName, int line)
{
    m_fileName = fileName;
    m_line = line;
    //init();
    // This basically mimics 'two phase initialization'
    QTimer::singleShot(0, this, SLOT(init()));
}

void BaseTextMark::init()
{
    m_init = true;
    Core::EditorManager *em = Core::EditorManager::instance();
    connect(em, SIGNAL(editorOpened(Core::IEditor *)),
        SLOT(editorOpened(Core::IEditor *)));

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
            if (m_markableInterface->addMark(this, m_line)) {
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

    if (!m_markableInterface->addMark(this, m_line))
        removeInternalMark();
}

void BaseTextMark::removeInternalMark()
{
    m_markableInterface = 0;
}

void BaseTextMark::updateMarker()
{
    //qDebug()<<"BaseTextMark::updateMarker()"<<m_markableInterface<<this;
    if (m_markableInterface)
        m_markableInterface->updateMark(this);
}

void BaseTextMark::moveMark(const QString & /* filename */, int /* line */)
{
    Core::EditorManager *em = Core::EditorManager::instance();
    if (!m_init) {
        connect(em, SIGNAL(editorOpened(Core::IEditor *)),
            SLOT(editorOpened(Core::IEditor *)));
        m_init = true;
    }

    if (m_markableInterface)
        m_markableInterface->removeMark(this);

    foreach (Core::IEditor *editor, em->openedEditors())
        editorOpened(editor);
}

} // namespace TextEditor
