/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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
#ifndef INSPECTOROUTPUTWIDGET_H
#define INSPECTOROUTPUTWIDGET_H

#include <QtCore/QObject>
#include <QtGui/QTextEdit>

QT_BEGIN_NAMESPACE
class QAction;
class QContextMenuEvent;
QT_END_NAMESPACE

QT_FORWARD_DECLARE_CLASS(RunControl);

namespace Qml {

class InspectorOutputWidget : public QTextEdit
{
    Q_OBJECT

public:
    InspectorOutputWidget(QWidget *parent = 0);
    virtual ~InspectorOutputWidget();

public slots:
    void addOutput(RunControl *, const QString &text);
    void addOutputInline(RunControl *, const QString &text);

    void addErrorOutput(RunControl *, const QString &text);
    void addInspectorStatus(const QString &text);

protected:
    void contextMenuEvent(QContextMenuEvent *e);

private:
    QAction *m_clearContents;
};

}

#endif // INSPECTOROUTPUTWIDGET_H

