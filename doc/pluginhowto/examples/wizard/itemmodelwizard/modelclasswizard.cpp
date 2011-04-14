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

#include "modelclasswizard.h"
#include "modelnamepage.h"

#include <QFile>
#include <QFileInfo>
#include <cppeditor/cppeditor.h>
#include <cppeditor/cppeditorconstants.h>
#include <coreplugin/basefilewizard.h>

ModelClassWizard::ModelClassWizard(const Core::BaseFileWizardParameters &parameters,QObject *parent)
: Core::BaseFileWizard(parameters, parent)
{
}

ModelClassWizard::~ModelClassWizard()
{
}

QWizard * ModelClassWizard::createWizardDialog(
        QWidget *parent,
        const QString &defaultPath,
        const WizardPageList &extensionPages) const
{
    // Create a wizard
    QWizard* wizard = new QWizard(parent);
    wizard->setWindowTitle("Model Class Wizard");

    // Make our page as first page
    ModelNamePage* page = new ModelNamePage(wizard);
    int pageId = wizard->addPage(page);
    wizard->setProperty("_PageId_", pageId);
    page->setPath(defaultPath);

    // Now add the remaining pages
    foreach (QWizardPage *p, extensionPages)
        wizard->addPage(p);
    return wizard;
}

QString ModelClassWizard::readFile(const QString& fileName, const QMap<QString,QString>&
                                   replacementMap) const
{
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    QString retStr = file.readAll();
    QMap<QString,QString>::const_iterator it = replacementMap.begin();
    QMap<QString,QString>::const_iterator end = replacementMap.end();

    while(it != end)
    {
        retStr.replace(it.key(), it.value());
        ++it;
    }
    return retStr;
}

Core::GeneratedFiles ModelClassWizard::generateFiles(
        const QWizard *w,QString *errorMessage) const
{
    Q_UNUSED(errorMessage);
    Core::GeneratedFiles ret;
    int pageId = w->property("_PageId_").toInt();
    ModelNamePage* page = qobject_cast<ModelNamePage*>(w->page(pageId));

    if(!page)
        return ret;
    ModelClassParameters params = page->parameters();
    QMap<QString,QString> replacementMap;

    replacementMap["{{UPPER_CLASS_NAME}}"] = params.className.toUpper();
    replacementMap["{{BASE_CLASS_NAME}}"] = params.baseClass;
    replacementMap["{{CLASS_NAME}}"] = params.className;
    replacementMap["{{CLASS_HEADER}}"] = QFileInfo(params.headerFile).fileName();

    Core::GeneratedFile headerFile(params.path + "/" + params.headerFile);
    headerFile.setEditorKind(CppEditor::Constants::CPPEDITOR_KIND);

    Core::GeneratedFile sourceFile(params.path + "/" + params.sourceFile);
    sourceFile.setEditorKind(CppEditor::Constants::CPPEDITOR_KIND);

    if(params.baseClass == "QAbstractItemModel")
    {
        headerFile.setContents(readFile(":/CustomProject/ItemModelHeader", replacementMap) );
        sourceFile.setContents(readFile(":/CustomProject/ItemModelSource", replacementMap) );
    }

    else if(params.baseClass == "QAbstractTableModel")
    {
        headerFile.setContents(readFile(":/CustomProject/TableModelHeader", replacementMap) );
        sourceFile.setContents(readFile(":/CustomProject/TableModelSource", replacementMap) );
    }

    else if(params.baseClass == "QAbstractListModel")
    {
        headerFile.setContents(readFile(":/CustomProject/ListModelHeader", replacementMap) );
        sourceFile.setContents(readFile(":/CustomProject/ListModelSource", replacementMap) );
    }

    ret << headerFile << sourceFile;
    return ret;
}
