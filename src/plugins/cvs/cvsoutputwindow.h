/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://www.qtsoftware.com/contact.
**
**************************************************************************/

#ifndef CVSOUTPUTWINDOW_H
#define CVSOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace CVS {
namespace Internal {

class CVSPlugin;

class CVSOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    CVSOutputWindow(CVSPlugin *cvsPlugin);
    ~CVSOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const {
        return QList<QWidget *>();
    }

    QString name() const;
    void clearContents();
    int priorityInStatusBar() const;
    void visibilityChanged(bool visible);

    bool canFocus();
    bool hasFocus();
    void setFocus();

    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();
    bool canNavigate();

public slots:
    void append(const QString &txt, bool popup = false);

private:

    CVSPlugin *m_cvsPlugin;
    QListWidget *m_outputListWidget;
};

} // namespace CVS
} // namespace Internal

#endif // CVSOUTPUTWINDOW_H
