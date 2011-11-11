/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the documentation of Qt Creator.
**
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Nokia Corporation and its Subsidiary(-ies) nor
**     the names of its contributors may be used to endorse or promote
**     products derived from this software without specific prior written
**     permission.
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
****************************************************************************/

#include "htmlfile.h"
#include<QFile>
#include<QFileInfo>

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}
struct HTMLFileData
{
    HTMLFileData(): mimeType(HTMLEditorConstants::C_HTMLEDITOR),
    editorWidget(0), editor(0), modified(false) { }
    const QString mimeType;
    HTMLEditorWidget* editorWidget;
    HTMLEditor* editor;
    QString fileName;
    bool modified;
};

HTMLFile::HTMLFile(HTMLEditor* editor, HTMLEditorWidget* editorWidget)
    : Core::IFile(editor)
{
    d = new HTMLFileData;
    d->editor = editor;
    d->editorWidget = editorWidget;
}


HTMLFile::~HTMLFile()
{
    delete d;
}

void HTMLFile::setModified(bool val)
{
    if(d->modified == val)
        return;

    d->modified = val;
    emit changed();
}

bool HTMLFile::isModified() const
{
    return d->modified;
}

QString HTMLFile::mimeType() const
{
    return d->mimeType;
}

bool HTMLFile::save(const QString &fileName)
{
    QFile file(fileName);

    if(file.open(QFile::WriteOnly))
    {
        d->fileName = fileName;

        QByteArray content = d->editorWidget->content();
        file.write(content);

        setModified(false);

        return true;
    }
    return false;
}

bool HTMLFile::open(const QString &fileName)
{
    QFile file(fileName);

    if(file.open(QFile::ReadOnly))
    {
        d->fileName = fileName;

        QString path = QFileInfo(fileName).absolutePath();
        d->editorWidget->setContent(file.readAll(), path);
        d->editor->setDisplayName(d->editorWidget->title());

        return true;
    }
    return false;
}

void HTMLFile::setFilename(const QString& filename)
{
    d->fileName = filename;
}

QString HTMLFile::fileName() const
{
    return d->fileName;
}
QString HTMLFile::defaultPath() const
{
    return QString();
}

QString HTMLFile::suggestedFileName() const
{
    return QString();
}

QString HTMLFile::fileFilter() const
{
    return QString();
}

QString HTMLFile::fileExtension() const
{
    return QString();
}

bool HTMLFile::isReadOnly() const
{
    return false;
}
bool HTMLFile::isSaveAsAllowed() const
{
    return true;
}

void HTMLFile::modified(ReloadBehavior* behavior)
{
    Q_UNUSED(behavior);
}
