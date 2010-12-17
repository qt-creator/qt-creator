/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"
#include "qt_private/formwindowbase_p.h"
#include "designerxmleditor.h"
#include <widgethost.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/mimedatabase.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/plaintexteditor.h>

#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>
#include <QtGui/QApplication>

namespace Designer {

struct FormWindowEditorPrivate
{
    explicit FormWindowEditorPrivate(Internal::DesignerXmlEditor *editor,
                                     QDesignerFormWindowInterface *form);

    TextEditor::PlainTextEditorEditable m_textEditable;
    Internal::FormWindowFile m_file;
    Core::Context m_context;
};

FormWindowEditorPrivate::FormWindowEditorPrivate(Internal::DesignerXmlEditor *editor,
                                                 QDesignerFormWindowInterface *form) :
    m_textEditable(editor), m_file(form)
{
}

FormWindowEditor::FormWindowEditor(Internal::DesignerXmlEditor *editor,
                                                     QDesignerFormWindowInterface *form,
                                                     QObject *parent) :
    Core::IEditor(parent),
    d(new FormWindowEditorPrivate(editor, form))
{
    d->m_context.add(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
    d->m_context.add(Designer::Constants::C_DESIGNER_XML_EDITOR);
    connect(form, SIGNAL(changed()), this, SIGNAL(changed()));
    // Revert to saved/load externally modified files
    connect(&(d->m_file), SIGNAL(reload(QString)), this, SLOT(slotOpen(QString)));
    // Force update of open editors model.
    connect(&(d->m_file), SIGNAL(saved()), this, SIGNAL(changed()));
    connect(&(d->m_file), SIGNAL(changed()), this, SIGNAL(changed()));
    connect(this, SIGNAL(changed()), this, SLOT(configureXmlEditor()));
}

FormWindowEditor::~FormWindowEditor()
{
    delete d;
}

bool FormWindowEditor::createNew(const QString &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::createNew" << contents.size();

    syncXmlEditor(QString());

    QDesignerFormWindowInterface *form = d->m_file.formWindow();
    QTC_ASSERT(form, return false);

    if (contents.isEmpty())
        return false;

    // If we have an override cursor, reset it over Designer loading,
    // should it pop up messages about missing resources or such.
    const bool hasOverrideCursor = QApplication::overrideCursor();
    QCursor overrideCursor;
    if (hasOverrideCursor) {
        overrideCursor = QCursor(*QApplication::overrideCursor());
        QApplication::restoreOverrideCursor();
    }

    form->setContents(contents);

    if (hasOverrideCursor)
        QApplication::setOverrideCursor(overrideCursor);

    if (form->mainContainer() == 0)
        return false;

    syncXmlEditor(contents);
    d->m_file.setFileName(QString());
    return true;
}

void FormWindowEditor::slotOpen(const QString &fileName)
{
    open(fileName);
}

bool FormWindowEditor::open(const QString &fileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::open" << fileName;

    QDesignerFormWindowInterface *form = d->m_file.formWindow();
    QTC_ASSERT(form, return false);

    if (fileName.isEmpty()) {
        setDisplayName(tr("untitled"));
        return true;
    }

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();

    QFile file(absfileName);
    if (!file.open(QIODevice::ReadOnly|QIODevice::Text))
        return false;

    form->setFileName(absfileName);

    const QString contents = QString::fromUtf8(file.readAll());
    form->setContents(contents);
    file.close();
    if (!form->mainContainer())
        return false;
    form->setDirty(false);
    syncXmlEditor(contents);

    setDisplayName(fi.fileName());
    d->m_file.setFileName(absfileName);

    if (Internal::ResourceHandler *rh = qFindChild<Designer::Internal::ResourceHandler*>(form))
        rh->updateResources();

    emit changed();

    return true;
}

void FormWindowEditor::syncXmlEditor()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::syncXmlEditor" << d->m_file.fileName();
    syncXmlEditor(contents());
}

void FormWindowEditor::configureXmlEditor() const
{
    TextEditor::PlainTextEditor *editor =
            qobject_cast<TextEditor::PlainTextEditor *>(d->m_textEditable.editor());
    if (editor)
        editor->configure(Core::ICore::instance()->mimeDatabase()->findByFile(
                d->m_file.fileName()));
}

void FormWindowEditor::syncXmlEditor(const QString &contents)
{
    d->m_textEditable.editor()->setPlainText(contents);
    d->m_textEditable.editor()->setReadOnly(true);
}

Core::IFile *FormWindowEditor::file()
{
    return &d->m_file;
}

QString FormWindowEditor::id() const
{
    return QLatin1String(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
}

QString FormWindowEditor::displayName() const
{
    return d->m_textEditable.displayName();
}

void FormWindowEditor::setDisplayName(const QString &title)
{
    d->m_textEditable.setDisplayName(title);
    emit changed();
}

bool FormWindowEditor::duplicateSupported() const
{
    return false;
}

Core::IEditor *FormWindowEditor::duplicate(QWidget *)
{
    return 0;
}

QByteArray FormWindowEditor::saveState() const
{
    return d->m_textEditable.saveState();
}

bool FormWindowEditor::restoreState(const QByteArray &state)
{
    return d->m_textEditable.restoreState(state);
}

Core::Context FormWindowEditor::context() const
{
    return d->m_context;
}

QWidget *FormWindowEditor::widget()
{
    return d->m_textEditable.widget();
}

bool FormWindowEditor::isTemporary() const
{
    return false;
}

QWidget *FormWindowEditor::toolBar()
{
    return 0;
}

QString FormWindowEditor::contents() const
{
    const qdesigner_internal::FormWindowBase *fw = qobject_cast<const qdesigner_internal::FormWindowBase *>(d->m_file.formWindow());
    QTC_ASSERT(fw, return QString());
    return fw->fileContents(); // No warnings about spacers here
}

TextEditor::BaseTextDocument *FormWindowEditor::textDocument()
{
    return qobject_cast<TextEditor::BaseTextDocument*>(d->m_textEditable.file());
}

TextEditor::PlainTextEditorEditable *FormWindowEditor::textEditable()
{
    return &d->m_textEditable;
}

QString FormWindowEditor::preferredModeType() const
{
    return QLatin1String(Core::Constants::MODE_DESIGN_TYPE);
}

} // namespace Designer

