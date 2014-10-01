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
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef FORMWINDOWEDITOR_H
#define FORMWINDOWEDITOR_H

#include "designer_export.h"
#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/plaintexteditor.h>

QT_BEGIN_NAMESPACE
class QDesignerFormWindowInterface;
QT_END_NAMESPACE

namespace TextEditor { class PlainTextEditor; }

namespace Designer {

namespace Internal { class DesignerXmlEditorWidget; }
struct FormWindowEditorPrivate;

// IEditor that is used for the QDesignerFormWindowInterface
// It is a read-only PlainTextEditor that shows the XML of the form.
// DesignerXmlEditorWidget is the corresponding PlainTextEditorWidget,
// FormWindowFile the corresponding BaseTextDocument.
// The content from the QDesignerFormWindowInterface is synced to the
// content of the XML editor.

class DESIGNER_EXPORT FormWindowEditor : public TextEditor::PlainTextEditor
{
    Q_PROPERTY(QString contents READ contents)
    Q_OBJECT
public:
    explicit FormWindowEditor(Internal::DesignerXmlEditorWidget *editor);
    virtual ~FormWindowEditor();

    // IEditor
    virtual bool open(QString *errorString, const QString &fileName, const QString &realFileName);

    virtual QWidget *toolBar();

    virtual bool isDesignModePreferred() const;

    // For uic code model support
    QString contents() const;

public slots:
    void syncXmlEditor();

private slots:
    void slotOpen(QString *errorString, const QString &fileName);

private:
    FormWindowEditorPrivate *d;
};

} // namespace Designer

#endif // FORMWINDOWEDITOR_H
