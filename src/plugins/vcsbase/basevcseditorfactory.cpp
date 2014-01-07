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

#include "basevcseditorfactory.h"
#include "vcsbaseeditor.h"

#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QCoreApplication>
#include <QStringList>

/*!
    \class VcsBase::BaseVCSEditorFactory

    \brief The BaseVCSEditorFactory class is the base class for editor
    factories creating instances of VcsBaseEditor subclasses.

    \sa VcsBase::VcsBaseEditorWidget
*/

namespace VcsBase {
namespace Internal {

class BaseVcsEditorFactoryPrivate
{
public:
    BaseVcsEditorFactoryPrivate(const VcsBaseEditorParameters *t);

    const VcsBaseEditorParameters *m_type;
    TextEditor::TextEditorActionHandler *m_editorHandler;
};

BaseVcsEditorFactoryPrivate::BaseVcsEditorFactoryPrivate(const VcsBaseEditorParameters *t) :
    m_type(t),
    m_editorHandler(new TextEditor::TextEditorActionHandler(t->context))
{
}

} // namespace Internal

BaseVcsEditorFactory::BaseVcsEditorFactory(const VcsBaseEditorParameters *t)
  : d(new Internal::BaseVcsEditorFactoryPrivate(t))
{
    setId(t->id);
    setDisplayName(QCoreApplication::translate("VCS", t->displayName));
    addMimeType(t->mimeType);
}

BaseVcsEditorFactory::~BaseVcsEditorFactory()
{
    delete d;
}

Core::IEditor *BaseVcsEditorFactory::createEditor(QWidget *parent)
{
    VcsBaseEditorWidget *vcsEditor = createVcsBaseEditor(d->m_type, parent);

    vcsEditor->setMimeType(mimeTypes().front());
    d->m_editorHandler->setupActions(vcsEditor);

    // Wire font settings and set initial values
    connect(TextEditor::TextEditorSettings::instance(), SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            vcsEditor, SLOT(setFontSettings(TextEditor::FontSettings)));
    vcsEditor->setFontSettings(TextEditor::TextEditorSettings::fontSettings());
    return vcsEditor->editor();
}

} // namespace VcsBase
