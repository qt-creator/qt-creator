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

#include "basevcseditorfactory.h"
#include "vcsbaseeditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QCoreApplication>

/*!
    \class VcsBase::BaseVCSEditorFactory

    \brief Base class for editor factories creating instances of VcsBaseEditor subclasses.

    \sa VcsBase::VcsBaseEditorWidget
*/

namespace VcsBase {
namespace Internal {

class BaseVcsEditorFactoryPrivate
{
public:
    BaseVcsEditorFactoryPrivate(const VcsBaseEditorParameters *t);

    const VcsBaseEditorParameters *m_type;
    const Core::Id m_id;
    QString m_displayName;
    const QStringList m_mimeTypes;
    TextEditor::TextEditorActionHandler *m_editorHandler;
};

BaseVcsEditorFactoryPrivate::BaseVcsEditorFactoryPrivate(const VcsBaseEditorParameters *t) :
    m_type(t),
    m_id(QByteArray(t->id)),
    m_mimeTypes(QStringList(QLatin1String(t->mimeType))),
    m_editorHandler(new TextEditor::TextEditorActionHandler(t->context))
{
}

} // namespace Internal

BaseVcsEditorFactory::BaseVcsEditorFactory(const VcsBaseEditorParameters *t)
  : d(new Internal::BaseVcsEditorFactoryPrivate(t))
{
    d->m_displayName = QCoreApplication::translate("VCS", t->displayName);
}

BaseVcsEditorFactory::~BaseVcsEditorFactory()
{
    delete d;
}

QStringList BaseVcsEditorFactory::mimeTypes() const
{
    return d->m_mimeTypes;
}

Core::Id BaseVcsEditorFactory::id() const
{
    return d->m_id;
}

QString BaseVcsEditorFactory::displayName() const
{
    return d->m_displayName;
}

Core::IEditor *BaseVcsEditorFactory::createEditor(QWidget *parent)
{
    VcsBaseEditorWidget *vcsEditor = createVcsBaseEditor(d->m_type, parent);

    vcsEditor ->setMimeType(d->m_mimeTypes.front());
    d->m_editorHandler->setupActions(vcsEditor);

    // Wire font settings and set initial values
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            vcsEditor, SLOT(setFontSettings(TextEditor::FontSettings)));
    vcsEditor->setFontSettings(settings->fontSettings());
    return vcsEditor->editor();
}

} // namespace VcsBase
