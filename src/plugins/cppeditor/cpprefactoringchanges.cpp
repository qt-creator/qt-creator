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

#include "cpprefactoringchanges.h"

using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;
using namespace CppEditor;

CppRefactoringChanges::CppRefactoringChanges(const Snapshot &snapshot)
    : m_snapshot(snapshot)
    , m_modelManager(CppTools::CppModelManagerInterface::instance())
{
    Q_ASSERT(m_modelManager);
    m_workingCopy = m_modelManager->workingCopy();
}

QStringList CppRefactoringChanges::apply()
{
    const QStringList changedFiles = TextEditor::RefactoringChanges::apply();
    m_modelManager->updateSourceFiles(changedFiles);
    return changedFiles;
}

Document::Ptr CppRefactoringChanges::document(const QString &fileName) const
{
    QString source;
    unsigned editorRevision = 0;
    if (m_workingCopy.contains(fileName)) {
        const QPair<QString, unsigned> workingCopy = m_workingCopy.get(fileName);
        source = workingCopy.first;
        editorRevision = workingCopy.second;
    } else {
        QFile file(fileName);
        if (! file.open(QFile::ReadOnly))
            return Document::Ptr();

        source = QTextStream(&file).readAll(); // ### FIXME read bytes, and remove the convert below
        file.close();
    }

    const QByteArray contents = m_snapshot.preprocessedCode(source, fileName);
    Document::Ptr doc = m_snapshot.documentFromSource(contents, fileName);
    doc->setEditorRevision(editorRevision);
    doc->check();
    return doc;
}
