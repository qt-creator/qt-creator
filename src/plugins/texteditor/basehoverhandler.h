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

#pragma once

#include "texteditor_global.h"
#include "helpitem.h"

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

    bool isAsyncHandler() const;
    void setIsAsyncHandler(bool isAsyncHandler);

    QString contextHelpId(TextEditorWidget *widget, int pos);

    using ReportPriority = std::function<void(int priority)>;
    void checkPriority(TextEditorWidget *widget, int pos, ReportPriority report);
    virtual void cancelAsyncCheck();

    void showToolTip(TextEditorWidget *widget, const QPoint &point, bool decorate = true);

protected:
    enum {
        Priority_None = 0,
        Priority_Tooltip = 5,
        Priority_Help = 10,
        Priority_Diagnostic = 20
    };
    void setPriority(int priority);
    int priority() const;

    void setToolTip(const QString &tooltip);
    const QString &toolTip() const;

    void setLastHelpItemIdentified(const HelpItem &help);
    const HelpItem &lastHelpItemIdentified() const;

    virtual void identifyMatch(TextEditorWidget *editorWidget, int pos);
    virtual void identifyMatchAsync(TextEditorWidget *editorWidget, int pos, ReportPriority report);
    virtual void decorateToolTip();
    virtual void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point);

private:
    void process(TextEditorWidget *widget, int pos, ReportPriority report);

    bool m_isAsyncHandler = false;

    QString m_toolTip;
    HelpItem m_lastHelpItemIdentified;
    int m_priority = -1;
};

} // namespace TextEditor
