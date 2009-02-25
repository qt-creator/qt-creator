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

#ifndef HELPENGINE_H
#define HELPENGINE_H

#include "docuparser.h"

#include <QtCore/QThread>
#include <QtCore/QPair>
#include <QtCore/QMap>
#include <QtCore/QWaitCondition>
#include <QtCore/QMutex>
#include <QtGui/QStringListModel>

namespace Help {
namespace Internal {

class HelpEngine;

typedef QList<ContentItem> ContentList;

class IndexListModel: public QStringListModel
{
public:
    IndexListModel(QObject *parent = 0)
        : QStringListModel(parent) {}

    void clear() { contents.clear(); setStringList(QStringList()); }

    QString description(int index) const { return stringList().at(index); }
    QStringList links(int index) const { return contents.values(stringList().at(index)); }
    void addLink(const QString &description, const QString &link) { contents.insert(description, link); }

    void publish() { filter(QString(), QString()); }

    QModelIndex filter(const QString &s, const QString &real);

    virtual Qt::ItemFlags flags(const QModelIndex &index) const
    { return QStringListModel::flags(index) & ~Qt::ItemIsEditable; }

private:
    QMultiMap<QString, QString> contents;
};

class TitleMapThread : public QThread
{
    Q_OBJECT

public:    
    TitleMapThread(HelpEngine *he);
    ~TitleMapThread();
    void setup();
    QList<QPair<QString, ContentList> > contents() const { return contentList; }
    QMap<QString, QString> documentTitleMap() const { return titleMap; }

signals:
    void errorOccured(const QString &errMsg);

protected:
    void run();

private:
    void getAllContents();
    void buildContentDict();
    
    QList<QPair<QString, ContentList> > contentList;
    QMap<QString, QString> titleMap;
    
    HelpEngine *engine;
    bool done;
};

class IndexThread : public QThread
{
    Q_OBJECT
        
public:
    IndexThread(HelpEngine *he);
    void setup();
    IndexListModel *model() const { return indexModel; }

protected:
    void run();

signals:
    void errorOccured(const QString &errMsg);

private:    
    void buildKeywordDB();

    HelpEngine *engine;
    QStringList keywordDocuments;
    IndexListModel *indexModel;
    bool indexDone;
};

class HelpEngine : public QObject
{
    Q_OBJECT

public:
    HelpEngine(QObject *parent, const QString &defaultQtVersionPath);
    ~HelpEngine();
    void init();    
    QList<QPair<QString, ContentList> > contents() const { return contentList; }
    IndexListModel *indices();
    
    QString titleOfLink(const QString &link);
    static QString removeAnchorFromLink(const QString &link);

    QString home() const;

signals:
    void indexInitialized();
    void contentsInitialized();
    void errorOccured(const QString &errMsg);

public slots:
    void buildContents();
    void buildIndex();

private slots:
    void titleMapFinished();
    void indexFinished();

private:
    friend class TitleMapThread;
    friend class IndexThread;

    void removeOldCacheFiles(bool onlyFulltextSearchIndex = false);
    QString cacheFilePath() const;
    quint32 getFileAges();

    QList<QPair<QString, ContentList> > contentList;
    QMap<QString, QString> titleMap;
    
    QString cacheFilesPath;
    IndexListModel *indexModel;

    QWaitCondition titleMapDoneCondition;
    QMutex mutex;

    TitleMapThread *titleMapThread;
    IndexThread *indexThread;

    bool contentsOnly;
};

} // namespace Internal
} // namespace Help

#endif // HELPENGINE_H
