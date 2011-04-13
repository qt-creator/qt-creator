/***************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#ifndef HTMLFILE_H
#define HTMLFILE_H
#include "coreplugin/ifile.h"
#include "htmleditorwidget.h"
#include "htmleditor.h"

struct HTMLFileData;
class HTMLFile : public Core::IFile
{
    Q_OBJECT

public:
    HTMLFile(HTMLEditor* editor, HTMLEditorWidget* editorWidget);
    ~HTMLFile();

    void setModified(bool val=true);
    bool isModified() const ;

    bool save(const QString &filename);
    bool open(const QString &filename);
    void setFilename(const QString &filename);
    QString mimiType(void) const ;
    QString fileName() const;
    QString defaultPath() const ;
    QString mimeType() const;
    QString suggestedFileName() const;
    QString fileFilter() const;
    QString fileExtension() const;
    bool isReadOnly() const;
    bool isSaveAsAllowed() const;
    void modified(ReloadBehavior* behavior);

protected slots:
    void modified() { setModified(true); }
private:
    HTMLFileData* d;
};
#endif // HTMLFILE_H
