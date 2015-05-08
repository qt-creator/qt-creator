/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H
#define CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H

#include "cpptools_global.h"

#include <texteditor/codeassist/assistenums.h>
#include <texteditor/codeassist/completionassistprovider.h>

QT_BEGIN_NAMESPACE
class QTextDocument;
QT_END_NAMESPACE

namespace CPlusPlus { struct LanguageFeatures; }

namespace TextEditor {
class TextEditorWidget;
class AssistInterface;
}

namespace CppTools {

class CPPTOOLS_EXPORT CppCompletionAssistProvider : public TextEditor::CompletionAssistProvider
{
    Q_OBJECT

public:
    bool supportsEditor(Core::Id editorId) const override;
    int activationCharSequenceLength() const override;
    bool isActivationCharSequence(const QString &sequence) const override;
    bool isContinuationChar(const QChar &c) const override;

    virtual TextEditor::AssistInterface *createAssistInterface(
            const QString &filePath,
            const TextEditor::TextEditorWidget *textEditorWidget,
            const CPlusPlus::LanguageFeatures &languageFeatures,
            int position,
            TextEditor::AssistReason reason) const = 0;

    static int activationSequenceChar(const QChar &ch, const QChar &ch2,
                                      const QChar &ch3, unsigned *kind,
                                      bool wantFunctionCall, bool wantQt5SignalSlots);
};

} // namespace CppTools

#endif // CPPTOOLS_CPPCOMPLETIONASSISTPROVIDER_H
