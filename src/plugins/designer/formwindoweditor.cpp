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

#include "formwindoweditor.h"
#include "formwindowfile.h"
#include "designerconstants.h"
#include "resourcehandler.h"

#include <coreplugin/coreconstants.h>
#include <texteditor/textdocument.h>

#include <utils/qtcassert.h>

#include <QBuffer>
#include <QDebug>
#include <QDesignerFormWindowInterface>
#include <QFileInfo>

namespace Designer {

using namespace Internal;

FormWindowEditor::FormWindowEditor()
{
    addContext(Designer::Constants::K_DESIGNER_XML_EDITOR_ID);
    addContext(Designer::Constants::C_DESIGNER_XML_EDITOR);
}

FormWindowEditor::~FormWindowEditor()
{
}

void FormWindowEditor::finalizeInitialization()
{
    // Revert to saved/load externally modified files.
    connect(formWindowFile(), &FormWindowFile::reloadRequested,
            [this](QString *errorString, const QString &fileName) {
                open(errorString, fileName, fileName);
    });
}

bool FormWindowEditor::open(QString *errorString, const QString &fileName, const QString &realFileName)
{
    if (Designer::Constants::Internal::debug)
        qDebug() << "FormWindowEditor::open" << fileName;

    auto document = qobject_cast<FormWindowFile *>(textDocument());
    QDesignerFormWindowInterface *form = document->formWindow();
    QTC_ASSERT(form, return false);

    if (fileName.isEmpty())
        return true;

    const QFileInfo fi(fileName);
    const QString absfileName = fi.absoluteFilePath();

    QString contents;
    if (document->read(absfileName, &contents, errorString) != Utils::TextFileFormat::ReadSuccess)
        return false;

    form->setFileName(absfileName);
    const QByteArray contentsBA = contents.toUtf8();
    QBuffer str;
    str.setData(contentsBA);
    str.open(QIODevice::ReadOnly);
    if (!form->setContents(&str, errorString))
        return false;
    form->setDirty(fileName != realFileName);

    document->syncXmlFromFormWindow();
    document->setFilePath(absfileName);
    document->setShouldAutoSave(false);
    document->resourceHandler()->updateResources(true);

    return true;
}

QWidget *FormWindowEditor::toolBar()
{
    return 0;
}

QString FormWindowEditor::contents() const
{
    return formWindowFile()->formWindowContents();
}

FormWindowFile *FormWindowEditor::formWindowFile() const
{
    return qobject_cast<FormWindowFile *>(textDocument());
}

bool FormWindowEditor::isDesignModePreferred() const
{
    return true;
}

} // namespace Designer

