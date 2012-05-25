/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef QMLJSFINDREFERENCES_H
#define QMLJSFINDREFERENCES_H

#include <QMutex>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QPointer>
#include <utils/filesearch.h>
#include <qmljs/qmljsdocument.h>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Find {
    struct SearchResultItem;
    class SearchResult;
} // namespace Find

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
    void renameUsages(const QString &fileName, quint32 offset,
                      const QString &replacement = QString());

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void openEditor(const Find::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Find::SearchResultItem> &items);

private:
    QPointer<Find::SearchResult> m_currentSearch;
    QFutureWatcher<Usage> m_watcher;
};

} // namespace QmlJSEditor

#endif // QMLJSFINDREFERENCES_H
