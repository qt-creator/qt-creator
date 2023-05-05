// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmljseditor_global.h"

#include <utils/filepath.h>
#include <utils/futuresynchronizer.h>
#include <utils/searchresultitem.h>

#include <QObject>
#include <QFutureWatcher>
#include <QPointer>

QT_FORWARD_DECLARE_CLASS(QTimer)

namespace Core { class SearchResult; }

namespace QmlJSEditor {

class QMLJSEDITOR_EXPORT FindReferences: public QObject
{
    Q_OBJECT
public:
    class Usage
    {
    public:
        Usage(const Utils::FilePath &path, const QString &lineText, int line, int col, int len)
            : path(path)
            , lineText(lineText)
            , line(line)
            , col(col)
            , len(len)
        {}

    public:
        Utils::FilePath path;
        QString lineText;
        int line = 0;
        int col = 0;
        int len = 0;
    };

public:
    FindReferences(QObject *parent = nullptr);
    ~FindReferences() override;

signals:
    void changed();

public:
    void findUsages(const Utils::FilePath &fileName, quint32 offset);
    void renameUsages(const Utils::FilePath &fileName,
                      quint32 offset,
                      const QString &replacement = QString());

    static QList<Usage> findUsageOfType(const Utils::FilePath &fileName, const QString &typeName);

private:
    void displayResults(int first, int last);
    void searchFinished();
    void cancel();
    void setPaused(bool paused);
    void onReplaceButtonClicked(const QString &text, const Utils::SearchResultItems &items,
                                bool preserveCase);

    QPointer<Core::SearchResult> m_currentSearch;
    QFutureWatcher<Usage> m_watcher;
    Utils::FutureSynchronizer m_synchronizer;
};

} // namespace QmlJSEditor
