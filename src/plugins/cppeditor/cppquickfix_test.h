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

#include "cppeditortestcase.h"

#include <QByteArray>
#include <QList>
#include <QSharedPointer>
#include <QStringList>

typedef QByteArray _;

namespace CppTools { class CppCodeStylePreferences; }
namespace TextEditor { class QuickFixOperation; }

namespace CppEditor {
namespace Internal {
namespace Tests {

/**
 * Represents a test document before and after applying the quick fix.
 *
 * A TestDocument's source may contain an '@' character to denote
 * the cursor position. This marker is removed before the Editor reads
 * the document.
 */
class QuickFixTestDocument : public TestDocument
{
public:
    typedef QSharedPointer<QuickFixTestDocument> Ptr;

    QuickFixTestDocument(const QByteArray &fileName, const QByteArray &source,
                         const QByteArray &expectedSource);

    static Ptr create(const QByteArray &fileName, const QByteArray &source,
                                  const QByteArray &expectedSource);

public:
    QString m_expectedSource;
};

/**
 * Encapsulates the whole process of setting up an editor, getting the
 * quick-fix, applying it, and checking the result.
 */
class QuickFixTestCase : public TestCase
{
public:
    QuickFixTestCase(const QList<QuickFixTestDocument::Ptr> &theTestFiles,
                     CppQuickFixFactory *factory,
                     const QStringList &includePaths = QStringList(),
                     int resultIndex = 0);
    ~QuickFixTestCase();

private:
    QSharedPointer<TextEditor::QuickFixOperation> getFix(CppQuickFixFactory *factory,
                                                         CPPEditorWidget *editorWidget,
                                                         int resultIndex = 0);

private:
    QList<QuickFixTestDocument::Ptr> m_testFiles;

    CppTools::CppCodeStylePreferences *m_cppCodeStylePreferences;
    QByteArray m_cppCodeStylePreferencesOriginalDelegateId;

    QStringList m_includePathsToRestore;
    bool m_restoreIncludePaths;
};

QList<QuickFixTestDocument::Ptr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected);

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
