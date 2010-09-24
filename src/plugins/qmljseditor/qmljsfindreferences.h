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

#ifndef QMLJSFINDREFERENCES_H
#define QMLJSFINDREFERENCES_H

#include <QtCore/QMutex>
#include <QtCore/QObject>
#include <QtCore/QPointer>
#include <QtCore/QFuture>
#include <QtCore/QFutureWatcher>
#include <utils/filesearch.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljslookupcontext.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Find {
    class SearchResultWindow;
    struct SearchResultItem;
} // end of namespace Find

namespace QmlJSEditor {

class FindReferences: public QObject
{
    Q_OBJECT
public:
    class Usage
    {
    public:
        Usage()
            : line(0), col(0), len(0) {}

        Usage(const QString &path, const QString &lineText, int line, int col, int len)
            : path(path), lineText(lineText), line(line), col(col), len(len) {}

    public:
        QString path;
        QString lineText;
        int line;
        int col;
        int len;
    };

public:
    FindReferences(QObject *parent = 0);
    virtual ~FindReferences();

Q_SIGNALS:
    void changed();

public:
    void findUsages(const QString &fileName, quint32 offset);

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void openEditor(const Find::SearchResultItem &item);

private:
    void findAll_helper(const QString &fileName, quint32 offset);

private:
    Find::SearchResultWindow *_resultWindow;
    QFutureWatcher<Usage> m_watcher;
};

} // end of namespace QmlJSEditor

#endif // QMLJSFINDREFERENCES_H
