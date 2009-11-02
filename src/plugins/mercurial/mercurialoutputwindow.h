/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Brian McGillion
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef MERCURIALOUTPUTWINDOW_H
#define MERCURIALOUTPUTWINDOW_H

#include <coreplugin/ioutputpane.h>

QT_BEGIN_NAMESPACE
class QListWidget;
QT_END_NAMESPACE

#include <QtCore/QByteArray>

namespace Mercurial {
namespace Internal {

class MercurialOutputWindow: public Core::IOutputPane
{
    Q_OBJECT

public:
    MercurialOutputWindow();
    ~MercurialOutputWindow();

    QWidget *outputWidget(QWidget *parent);
    QList<QWidget*> toolBarWidgets() const;
    QString name() const;
    int priorityInStatusBar() const;
    void clearContents();
    void visibilityChanged(bool visible);
    void setFocus();
    bool hasFocus();
    bool canFocus();
    bool canNavigate();
    bool canNext();
    bool canPrevious();
    void goToNext();
    void goToPrev();

public slots:
    void append(const QString &text);
    void append(const QByteArray &array);

private:
    QListWidget *outputListWidgets;
};

} //namespace Internal
} //namespace Mercurial

#endif // MERCURIALOUTPUTWINDOW_H
