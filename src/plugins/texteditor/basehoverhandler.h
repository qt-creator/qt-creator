/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef BASEHOVERHANDLER_H
#define BASEHOVERHANDLER_H

#include "texteditor_global.h"
#include "helpitem.h"

#include <QObject>
#include <QString>

QT_BEGIN_NAMESPACE
class QPoint;
QT_END_NAMESPACE

namespace Core {
class IEditor;
}

namespace TextEditor {

class ITextEditor;
class BaseTextEditorWidget;

class TEXTEDITOR_EXPORT BaseHoverHandler : public QObject
{
    Q_OBJECT
public:
    BaseHoverHandler(QObject *parent = 0);
    virtual ~BaseHoverHandler();

private slots:
    void editorOpened(Core::IEditor *editor);
    void showToolTip(TextEditor::ITextEditor *editor, const QPoint &point, int pos);
    void updateContextHelpId(TextEditor::ITextEditor *editor, int pos);

protected:
    void setToolTip(const QString &tooltip);
    void appendToolTip(const QString &extension);
    const QString &toolTip() const;

    void addF1ToToolTip();

    void setIsDiagnosticTooltip(bool isDiagnosticTooltip);
    bool isDiagnosticTooltip() const;

    void setLastHelpItemIdentified(const HelpItem &help);
    const HelpItem &lastHelpItemIdentified() const;

    static BaseTextEditorWidget *baseTextEditor(ITextEditor *editor);

private:
    void clear();
    void process(ITextEditor *editor, int pos);

    virtual bool acceptEditor(Core::IEditor *editor) = 0;
    virtual void identifyMatch(ITextEditor *editor, int pos) = 0;
    virtual void decorateToolTip();
    virtual void operateTooltip(ITextEditor *editor, const QPoint &point);

    bool m_diagnosticTooltip;
    QString m_toolTip;
    HelpItem m_lastHelpItemIdentified;
};

} // namespace TextEditor

#endif // BASEHOVERHANDLER_H
