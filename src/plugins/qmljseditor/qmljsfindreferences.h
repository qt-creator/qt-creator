/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QMLJSFINDREFERENCES_H
#define QMLJSFINDREFERENCES_H

#include "qmljseditor_global.h"

#include <QMutex>
#include <QObject>
#include <QFuture>
#include <QFutureWatcher>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core {
class SearchResultItem;
class SearchResult;
} // namespace Core

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT FindReferences: public QObject
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

    static QList<Usage> findUsageOfType(const QString &fileName, const QString typeName);

private Q_SLOTS:
    void displayResults(int first, int last);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void openEditor(const Core::SearchResultItem &item);
    void onReplaceButtonClicked(const QString &text, const QList<Core::SearchResultItem> &items, bool preserveCase);

private:
    QPointer<Core::SearchResult> m_currentSearch;
    QFutureWatcher<Usage> m_watcher;
};

} // namespace QmlJSEditor

#endif // QMLJSFINDREFERENCES_H
