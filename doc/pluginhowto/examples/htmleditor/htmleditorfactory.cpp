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

#include "coreplugin/editormanager/editormanager.h"
#include "htmleditorfactory.h"
#include "htmleditorwidget.h"
#include "htmleditorplugin.h"
#include "htmleditor.h"

namespace HTMLEditorConstants
{
    const char* const C_HTMLEDITOR_MIMETYPE = "text/html";
    const char* const C_HTMLEDITOR = "HTML Editor";
}

struct HTMLEditorFactoryData
{
    HTMLEditorFactoryData() : kind(HTMLEditorConstants::C_HTMLEDITOR)
    {
        mimeTypes << QString(HTMLEditorConstants::C_HTMLEDITOR_MIMETYPE);
    }

    QString kind;
    QStringList mimeTypes;
};


HTMLEditorFactory::HTMLEditorFactory(HTMLEditorPlugin* owner)
    :Core::IEditorFactory(owner)
{
    d = new HTMLEditorFactoryData;
}

HTMLEditorFactory::~HTMLEditorFactory()
{
    delete d;
}

QStringList HTMLEditorFactory::mimeTypes() const
{
    return d->mimeTypes;
}

QString HTMLEditorFactory::kind() const
{
    return d->kind;
}

Core::IFile* HTMLEditorFactory::open(const QString& fileName)
{
    Core::EditorManager* em = Core::EditorManager::instance();
    Core::IEditor* iface = em->openEditor(fileName, d->kind);

    return iface ? iface->file() : 0;
}

Core::IEditor* HTMLEditorFactory::createEditor(QWidget* parent)
{
    HTMLEditorWidget* editorWidget = new HTMLEditorWidget(parent);

    return new HTMLEditor(editorWidget);
}


