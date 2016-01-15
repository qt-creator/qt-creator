/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppeditortestcase.h"

#include <cpptools/projectpartheaderpath.h>

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

///
/// Represents a test document before and after applying the quick fix.
///
/// A TestDocument's source may contain an '@' character to denote
/// the cursor position. For selections the markers "@{start}" and
/// "@{end}" can be used. The markers are removed before the editor
/// reads the document.
///

class QuickFixTestDocument : public TestDocument
{
public:
    typedef QSharedPointer<QuickFixTestDocument> Ptr;

    QuickFixTestDocument(const QByteArray &fileName, const QByteArray &source,
                         const QByteArray &expectedSource);

    static Ptr create(const QByteArray &fileName, const QByteArray &source,
                      const QByteArray &expectedSource);

private:
    void removeMarkers();

public:
    QString m_expectedSource;
};

typedef QList<QuickFixTestDocument::Ptr> QuickFixTestDocuments;

class BaseQuickFixTestCase : public TestCase
{
public:
    /// Exactly one QuickFixTestDocument must contain the cursor position marker '@'
    /// or "@{start}" and "@{end}"
    BaseQuickFixTestCase(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                         const CppTools::ProjectPartHeaderPaths &headerPaths
                            = CppTools::ProjectPartHeaderPaths());

    ~BaseQuickFixTestCase();

protected:
    QuickFixTestDocument::Ptr m_documentWithMarker;
    QList<QuickFixTestDocument::Ptr> m_testDocuments;

private:
    QScopedPointer<CppTools::Tests::TemporaryDir> m_temporaryDirectory;

    CppTools::CppCodeStylePreferences *m_cppCodeStylePreferences;
    QByteArray m_cppCodeStylePreferencesOriginalDelegateId;

    CppTools::ProjectPartHeaderPaths m_headerPathsToRestore;
    bool m_restoreHeaderPaths;
};

/// Tests a concrete QuickFixOperation of a given CppQuickFixFactory
class QuickFixOperationTest : public BaseQuickFixTestCase
{
public:
    QuickFixOperationTest(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                          CppQuickFixFactory *factory,
                          const CppTools::ProjectPartHeaderPaths &headerPaths
                            = CppTools::ProjectPartHeaderPaths(),
                          int operationIndex = 0,
                          const QByteArray &expectedFailMessage = QByteArray());

    static void run(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                    CppQuickFixFactory *factory,
                    const QString &headerPath,
                    int operationIndex = 0);
};

/// Tests the offered operations provided by a given CppQuickFixFactory
class QuickFixOfferedOperationsTest : public BaseQuickFixTestCase
{
public:
    QuickFixOfferedOperationsTest(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                                  CppQuickFixFactory *factory,
                                  const CppTools::ProjectPartHeaderPaths &headerPaths
                                    = CppTools::ProjectPartHeaderPaths(),
                                  const QStringList &expectedOperations = QStringList());
};

QList<QuickFixTestDocument::Ptr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected);

} // namespace Tests
} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(CppEditor::Internal::Tests::QuickFixTestDocuments)
