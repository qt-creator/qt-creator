// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <coreplugin/helpitem.h>
#include <coreplugin/icontext.h>

#include <functional>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace TextEditor {

class TextEditorWidget;

class TEXTEDITOR_EXPORT BaseHoverHandler
{
public:
    virtual ~BaseHoverHandler();

    void contextHelpId(TextEditorWidget *widget,
                       int pos,
                       const Core::IContext::HelpCallback &callback);

    using ReportPriority = std::function<void(int priority)>;
    void checkPriority(TextEditorWidget *widget, int pos, ReportPriority report);
    virtual void abort() {} // Implement for asynchronous priority reporter

    void showToolTip(TextEditorWidget *widget, const QPoint &point);
    bool lastHelpItemAppliesTo(const TextEditorWidget *widget) const;

protected:
    enum {
        Priority_None = 0,
        Priority_Tooltip = 5,
        Priority_Help = 10,
        Priority_Diagnostic = 20,
        Priority_Suggestion = 40
    };
    void setPriority(int priority);
    int priority() const;

    void setToolTip(const QString &tooltip, Qt::TextFormat format = Qt::PlainText);
    const QString &toolTip() const;

    void setLastHelpItemIdentified(const Core::HelpItem &help);
    const Core::HelpItem &lastHelpItemIdentified() const;

    bool isContextHelpRequest() const;

    void propagateHelpId(TextEditorWidget *widget, const Core::IContext::HelpCallback &callback);

    // identifyMatch() is required to report a priority by using the "report" callback.
    // It is recommended to use e.g.
    //    Utils::ExecuteOnDestruction reportPriority([this, report](){ report(priority()); });
    // at the beginning of an implementation to ensure this in any case.
    virtual void identifyMatch(TextEditorWidget *editorWidget, int pos, ReportPriority report);
    virtual void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point);

private:
    void process(TextEditorWidget *widget, int pos, ReportPriority report);

    QString m_toolTip;
    Qt::TextFormat m_textFormat = Qt::PlainText;
    Core::HelpItem m_lastHelpItemIdentified;
    int m_priority = -1;
    bool m_isContextHelpRequest = false;
    TextEditorWidget *m_lastWidget = nullptr;
};

} // namespace TextEditor
