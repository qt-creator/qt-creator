/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SUBVERSIONOUTPUTWINDOW_H
#define SUBVERSIONOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

namespace Subversion {
namespace Internal {

class SubversionPlugin;

class SubversionOutputWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    SubversionOutputWindow(SubversionPlugin *svnPlugin);
    ~SubversionOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets(void) const {
        return QList<QWidget *>();
    }

    QString name() const;
    void clearContents();
    int priorityInStatusBar() const;
    void visibilityChanged(bool visible);

    virtual bool canFocus();
    virtual bool hasFocus();
    virtual void setFocus();

public slots:
    void append(const QString &txt, bool popup = false);

private:

    SubversionPlugin *m_svnPlugin;
    QListWidget *m_outputListWidget;
};

} // namespace Subversion
} // namespace Internal

#endif // SUBVERSIONOUTPUTWINDOW_H
