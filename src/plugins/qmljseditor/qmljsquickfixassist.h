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

#ifndef QMLJSQUICKFIXASSIST_H
#define QMLJSQUICKFIXASSIST_H

#include "qmljseditor.h"

#include <qmljstools/qmljsrefactoringchanges.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/quickfixassistprovider.h>
#include <texteditor/codeassist/quickfixassistprocessor.h>


namespace QmlJSEditor {
namespace Internal {

class QmlJSQuickFixAssistInterface : public TextEditor::AssistInterface
{
public:
    QmlJSQuickFixAssistInterface(QmlJSEditorWidget *editor, TextEditor::AssistReason reason);
    ~QmlJSQuickFixAssistInterface();

    const QmlJSTools::SemanticInfo &semanticInfo() const;
    QmlJSTools::QmlJSRefactoringFilePtr currentFile() const;

private:
    QmlJSTools::SemanticInfo m_semanticInfo;
    QmlJSTools::QmlJSRefactoringFilePtr m_currentFile;
};


class QmlJSQuickFixProcessor : public TextEditor::QuickFixAssistProcessor
{
public:
    QmlJSQuickFixProcessor(const TextEditor::IAssistProvider *provider);
    ~QmlJSQuickFixProcessor();

    const TextEditor::IAssistProvider *provider() const Q_DECL_OVERRIDE;

private:
    const TextEditor::IAssistProvider *m_provider;
};


class QmlJSQuickFixAssistProvider : public TextEditor::QuickFixAssistProvider
{
public:
    QmlJSQuickFixAssistProvider();
    ~QmlJSQuickFixAssistProvider();

    bool isAsynchronous() const Q_DECL_OVERRIDE;
    bool supportsEditor(Core::Id editorId) const Q_DECL_OVERRIDE;
    TextEditor::IAssistProcessor *createProcessor() const Q_DECL_OVERRIDE;

    QList<TextEditor::QuickFixFactory *> quickFixFactories() const Q_DECL_OVERRIDE;
};

} // Internal
} // QmlJSEditor

#endif // QMLJSQUICKFIXASSIST_H
