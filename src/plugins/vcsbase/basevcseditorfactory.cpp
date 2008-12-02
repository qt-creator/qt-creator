/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/
#include "basevcseditorfactory.h"
#include "vcsbaseplugin.h"
#include "vcsbaseeditor.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/fontsettings.h>
#include <texteditor/texteditoractionhandler.h>
#include <texteditor/texteditorsettings.h>

namespace VCSBase {

struct BaseVCSEditorFactoryPrivate  {
    BaseVCSEditorFactoryPrivate(const VCSBaseEditorParameters *t, Core::ICore *core);

    const VCSBaseEditorParameters *m_type;
    const QString m_kind;
    const QStringList m_mimeTypes;
    Core::ICore *m_core;
    TextEditor::TextEditorActionHandler *m_editorHandler;
};

BaseVCSEditorFactoryPrivate::BaseVCSEditorFactoryPrivate(const VCSBaseEditorParameters *t, Core::ICore *core) :
    m_type(t),
    m_kind(QLatin1String(t->kind)),
    m_mimeTypes(QStringList(QLatin1String(t->mimeType))),
    m_core(core),
    m_editorHandler(new TextEditor::TextEditorActionHandler(core, t->kind))
{
}

BaseVCSEditorFactory::BaseVCSEditorFactory(const VCSBaseEditorParameters *t,
                                           Core::ICore *core) :
    m_d(new BaseVCSEditorFactoryPrivate(t, core))
{
}

BaseVCSEditorFactory::~BaseVCSEditorFactory()
{
    delete m_d;
}

QStringList BaseVCSEditorFactory::mimeTypes() const
{
    return m_d->m_mimeTypes;
}

QString BaseVCSEditorFactory::kind() const
{
    return m_d->m_kind;
}

Core::IFile *BaseVCSEditorFactory::open(const QString &fileName)
{
    Core::IEditor *iface = m_d->m_core->editorManager()->openEditor(fileName, kind());
    return iface ? iface->file() : 0;
}

Core::IEditor *BaseVCSEditorFactory::createEditor(QWidget *parent)
{
    VCSBaseEditor *vcsEditor = createVCSBaseEditor(m_d->m_type, parent);

    vcsEditor ->setMimeType(m_d->m_mimeTypes.front());
    m_d->m_editorHandler->setupActions(vcsEditor);

    // Wire font settings and set initial values
    TextEditor::TextEditorSettings *settings = TextEditor::TextEditorSettings::instance();
    connect(settings, SIGNAL(fontSettingsChanged(TextEditor::FontSettings)),
            vcsEditor, SLOT(setFontSettings(TextEditor::FontSettings)));
    vcsEditor->setFontSettings(settings->fontSettings());
    return vcsEditor->editableInterface();
}

} // namespace VCSBase
