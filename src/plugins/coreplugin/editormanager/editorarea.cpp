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

#include "editorarea.h"

#include "editormanager.h"
#include "ieditor.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icontext.h>
#include <coreplugin/idocument.h>
#include <coreplugin/icore.h>
#include <utils/qtcassert.h>

#include <QApplication>

namespace Core {
namespace Internal {

EditorArea::EditorArea()
    : m_currentView(0),
      m_currentDocument(0)
{
    m_context = new IContext;
    m_context->setContext(Context(Constants::C_EDITORMANAGER));
    m_context->setWidget(this);
    ICore::addContextObject(m_context);

    setCurrentView(view());
    updateCloseSplitButton();

    connect(qApp, &QApplication::focusChanged,
            this, &EditorArea::focusChanged);
    connect(this, &SplitterOrView::splitStateChanged, this, &EditorArea::updateCloseSplitButton);
}

EditorArea::~EditorArea()
{
    // disconnect
    setCurrentView(0);
    disconnect(qApp, &QApplication::focusChanged,
               this, &EditorArea::focusChanged);

    ICore::removeContextObject(m_context);
    delete m_context;
}

IDocument *EditorArea::currentDocument() const
{
    return m_currentDocument;
}

void EditorArea::focusChanged(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)
    // only interesting if the focus moved within the editor area
    if (!focusWidget() || focusWidget() != now)
        return;
    // find the view with focus
    EditorView *current = findFirstView();
    while (current) {
        if (current->focusWidget() && current->focusWidget() == now) {
            setCurrentView(current);
            break;
        }
        current = current->findNextView();
    }
}

void EditorArea::setCurrentView(EditorView *view)
{
    if (view == m_currentView)
        return;
    if (m_currentView) {
        disconnect(m_currentView.data(), &EditorView::currentEditorChanged,
                   this, &EditorArea::updateCurrentEditor);
    }
    m_currentView = view;
    if (m_currentView) {
        connect(m_currentView.data(), &EditorView::currentEditorChanged,
                this, &EditorArea::updateCurrentEditor);
    }
    updateCurrentEditor(m_currentView ? m_currentView->currentEditor() : 0);
}

void EditorArea::updateCurrentEditor(IEditor *editor)
{
    IDocument *document = editor ? editor->document() : 0;
    if (document == m_currentDocument)
        return;
    if (m_currentDocument) {
        disconnect(m_currentDocument.data(), &IDocument::changed,
                   this, &EditorArea::windowTitleNeedsUpdate);
    }
    m_currentDocument = document;
    if (m_currentDocument) {
        connect(m_currentDocument.data(), &IDocument::changed,
                this, &EditorArea::windowTitleNeedsUpdate);
    }
    emit windowTitleNeedsUpdate();
}

void EditorArea::updateCloseSplitButton()
{
    if (EditorView *v = view())
        v->setCloseSplitEnabled(false);
}

} // Internal
} // Core
