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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/
#ifndef INSPECTOROUTPUTPANE_H
#define INSPECTOROUTPUTPANE_H

#include <coreplugin/ioutputpane.h>

#include <QtCore/QObject>

QT_BEGIN_NAMESPACE

class QTextEdit;

class RunControl;

class InspectorOutputPane : public Core::IOutputPane
{
    Q_OBJECT
public:
    InspectorOutputPane(QObject *parent = 0);
    virtual ~InspectorOutputPane();

    virtual QWidget *outputWidget(QWidget *parent);
    virtual QList<QWidget*> toolBarWidgets() const;
    virtual QString name() const;

    virtual int priorityInStatusBar() const;

    virtual void clearContents();
    virtual void visibilityChanged(bool visible);

    virtual void setFocus();
    virtual bool hasFocus();
    virtual bool canFocus();

    virtual bool canNavigate();
    virtual bool canNext();
    virtual bool canPrevious();
    virtual void goToNext();
    virtual void goToPrev();

public slots:
    void addOutput(RunControl *, const QString &text);
    void addOutputInline(RunControl *, const QString &text);

    void addErrorOutput(RunControl *, const QString &text);
    void addInspectorStatus(const QString &text);

private:
    QTextEdit *m_textEdit;
};

QT_END_NAMESPACE

#endif

