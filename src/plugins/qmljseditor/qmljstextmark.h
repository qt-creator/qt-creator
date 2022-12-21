// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljsstaticanalysismessage.h>

#include <texteditor/textmark.h>

namespace QmlJSEditor {
namespace Internal {

class QmlJSTextMark : public TextEditor::TextMark
{
public:
    using RemovedFromEditorHandler = std::function<void(QmlJSTextMark *)>;

    QmlJSTextMark(const Utils::FilePath &fileName,
                  const QmlJS::DiagnosticMessage &diagnostic,
                  const RemovedFromEditorHandler &removedHandler);
    QmlJSTextMark(const Utils::FilePath &fileName,
                  const QmlJS::StaticAnalysis::Message &message,
                  const RemovedFromEditorHandler &removedHandler);

private:
    void removedFromEditor() override;
    void init(bool warning, const QString &message);

private:
    RemovedFromEditorHandler m_removedFromEditorHandler;
};

} // namespace Internal
} // namespace QmlJSEditor
