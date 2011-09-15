/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "basevcseditorfactory.h"
#include "vcsbaseeditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QCoreApplication>

/*!
    \class VCSBase::BaseVCSEditorFactory

    \brief Base class for editor factories creating instances of VCSBaseEditor subclasses.

    \sa VCSBase::VCSBaseEditorWidget
*/

namespace VCSBase {

struct BaseVCSEditorFactoryPrivate
{
    BaseVCSEditorFactoryPrivate(const VCSBaseEditorParameters *t);

    const VCSBaseEditorParameters *m_type;
    const QString m_id;
    QString m_displayName;
    const QStringList m_mimeTypes;
    TextEditor::TextEditorActionHandler *m_editorHandler;
};

BaseVCSEditorFactoryPrivate::BaseVCSEditorFactoryPrivate(const VCSBaseEditorParameters *t) :
    m_type(t),
    m_id(t->id),
    m_mimeTypes(QStringList(QLatin1String(t->mimeType))),
    m_editorHandler(new TextEditor::TextEditorActionHandler(t->context))
{
}

BaseVCSEditorFactory::BaseVCSEditorFactory(const VCSBaseEditorParameters *t)
  : d(new BaseVCSEditorFactoryPrivate(t))
{
    d->m_displayName = QCoreApplication::translate("VCS", t->displayName);
}

BaseVCSEditorFactory::~BaseVCSEditorFactory()
{
    delete d;
}

QStringList BaseVCSEditorFactory::mimeTypes() const
{
    return d->m_mimeTypes;
}

Core::Id BaseVCSEditorFactory::id() const
{
    return d->m_id;
}

QString BaseVCSEditorFactory::displayName() const
{
    return d->m_displayName;
}

Core::IFile *BaseVCSEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = Core::EditorManager::instance()->openEditor(fileName, id());
    return iface ? iface->file() : 0;
}

Core::IEditor *BaseVCSEditorFactory::createEditor(QWidget *parent)
{
    VCSBaseEditorWidget *vcsEditor = createVCSBaseEditor(d->m_type, parent);

    vcsEditor ->setMimeType(d->m_mimeTypes.front());
    d->m_editorHandler->setupActions(vcsEditor);

    // Wire font settings and set initial values
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            vcsEditor, SLOT(setFontSettings(TextEditor::FontSettings)));
    vcsEditor->setFontSettings(settings->fontSettings());
    return vcsEditor->editor();
}

} // namespace VCSBase
