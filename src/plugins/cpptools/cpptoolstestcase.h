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

#ifndef CPPTOOLSTESTCASE_H
#define CPPTOOLSTESTCASE_H

#include "cppmodelmanagerinterface.h"
#include "cpptools_global.h"

#include <coreplugin/editormanager/ieditor.h>

#include <QStringList>

namespace CPlusPlus {
class Document;
class Snapshot;
}
namespace Core { class IEditor; }
namespace TextEditor { class BaseTextEditor; }

namespace CppTools {
namespace Tests {

class CPPTOOLS_EXPORT TestDocument
{
public:
    TestDocument(const QByteArray &fileName, const QByteArray &source, char cursorMarker = '@');

    QString filePath() const;
    bool writeToDisk() const;

public:
    QByteArray m_fileName;
    QByteArray m_source;
    char m_cursorMarker;
};

class CPPTOOLS_EXPORT TestCase
{
    Q_DISABLE_COPY(TestCase)

public:
    TestCase(bool runGarbageCollector = true);
    ~TestCase();

    bool succeededSoFar() const;
    bool openBaseTextEditor(const QString &fileName, TextEditor::BaseTextEditor **editor);
    void closeEditorAtEndOfTestCase(Core::IEditor *editor);

    static bool closeEditorWithoutGarbageCollectorInvocation(Core::IEditor *editor);

    static bool parseFiles(const QString &filePath);
    static bool parseFiles(const QStringList &filePaths);

    static CPlusPlus::Snapshot globalSnapshot();
    static bool garbageCollectGlobalSnapshot();

    static CPlusPlus::Document::Ptr waitForFileInGlobalSnapshot(const QString &filePath);
    static QList<CPlusPlus::Document::Ptr> waitForFilesInGlobalSnapshot(
            const QStringList &filePaths);

    static bool writeFile(const QString &filePath, const QByteArray &contents);

protected:
    CppModelManagerInterface *m_modelManager;
    bool m_succeededSoFar;

private:
    QList<Core::IEditor *> m_editorsToClose;
    bool m_runGarbageCollector;
};

} // namespace Tests
} // namespace CppTools

#endif // CPPTOOLSTESTCASE_H
