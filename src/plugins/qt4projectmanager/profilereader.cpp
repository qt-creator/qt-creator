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

#include "profilereader.h"

#include <QtCore/QDir>
#include <QtCore/QDebug>

using namespace Qt4ProjectManager;
using namespace Qt4ProjectManager::Internal;

ProFileReader::ProFileReader(ProFileOption *option) : ProFileEvaluator(option)
{
}

ProFileReader::~ProFileReader()
{
    foreach (ProFile *pf, m_proFiles)
        pf->deref();
}

bool ProFileReader::readProFile(const QString &fileName)
{
    if (ProFile *pro = parsedProFile(fileName)) {
        aboutToEval(pro);
        bool ok = accept(pro);
        pro->deref();
        return ok;
    }
    return false;
}

void ProFileReader::aboutToEval(ProFile *pro)
{
    if (!m_includeFiles.contains(pro->fileName())) {
        m_includeFiles.insert(pro->fileName(), pro);
        m_proFiles.append(pro);
        pro->ref();
    }
}

QList<ProFile*> ProFileReader::includeFiles() const
{
    QString qmakeMkSpecDir = QFileInfo(propertyValue("QMAKE_MKSPECS")).absoluteFilePath();
    QList<ProFile *> list;
    QMap<QString, ProFile *>::const_iterator it, end;
    end = m_includeFiles.constEnd();
    for (it = m_includeFiles.constBegin(); it != end; ++it) {
        if (!QFileInfo((it.key())).absoluteFilePath().startsWith(qmakeMkSpecDir))
            list.append(it.value());
    }
    return list;
}

QString ProFileReader::value(const QString &variable) const
{
    QStringList vals = values(variable);
    if (!vals.isEmpty())
        return vals.first();

    return QString();
}


void ProFileReader::fileMessage(const QString &message)
{
    Q_UNUSED(message)
    // we ignore these...
}

void ProFileReader::logMessage(const QString &message)
{
    Q_UNUSED(message)
    // we ignore these...
}

void ProFileReader::errorMessage(const QString &message)
{
    emit errorFound(message);
}

ProFile *ProFileReader::proFileFor(const QString &name)
{
    return m_includeFiles.value(name);
}

ProFileCacheManager *ProFileCacheManager::s_instance = 0;

ProFileCacheManager::ProFileCacheManager(QObject *parent) :
        QObject(parent),
        m_cache(0),
        m_refCount(0)
{
    s_instance = this;
    m_timer.setInterval(5000);
    m_timer.setSingleShot(true);
    connect(&m_timer, SIGNAL(timeout()),
            this, SLOT(clear()));
}

void ProFileCacheManager::incRefCount()
{
    ++m_refCount;
    m_timer.stop();
}

void ProFileCacheManager::decRefCount()
{
    --m_refCount;
    if (!m_refCount)
        m_timer.start();
}

ProFileCacheManager::~ProFileCacheManager()
{
    s_instance = 0;
    clear();
}

ProFileCache *ProFileCacheManager::cache()
{
    if (!m_cache)
        m_cache = new ProFileCache;
    return m_cache;
}

void ProFileCacheManager::clear()
{
    Q_ASSERT(m_refCount == 0);
    // Just deleting the cache will be safe as long as the sequence of
    // obtaining a cache pointer and using it is atomic as far as the main
    // loop is concerned. Use a shared pointer once this is not true anymore.
    delete m_cache;
    m_cache = 0;
}

void ProFileCacheManager::discardFiles(const QString &prefix)
{
    if (m_cache)
        m_cache->discardFiles(prefix);
}

void ProFileCacheManager::discardFile(const QString &fileName)
{
    if (m_cache)
        m_cache->discardFile(fileName);
}
