// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "chatinputcompletion.h"

#include <texteditor/texteditor.h>

QT_BEGIN_NAMESPACE
class QImage;
class QMimeData;
QT_END_NAMESPACE

namespace Utils { class HistoryCompleter; }

namespace AcpClient::Internal {

class ChatInputEdit : public TextEditor::TextEditorWidget
{
    Q_OBJECT

public:
    explicit ChatInputEdit(QWidget *parent = nullptr);

    void setAvailableCommands(const QList<CommandInfo> &commands);

signals:
    void sendRequested();
    void imagePasted(const QImage &image);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    bool canInsertFromMimeData(const QMimeData *source) const override;
    void insertFromMimeData(const QMimeData *source) override;
    void setDisplaySettings(const TextEditor::DisplaySettingsData &settings) override;
    void setMarginSettings(const TextEditor::MarginSettingsData &settings) override;
    int extraAreaWidth(int * = nullptr) const override { return 0; }

private:
    void updateHeight();
    void updateSuggestion();
    void historyUp();
    void historyDown();

    ChatInputCompletionProvider *m_completionProvider = nullptr;
    Utils::HistoryCompleter *m_history = nullptr;
    int m_historyIndex = -1;
    QString m_editBuffer;
};

} // namespace AcpClient::Internal
