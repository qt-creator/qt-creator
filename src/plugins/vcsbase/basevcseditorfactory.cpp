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

#include <diffeditor/diffeditorconstants.h>

#include <QCoreApplication>
#include <QStringList>

using namespace TextEditor;

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
    const VcsBaseEditorParameters *m_parameters;
    QObject *m_describeReceiver;
    const char *m_describeSlot;
    BaseTextEditorWidgetCreator m_widgetCreator;
};

} // namespace Internal

VcsEditorFactory::VcsEditorFactory(const VcsBaseEditorParameters *parameters,
                                   const BaseTextEditorWidgetCreator &creator,
                                   QObject *describeReceiver, const char *describeSlot)
  : d(new Internal::BaseVcsEditorFactoryPrivate)
{
    d->m_parameters = parameters;
    d->m_describeReceiver = describeReceiver;
    d->m_describeSlot = describeSlot;
    d->m_widgetCreator = creator;
    setId(parameters->id);
    setDisplayName(QCoreApplication::translate("VCS", parameters->displayName));
    if (QLatin1String(parameters->mimeType) != QLatin1String(DiffEditor::Constants::DIFF_EDITOR_MIMETYPE))
        addMimeType(parameters->mimeType);
    new TextEditor::TextEditorActionHandler(this, parameters->context);
}

VcsEditorFactory::~VcsEditorFactory()
{
    delete d;
}

Core::IEditor *VcsEditorFactory::createEditor()
{
    TextEditor::BaseTextEditor *editor = new VcsBaseEditor(d->m_parameters);

    VcsBaseEditorWidget *widget = qobject_cast<VcsBaseEditorWidget *>(d->m_widgetCreator());
    widget->setParameters(d->m_parameters);

    // Pass on signals.
    connect(widget, SIGNAL(describeRequested(QString,QString)),
            editor, SIGNAL(describeRequested(QString,QString)));
    connect(widget, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)),
            editor, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)));
    editor->setEditorWidget(widget);

    widget->init();
    if (d->m_describeReceiver)
        connect(widget, SIGNAL(describeRequested(QString,QString)), d->m_describeReceiver, d->m_describeSlot);

    if (!mimeTypes().isEmpty())
        widget->textDocument()->setMimeType(mimeTypes().front());

    return editor;
}

} // namespace VcsBase
