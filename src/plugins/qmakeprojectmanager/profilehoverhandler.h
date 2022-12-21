// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/basehoverhandler.h>
#include <texteditor/codeassist/keywordscompletionassist.h>

QT_BEGIN_NAMESPACE
QT_END_NAMESPACE

namespace QmakeProjectManager {
namespace Internal {

class ProFileHoverHandler : public TextEditor::BaseHoverHandler
{
public:
    ProFileHoverHandler();

private:
    void identifyMatch(TextEditor::TextEditorWidget *editorWidget,
                       int pos,
                       ReportPriority report) override;
    void identifyQMakeKeyword(const QString &text, int pos);

    enum ManualKind {
        VariableManual,
        FunctionManual,
        UnknownManual
    };

    QString manualName() const;
    void identifyDocFragment(ManualKind manualKind,
                       const QString &keyword);

    QString m_docFragment;
    ManualKind m_manualKind = UnknownManual;
    const TextEditor::Keywords m_keywords;
};

} // namespace Internal
} // namespace QmakeProjectManager
