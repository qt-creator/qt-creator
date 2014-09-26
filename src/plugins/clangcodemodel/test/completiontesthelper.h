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

#ifndef CLANGCODEMODEL_TESTS_COMPLETIONTESTHELPER_H
#define CLANGCODEMODEL_TESTS_COMPLETIONTESTHELPER_H

#ifdef WITH_TESTS

#include "../clangcompleter.h"

#include <QObject>
#include <QTextDocument>
#include <texteditor/texteditor.h>
#include <cplusplus/CppDocument.h>

namespace TextEditor { class IAssistProposal; }

namespace ClangCodeModel {
namespace Internal {

class CompletionTestHelper : public QObject
{
    Q_OBJECT
public:
    explicit CompletionTestHelper(QObject *parent = 0);
    ~CompletionTestHelper();

    void operator <<(const QString &fileName);
    QStringList codeCompleteTexts();
    QList<CodeCompletionResult> codeComplete();

    int position() const;
    const QByteArray &source() const;

    void addOption(const QString &option);

private:
    void findCompletionPos();

    UnsavedFiles m_unsavedFiles;
    ClangCompleter::Ptr m_completer;
    QStringList m_clangOptions;

    QByteArray m_sourceCode;
    int m_position;
    int m_line;
    int m_column;
};

} // namespace Internal
} // namespace ClangCodeModel

#endif

#endif // CLANGCODEMODEL_TESTS_COMPLETIONTESTHELPER_H
