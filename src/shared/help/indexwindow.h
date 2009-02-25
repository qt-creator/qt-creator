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

#ifndef INDEXWINDOW_H
#define INDEXWINDOW_H

#include <QtCore/QUrl>
#include <QtGui/QWidget>
#include <QtGui/QLineEdit>

QT_BEGIN_NAMESPACE

class QHelpIndexWidget;
class QHelpEngine;

class IndexWindow : public QWidget
{
    Q_OBJECT

public:
    IndexWindow(QHelpEngine *helpEngine, QWidget *parent = 0);
    ~IndexWindow();

    void setSearchLineEditText(const QString &text);
    QString searchLineEditText() const
    {
        return m_searchLineEdit->text();
    }

signals:
    void linkActivated(const QUrl &link);
    void linksActivated(const QMap<QString, QUrl> &links,
        const QString &keyword);
    void escapePressed();

private slots:
    void filterIndices(const QString &filter);
    void enableSearchLineEdit();
    void disableSearchLineEdit();

private:
    bool eventFilter(QObject *obj, QEvent *e);
    void focusInEvent(QFocusEvent *e);

    QLineEdit *m_searchLineEdit;
    QHelpIndexWidget *m_indexWidget;
    QHelpEngine *m_helpEngine;
};

QT_END_NAMESPACE

#endif // INDEXWINDOW_H
