// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "profilereader.h"

#include "qtsupporttr.h"

#include <coreplugin/icore.h>
#include <projectexplorer/taskhub.h>

#include <QCoreApplication>
#include <QDebug>

using namespace ProjectExplorer;
using namespace QtSupport;

static QString format(const QString &fileName, int lineNo, const QString &msg)
{
    if (lineNo > 0)
        return QString::fromLatin1("%1(%2): %3").arg(fileName, QString::number(lineNo), msg);
    else if (!fileName.isEmpty())
        return QString::fromLatin1("%1: %2").arg(fileName, msg);
    else
        return msg;
}

ProMessageHandler::ProMessageHandler(bool verbose, bool exact)
    : m_verbose(verbose)
    , m_exact(exact)
    //: Prefix used for output from the cumulative evaluation of project files.
    , m_prefix(Tr::tr("[Inexact] "))
{
}

ProMessageHandler::~ProMessageHandler()
{
    if (!m_messages.isEmpty())
        Core::MessageManager::writeFlashing(m_messages);
}

static void addTask(Task::TaskType type,
                    const QString &description,
                    const Utils::FilePath &file = {},
                    int line = -1)
{
    QMetaObject::invokeMethod(TaskHub::instance(), [=]() {
        TaskHub::addTask(BuildSystemTask(type, description, file, line));
    });
}

void ProMessageHandler::message(int type, const QString &msg, const QString &fileName, int lineNo)
{
    if ((type & CategoryMask) == ErrorMessage && ((type & SourceMask) == SourceParser || m_verbose)) {
        // parse error in qmake files
        if (m_exact) {
            addTask(Task::Error, msg, Utils::FilePath::fromString(fileName), lineNo);
        } else {
            appendMessage(format(fileName, lineNo, msg));
        }
    }
}

void ProMessageHandler::fileMessage(int type, const QString &msg)
{
    // message(), warning() or error() calls in qmake files
    if (!m_verbose)
        return;
    if (m_exact && type == QMakeHandler::ErrorMessage)
        addTask(Task::Error, msg);
    else if (m_exact && type == QMakeHandler::WarningMessage)
        addTask(Task::Warning, msg);
    else
        appendMessage(msg);
}

void ProMessageHandler::appendMessage(const QString &msg)
{
    m_messages << (m_exact ? msg : m_prefix + msg);
}

ProFileReader::ProFileReader(QMakeGlobals *option, QMakeVfs *vfs)
    : QMakeParser(ProFileCacheManager::instance()->cache(), vfs, this)
    , ProFileEvaluator(option, this, vfs, this)
    , m_ignoreLevel(0)
{
    setExtraConfigs(QStringList("qtc_run"));
}

ProFileReader::~ProFileReader()
{
    for (ProFile *pf : std::as_const(m_proFiles))
        pf->deref();
}

void ProFileReader::setCumulative(bool on)
{
    ProMessageHandler::setVerbose(!on);
    ProMessageHandler::setExact(!on);
    ProFileEvaluator::setCumulative(on);
}

void ProFileReader::aboutToEval(ProFile *parent, ProFile *pro, EvalFileType type)
{
    if (m_ignoreLevel || (type != EvalProjectFile && type != EvalIncludeFile)) {
        m_ignoreLevel++;
    } else if (parent) {  // Skip the actual .pro file, as nobody needs that.
        QVector<ProFile *> &children = m_includeFiles[parent];
        if (!children.contains(pro)) {
            children.append(pro);
            m_proFiles.append(pro);
            pro->ref();
        }
    }
}

void ProFileReader::doneWithEval(ProFile *)
{
    if (m_ignoreLevel)
        m_ignoreLevel--;
}

QHash<ProFile *, QVector<ProFile *> > ProFileReader::includeFiles() const
{
    return m_includeFiles;
}

ProFileCacheManager *ProFileCacheManager::s_instance = nullptr;

ProFileCacheManager::ProFileCacheManager(QObject *parent) :
    QObject(parent)
{
    s_instance = this;
    m_timer.setInterval(5000);
    m_timer.setSingleShot(true);
    connect(&m_timer, &QTimer::timeout,
            this, &ProFileCacheManager::clear);
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
    s_instance = nullptr;
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
    m_cache = nullptr;
}

void ProFileCacheManager::discardFiles(const QString &device, const QString &prefix, QMakeVfs *vfs)
{
    if (m_cache)
        m_cache->discardFiles(device, prefix, vfs);
}

void ProFileCacheManager::discardFile(const QString &device, const QString &fileName, QMakeVfs *vfs)
{
    if (m_cache)
        m_cache->discardFile(device, fileName, vfs);
}
