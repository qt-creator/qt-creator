/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef COMPILEOUTPUTWINDOW_H
#define COMPILEOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

#include <QtGui/QTextEdit>

namespace ProjectExplorer {

class BuildManager;

namespace Internal {

class CompileOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    CompileOutputWindow(BuildManager *bm);
    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets(void) const { return QList<QWidget *>(); }
    QString name() const { return tr("Compile Output"); }
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void appendText(const QString &text);
    bool canFocus();
    bool hasFocus();
    void setFocus();

private:
    QTextEdit *m_textEdit;
};

} // namespace Internal
} // namespace ProjectExplorer

#endif // COMPILEOUTPUTWINDOW_H
