/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "designerxmleditor.h"
#include "designerconstants.h"
#include "qt_private/formwindowbase_p.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/modemanager.h>
#include <coreplugin/imode.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>
#include <texteditor/basetextdocument.h>

#include <utils/qtcassert.h>

#include <QtDesigner/QDesignerFormWindowInterface>

#include <QtCore/QDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QFile>

namespace Designer {

DesignerXmlEditorEditable::DesignerXmlEditorEditable(Internal::DesignerXmlEditor *editor,
                                                     QDesignerFormWindowInterface *form,
                                                     QObject *parent) :
    Core::IEditor(parent),
    m_textEditable(editor),
    m_file(form)
{
    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(QLatin1String(Designer::Constants::K_DESIGNER_XML_EDITOR_ID));
    m_context << uidm->uniqueIdentifier(QLatin1String(Designer::Constants::C_DESIGNER_XML_EDITOR));
    connect(form, SIGNAL(changed()), this, SIGNAL(changed()));
    // Revert to saved/load externally modified files
    connect(&m_file, SIGNAL(reload(QString)), this, SLOT(slotOpen(QString)));
}

bool DesignerXmlEditorEditable::createNew(const QString &contents)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "DesignerXmlEditorEditable::createNew" << contents.size();

    syncXmlEditor(QString());

    QDesignerFormWindowInterface *form = m_file.formWindow();
    QTC_ASSERT(form, return false);

    if (contents.isEmpty())
        return false;

    form->setContents(contents);
    if (form->mainContainer() == 0)
        return false;

    syncXmlEditor(contents);
    m_file.setFileName(QString());
    return true;
}

void DesignerXmlEditorEditable::slotOpen(const QString &fileName)
{
    open(fileName);
}

bool DesignerXmlEditorEditable::open(const QString &fileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "DesignerXmlEditorEditable::open" << fileName;

    QDesignerFormWindowInterface *form = m_file.formWindow();
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
    m_file.setFileName(absfileName);

    emit changed();

    return true;
}

void DesignerXmlEditorEditable::syncXmlEditor()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "DesignerXmlEditorEditable::syncXmlEditor" << m_file.fileName();
    syncXmlEditor(contents());
}

void DesignerXmlEditorEditable::syncXmlEditor(const QString &contents)
{
    m_textEditable.editor()->setPlainText(contents);
    m_textEditable.editor()->setReadOnly(true);
}

Core::IFile *DesignerXmlEditorEditable::file()
{
    return &m_file;
}

QString DesignerXmlEditorEditable::id() const
{
    return QLatin1String(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
}

QString DesignerXmlEditorEditable::displayName() const
{
    return m_textEditable.displayName();
}

void DesignerXmlEditorEditable::setDisplayName(const QString &title)
{
    m_textEditable.setDisplayName(title);
}

bool DesignerXmlEditorEditable::duplicateSupported() const
{
    return false;
}

Core::IEditor *DesignerXmlEditorEditable::duplicate(QWidget *)
{
    return 0;
}

QByteArray DesignerXmlEditorEditable::saveState() const
{
    return m_textEditable.saveState();
}

bool DesignerXmlEditorEditable::restoreState(const QByteArray &state)
{
    return m_textEditable.restoreState(state);
}

QList<int> DesignerXmlEditorEditable::context() const
{
    return m_context;
}

QWidget *DesignerXmlEditorEditable::widget()
{
    return m_textEditable.widget();
}

bool DesignerXmlEditorEditable::isTemporary() const
{
    return false;
}

QWidget *DesignerXmlEditorEditable::toolBar()
{
    return 0;
}

QString DesignerXmlEditorEditable::contents() const
{
    const qdesigner_internal::FormWindowBase *fw = qobject_cast<const qdesigner_internal::FormWindowBase *>(m_file.formWindow());
    QTC_ASSERT(fw, return QString());
    return fw->fileContents(); // No warnings about spacers here
}

TextEditor::BaseTextDocument *DesignerXmlEditorEditable::textDocument()
{
    return qobject_cast<TextEditor::BaseTextDocument*>(m_textEditable.file());
}

TextEditor::PlainTextEditorEditable *DesignerXmlEditorEditable::textEditable()
{
    return &m_textEditable;
}

namespace Internal {

DesignerXmlEditor::DesignerXmlEditor(QDesignerFormWindowInterface *form,
                                     QWidget *parent) :
    TextEditor::PlainTextEditor(parent),
    m_editable(new DesignerXmlEditorEditable(this, form))
{
    setReadOnly(true);
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
             SLOT(updateEditorInfoBar(Core::IEditor*)));
}

TextEditor::BaseTextEditorEditable *DesignerXmlEditor::createEditableInterface()
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "DesignerXmlEditor::createEditableInterface()";
    return m_editable->textEditable();
}

void DesignerXmlEditor::updateEditorInfoBar(Core::IEditor *editor)
{
    if (editor == m_editable) {
        Core::EditorManager::instance()->showEditorInfoBar(Constants::INFO_READ_ONLY,
            tr("This file can only be edited in Design Mode."),
            "Open Designer", this, SLOT(designerModeClicked()));
    }
    if (!editor)
        Core::EditorManager::instance()->hideEditorInfoBar(Constants::INFO_READ_ONLY);
}

void DesignerXmlEditor::designerModeClicked()
{
    Core::ICore::instance()->modeManager()->activateMode(QLatin1String(Core::Constants::MODE_DESIGN));
}

DesignerXmlEditorEditable *DesignerXmlEditor::designerEditable() const
{
    return m_editable;
}

}
} // namespace Designer

