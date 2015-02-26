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

#ifndef BASEHOVERHANDLER_H
#define BASEHOVERHANDLER_H

#include "texteditor_global.h"
#include "helpitem.h"

#include <QObject>

namespace Core { class IEditor; }

namespace TextEditor {

class BaseTextEditor;
class TextEditorWidget;

class TEXTEDITOR_EXPORT BaseHoverHandler : public QObject
{
    Q_OBJECT

public:
    BaseHoverHandler();
    ~BaseHoverHandler();

    QString contextHelpId(TextEditorWidget *widget, int pos);
    void showToolTip(TextEditorWidget *widget, const QPoint &point, int pos);

protected:
    void setToolTip(const QString &tooltip);
    void appendToolTip(const QString &extension);
    const QString &toolTip() const;

    void addF1ToToolTip();

    void setIsDiagnosticTooltip(bool isDiagnosticTooltip);
    bool isDiagnosticTooltip() const;

    void setLastHelpItemIdentified(const HelpItem &help);
    const HelpItem &lastHelpItemIdentified() const;

private:
    void clear();
    void process(TextEditorWidget *widget, int pos);

    virtual void identifyMatch(TextEditorWidget *editorWidget, int pos) = 0;
    virtual void decorateToolTip();
    virtual void operateTooltip(TextEditorWidget *editorWidget, const QPoint &point);

    bool m_diagnosticTooltip;
    QString m_toolTip;
    HelpItem m_lastHelpItemIdentified;
};

} // namespace TextEditor

#endif // BASEHOVERHANDLER_H
