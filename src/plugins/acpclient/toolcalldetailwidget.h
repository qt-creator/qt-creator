// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "collapsibleframe.h"

#include <acp/acp.h>

#include <utils/icon.h>

#include <optional>

class QAbstractButton;
class QLabel;

namespace Utils {
class ElidingLabel;
class MarkdownBrowser;
} // namespace Utils

namespace AcpClient::Internal {

QWidget *toolCallStatusWidget(Acp::ToolCallStatus status, QWidget *parent = nullptr);
QColor toolCallBorderColor(Acp::ToolCallStatus status);
std::optional<Utils::Icon> iconForToolKind(std::optional<Acp::ToolKind> kind);

class ToolCallDetailWidget : public CollapsibleFrame
{
    Q_OBJECT

public:
    explicit ToolCallDetailWidget(const Acp::ToolCall &toolCall, QWidget *parent = nullptr);

    Acp::ToolCallStatus status() const { return m_status; }
    void applyStatus(Acp::ToolCallStatus status);
    void updateContent(const Acp::ToolCallUpdate &update);
    void setContentMaxWidth(int width);

    void addPermissionControls(const QList<Acp::PermissionOption> &options,
                               bool addDenyFallback);
    void resolvePermission(const QString &text, bool accepted);

signals:
    void permissionOptionSelected(const QString &optionId);
    void permissionCancelled();

protected:
    void paintEvent(QPaintEvent *e) override;

private:
    void populateContent(const Acp::ToolCall &toolCall);
    void addDiffContent(const Acp::Diff &diff);
    void addMarkdownContent(const QString &markdown);
    void addTerminalContent(const Acp::Terminal &terminal);
    void addTextContent(const Acp::Content &content);
    void addLocations(const QList<Acp::ToolCallLocation> &locations);
    void addRawInputContent(const QJsonValue &rawInput);
    void addBodyWidget(QWidget *widget);

    QWidget *m_statusWidget = nullptr;
    Utils::ElidingLabel *m_titleLabel = nullptr;
    QLabel *m_commandLabel = nullptr;
    CollapsibleFrame *m_rawInputFrame = nullptr;
    Utils::MarkdownBrowser *m_rawInputContent = nullptr;
    Acp::ToolCallStatus m_status = Acp::ToolCallStatus::in_progress;
    int m_contentMaxWidth = -1;

    QWidget *m_permissionRow = nullptr;
    QLabel *m_permissionStatusLabel = nullptr;
    QList<QAbstractButton *> m_permissionButtons;
    bool m_collapsibleBeforePermission = false;
    bool m_collapsedBeforePermission = false;
};

} // namespace AcpClient::Internal
