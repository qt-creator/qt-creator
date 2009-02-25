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

#include "helpengine.h"
#include "config.h"

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDateTime>
#include <QtCore/QCoreApplication>

using namespace Help::Internal;

static bool verifyDirectory(const QString &str)
{
    QFileInfo dirInfo(str);
    if (!dirInfo.exists())
        return QDir().mkdir(str);
    if (!dirInfo.isDir()) {
        qWarning("'%s' exists but is not a directory", str.toLatin1().constData());
        return false;
    }
    return true;
}

struct IndexKeyword
{
    IndexKeyword(const QString &kw, const QString &l)
        : keyword(kw), link(l) {}
    IndexKeyword() : keyword(QString()), link(QString()) {}
    bool operator<(const IndexKeyword &ik) const {
        return keyword.toLower() < ik.keyword.toLower();
    }
    bool operator<=(const IndexKeyword &ik) const {
        return keyword.toLower() <= ik.keyword.toLower();
    }
    bool operator>(const IndexKeyword &ik) const {
        return keyword.toLower() > ik.keyword.toLower();
    }
    Q_DUMMY_COMPARISON_OPERATOR(IndexKeyword)
    QString keyword;
    QString link;
};

QDataStream &operator>>(QDataStream &s, IndexKeyword &ik)
{
    s >> ik.keyword;
    s >> ik.link;
    return s;
}

QDataStream &operator<<(QDataStream &s, const IndexKeyword &ik)
{
    s << ik.keyword;
    s << ik.link;
    return s;
}


/**
 * Compare in a human-preferred alphanumeric way,
 * e.g. 'Qt tutorial 2' will be less than 'Qt tutorial 11'.
 */
bool caseInsensitiveLessThan(const QString &as, const QString &bs)
{
    const QChar *a = as.unicode();
    const QChar *b = bs.unicode();
    int result = 0;
    while (result == 0)
    {
        ushort aa = a->unicode();
        ushort bb = b->unicode();

        if (aa == 0 || bb == 0) {
            result = aa - bb;
            break;
        }
        else if (a->isDigit() && b->isDigit())
        {
            const QChar *a_begin = a;
            const QChar *b_begin = b;
            bool loop = true;
            do {
                if (a->isDigit()) ++a;
                else if (b->isDigit()) ++b;
                else loop = false;
            } while (loop);

            // optimization: comparing the length of the two numbers is more efficient than constructing two qstrings.
            result = (a - a_begin) - (b - b_begin);
            if (result == 0) {
                QString astr(a_begin, a - a_begin);
                QString bstr(b_begin, b - b_begin);
                long la = astr.toLong();
                long lb = bstr.toLong();
                result = la - lb;
            }
        } else {
            aa = QChar(aa).toLower().unicode();
            bb = QChar(bb).toLower().unicode();
            result = aa - bb;
            ++a;
            ++b;
        }
    }

    return result < 0 ? true : false;
}

/**
 * \a real is kinda a hack for the smart search, need a way to match a regexp to an item
 * How would you say the best match for Q.*Wiget is QWidget?
 */
QModelIndex IndexListModel::filter(const QString &s, const QString &real)
{
    QStringList list;

    int goodMatch = -1;
    int perfectMatch = -1;
    if (s.isEmpty())
        perfectMatch = 0;

    const QRegExp regExp(s);
    QMultiMap<QString, QString>::iterator it = contents.begin();
    QString lastKey;
    for (; it != contents.end(); ++it) {
        if (it.key() == lastKey)
            continue;
        lastKey = it.key();
        const QString key = it.key();
        if (key.contains(regExp) || key.contains(s, Qt::CaseInsensitive)) {
            list.append(key);
            //qDebug() << regExp << regExp.indexIn(s) << s << key << regExp.matchedLength();
            if (perfectMatch == -1 && (key.startsWith(real, Qt::CaseInsensitive))) {
                if (goodMatch == -1)
                    goodMatch = list.count() - 1;
                if (s.length() == key.length())
                    perfectMatch = list.count() - 1;
            } else if (perfectMatch > -1 && s == key) {
                perfectMatch = list.count() - 1;
            }
        }
    }
    
    int bestMatch = perfectMatch;
    if (bestMatch == -1)
        bestMatch = goodMatch;

    bestMatch = qMax(0, bestMatch);
    
    // sort the new list
    QString match;
    if (bestMatch >= 0 && list.count() > bestMatch)
        match = list[bestMatch];
    qSort(list.begin(), list.end(), caseInsensitiveLessThan);
    setStringList(list);
    for (int i = 0; i < list.size(); ++i) {
        if (list.at(i) == match){
            bestMatch = i;
            break;
        }
    }
    return index(bestMatch, 0, QModelIndex());
}



HelpEngine::HelpEngine(QObject *parent, const QString &defaultQtVersionPath)
    : QObject(parent)
{
    titleMapThread = new TitleMapThread(this);
    connect(titleMapThread, SIGNAL(errorOccured(const QString&)),
        this, SIGNAL(errorOccured(const QString&)));
    connect(titleMapThread, SIGNAL(finished()), this, SLOT(titleMapFinished()));
    indexThread = new IndexThread(this);
    connect(indexThread, SIGNAL(errorOccured(const QString&)),
        this, SIGNAL(errorOccured(const QString&)));
    connect(indexThread, SIGNAL(finished()), this, SLOT(indexFinished()));

    indexModel = new IndexListModel(this);

    Config::loadConfig(defaultQtVersionPath);
    cacheFilesPath = QDir::homePath() + QLatin1String("/.assistant");
}

HelpEngine::~HelpEngine()
{
    Config::configuration()->save();
}

void HelpEngine::init()
{
}

QString HelpEngine::cacheFilePath() const
{
    return cacheFilesPath;
}

IndexListModel *HelpEngine::indices()
{
    return indexModel;
}

void HelpEngine::buildContents()
{
    contentsOnly = true;
    if (!titleMapThread->isRunning()) {        
        titleMapThread->start(QThread::NormalPriority);        
    }
}

void HelpEngine::buildIndex()
{
    if (!titleMapThread->isRunning()) {
        contentsOnly = false;
        titleMapThread->start(QThread::NormalPriority);        
    }
    if (!indexThread->isRunning())
        indexThread->start(QThread::NormalPriority);
}

void HelpEngine::titleMapFinished()
{
    contentList = titleMapThread->contents();
    titleMap = titleMapThread->documentTitleMap();
    if (contentsOnly) {
        contentsOnly = false;
        emit contentsInitialized();
    }
}

void HelpEngine::indexFinished()
{
    indexModel = indexThread->model();
    emit indexInitialized();
}

void HelpEngine::removeOldCacheFiles(bool onlyFulltextSearchIndex)
{
    if (!verifyDirectory(cacheFilesPath)) {
        qWarning("Failed to created assistant directory");
        return;
    }
    QString pname = QLatin1String(".") + Config::configuration()->profileName();

    QStringList fileList;
    fileList << QLatin1String("indexdb40.dict")
        << QLatin1String("indexdb40.doc");

    if (!onlyFulltextSearchIndex)
        fileList << QLatin1String("indexdb40") << QLatin1String("contentdb40");

    QStringList::iterator it = fileList.begin();
    for (; it != fileList.end(); ++it) {
		if (QFile::exists(cacheFilesPath + QDir::separator() + *it + pname)) {
            QFile f(cacheFilesPath + QDir::separator() + *it + pname);
            f.remove();
        }
    }
}

quint32 HelpEngine::getFileAges()
{
    QStringList addDocuFiles = Config::configuration()->docFiles();
    QStringList::const_iterator i = addDocuFiles.begin();

    quint32 fileAges = 0;
    for (; i != addDocuFiles.end(); ++i) {
        QFileInfo fi(*i);
        if (fi.exists())
            fileAges += fi.lastModified().toTime_t();
    }

    return fileAges;
}

QString HelpEngine::removeAnchorFromLink(const QString &link)
{
    int i = link.length();
	int j = link.lastIndexOf('/');
    int l = link.lastIndexOf(QDir::separator());
    if (l > j)
        j = l;
	if (j > -1) {
		QString fileName = link.mid(j+1);
		int k = fileName.lastIndexOf('#');
		if (k > -1)
			i = j + k + 1;
	}
	return link.left(i);
}

QString HelpEngine::titleOfLink(const QString &link)
{
    QString s = HelpEngine::removeAnchorFromLink(link);
    s = titleMap[s];
    if (s.isEmpty())
        return link;
    return s;
}

QString HelpEngine::home() const
{
    QString link = Config::configuration()->homePage();
    if (!link.startsWith(QLatin1String("file:")))
        link.prepend("file:");
    return link;
}



TitleMapThread::TitleMapThread(HelpEngine *he)
    : QThread(he)
{
    engine = he;
    done = false;
}

TitleMapThread::~TitleMapThread()
{

}

void TitleMapThread::run()
{
    if (done) {
        engine->mutex.lock();
        engine->titleMapDoneCondition.wakeAll();
        engine->mutex.unlock();
        return;
    }

    bool needRebuild = false;
    if (Config::configuration()->profileName() == QLatin1String("default")) {
        const QStringList docuFiles = Config::configuration()->docFiles();
        for (QStringList::ConstIterator it = docuFiles.begin(); it != docuFiles.end(); it++) {
            if (!QFile::exists(*it)) {
                Config::configuration()->saveProfile(Profile::createDefaultProfile());
                Config::configuration()->loadDefaultProfile();
                needRebuild = true;
                break;
            }
        }
    }

    if (Config::configuration()->docRebuild() || needRebuild) {
        engine->removeOldCacheFiles();
        Config::configuration()->setDocRebuild(false);
        Config::configuration()->save();
    }
    if (contentList.isEmpty())
        getAllContents();
    
    titleMap.clear();
    for (QList<QPair<QString, ContentList> >::Iterator it = contentList.begin(); it != contentList.end(); ++it) {
        ContentList lst = (*it).second;
        foreach (ContentItem item, lst) {
            titleMap[item.reference] = item.title.trimmed();
        }
    }
    done = true;
    engine->mutex.lock();
    engine->titleMapDoneCondition.wakeAll();
    engine->mutex.unlock();
}

void TitleMapThread::getAllContents()
{
    QFile contentFile(engine->cacheFilePath() + QDir::separator() + QLatin1String("contentdb40.")
		+ Config::configuration()->profileName());
    contentList.clear();
    if (!contentFile.open(QFile::ReadOnly)) {
        buildContentDict();
        return;
    }

    QDataStream ds(&contentFile);
    quint32 fileAges;
    ds >> fileAges;
    if (fileAges != engine->getFileAges()) {
        contentFile.close();
        engine->removeOldCacheFiles(true);        
        buildContentDict();
        return;
    }
    QString key;
    QList<ContentItem> lst;
    while (!ds.atEnd()) {
        ds >> key;
        ds >> lst;
        contentList += qMakePair(key, QList<ContentItem>(lst));
    }
    contentFile.close();

}

void TitleMapThread::buildContentDict()
{
    QStringList docuFiles = Config::configuration()->docFiles();

    quint32 fileAges = 0;
    for (QStringList::iterator it = docuFiles.begin(); it != docuFiles.end(); it++) {
        QFile file(*it);
        if (!file.exists()) {
#ifdef _SHOW_ERRORS_
            emit errorOccured(tr("Documentation file %1 does not exist!\n"
                "Skipping file.").arg(QFileInfo(file).absoluteFilePath()));            
#endif
            continue;
        }
        fileAges += QFileInfo(file).lastModified().toTime_t();
        DocuParser *handler = DocuParser::createParser(*it);
        if (!handler) {
#ifdef _SHOW_ERRORS_
            emit errorOccured(tr("Documentation file %1 is not compatible!\n"
                "Skipping file.").arg(QFileInfo(file).absoluteFilePath()));
#endif
            continue;
        }
        bool ok = handler->parse(&file);
        file.close();
        if (ok) {
            contentList += qMakePair(*it, QList<ContentItem>(handler->getContentItems()));
            delete handler;
        } else {
#ifdef _SHOW_ERRORS_
            QString msg = QString::fromLatin1("In file %1:\n%2")
                          .arg(QFileInfo(file).absoluteFilePath())
                          .arg(handler->errorProtocol());
            emit errorOccured(msg);
#endif
            continue;
        }
    }

    QFile contentOut(engine->cacheFilePath() + QDir::separator() + QLatin1String("contentdb40.")
		+ Config::configuration()->profileName());
    if (contentOut.open(QFile::WriteOnly)) {
        QDataStream s(&contentOut);
        s << fileAges;
        for (QList<QPair<QString, ContentList> >::Iterator it = contentList.begin(); it != contentList.end(); ++it) {
            s << *it;
        }
        contentOut.close();
    }    
}


IndexThread::IndexThread(HelpEngine *he)
    : QThread(he)
{
    engine = he;
    indexModel = new IndexListModel(this);
    indexDone = false;
}

void IndexThread::run()
{
    if (indexDone)
        return;
    engine->mutex.lock();
    if (engine->titleMapThread->isRunning())
        engine->titleMapDoneCondition.wait(&(engine->mutex));
    engine->mutex.unlock();

    keywordDocuments.clear();
    QList<IndexKeyword> lst;
    QFile indexFile(engine->cacheFilePath() + QDir::separator() + QLatin1String("indexdb40.") +
                     Config::configuration()->profileName());
    if (!indexFile.open(QFile::ReadOnly)) {
        buildKeywordDB();
        if (!indexFile.open(QFile::ReadOnly)) {
#ifdef _SHOW_ERRORS_
            emit errorOccured(tr("Failed to load keyword index file!"));
#endif
            return;
        }
    }

    QDataStream ds(&indexFile);
    quint32 fileAges;
    ds >> fileAges;
    if (fileAges != engine->getFileAges()) {
        indexFile.close();
        buildKeywordDB();
        if (!indexFile.open(QFile::ReadOnly)) {
#ifdef _SHOW_ERRORS_
            emit errorOccured(tr("Cannot open the index file %1")
                .arg(QFileInfo(indexFile).absoluteFilePath()));
#endif
            return;
        }
        ds.setDevice(&indexFile);
        ds >> fileAges;
    }
    ds >> lst;
    indexFile.close();
    
    for (int i=0; i<lst.count(); ++i) {
        const IndexKeyword &idx = lst.at(i);
        indexModel->addLink(idx.keyword, idx.link);
        keywordDocuments << HelpEngine::removeAnchorFromLink(idx.link);    
    }
    indexModel->publish();
    indexDone = true;
}

void IndexThread::buildKeywordDB()
{
    QStringList addDocuFiles = Config::configuration()->docFiles();
    QStringList::iterator i = addDocuFiles.begin();

    int steps = 0;
    for (; i != addDocuFiles.end(); i++)
        steps += QFileInfo(*i).size();

    QList<IndexKeyword> lst;
    quint32 fileAges = 0;
    for (i = addDocuFiles.begin(); i != addDocuFiles.end(); i++) {
        QFile file(*i);
        if (!file.exists()) {
#ifdef _SHOW_ERRORS_
            emit errorOccured(tr("Documentation file %1 does not exist!\n"
                    "Skipping file.").arg(QFileInfo(file).absoluteFilePath()));
#endif
            continue;
        }
        fileAges += QFileInfo(file).lastModified().toTime_t();
        DocuParser *handler = DocuParser::createParser(*i);
        bool ok = handler->parse(&file);
        file.close();
        if (!ok){
#ifdef _SHOW_ERRORS_
            QString msg = QString::fromLatin1("In file %1:\n%2")
                          .arg(QFileInfo(file).absoluteFilePath())
                          .arg(handler->errorProtocol());
            emit errorOccured(msg);
#endif
            delete handler;
            continue;
        }

        QList<IndexItem*> indLst = handler->getIndexItems();
        foreach (IndexItem *indItem, indLst) {
            QFileInfo fi(indItem->reference);
            lst.append(IndexKeyword(indItem->keyword, indItem->reference));            
        }
        delete handler;
    }
    if (!lst.isEmpty())
        qSort(lst);

    QFile indexout(engine->cacheFilePath() + QDir::separator() + QLatin1String("indexdb40.")
		+ Config::configuration()->profileName());
    if (verifyDirectory(engine->cacheFilePath()) && indexout.open(QFile::WriteOnly)) {
        QDataStream s(&indexout);
        s << fileAges;
        s << lst;
        indexout.close();
    }
}
