/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "spotlightlocatorfilter.h"

#include "../messagemanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/reaper.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/qtcassert.h>

#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QTimer>
#include <QWaitCondition>

using namespace Utils;

namespace Core {
namespace Internal {

// #pragma mark -- SpotlightIterator

class SpotlightIterator : public BaseFileFilter::Iterator
{
public:
    SpotlightIterator(const QStringList &command);
    ~SpotlightIterator() override;

    void toFront() override;
    bool hasNext() const override;
    Utils::FilePath next() override;
    Utils::FilePath filePath() const override;

    void scheduleKillProcess();
    void killProcess();

private:
    void ensureNext();

    std::unique_ptr<QProcess> m_process;
    QMutex m_mutex;
    QWaitCondition m_waitForItems;
    QList<FilePath> m_queue;
    QList<FilePath> m_filePaths;
    int m_index;
    bool m_finished;
};

SpotlightIterator::SpotlightIterator(const QStringList &command)
    : m_index(-1)
    , m_finished(false)
{
    QTC_ASSERT(!command.isEmpty(), return );
    m_process.reset(new QProcess);
    m_process->setProgram(
        Utils::Environment::systemEnvironment().searchInPath(command.first()).toString());
    m_process->setArguments(command.mid(1));
    m_process->setProcessEnvironment(Utils::Environment::systemEnvironment().toProcessEnvironment());
    QObject::connect(m_process.get(),
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [this] { scheduleKillProcess(); });
    QObject::connect(m_process.get(), &QProcess::errorOccurred, [this, command] {
        MessageManager::writeFlashing(
            SpotlightLocatorFilter::tr("Locator: Error occurred when running \"%1\".")
                .arg(command.first()));
        scheduleKillProcess();
    });
    QObject::connect(m_process.get(), &QProcess::readyReadStandardOutput, [this] {
        const QStringList items = QString::fromUtf8(m_process->readAllStandardOutput()).split('\n');
        QMutexLocker lock(&m_mutex);
        m_queue.append(Utils::transform(items, &FilePath::fromString));
        if (m_filePaths.size() + m_queue.size() > 10000) // limit the amount of data
            scheduleKillProcess();
        m_waitForItems.wakeAll();
    });
    m_process->start(QIODevice::ReadOnly);
}

SpotlightIterator::~SpotlightIterator()
{
    killProcess();
}

void SpotlightIterator::toFront()
{
    m_index = -1;
}

bool SpotlightIterator::hasNext() const
{
    auto that = const_cast<SpotlightIterator *>(this);
    that->ensureNext();
    return (m_index + 1 < m_filePaths.size());
}

Utils::FilePath SpotlightIterator::next()
{
    ensureNext();
    ++m_index;
    QTC_ASSERT(m_index < m_filePaths.size(), return FilePath());
    return m_filePaths.at(m_index);
}

Utils::FilePath SpotlightIterator::filePath() const
{
    QTC_ASSERT(m_index < m_filePaths.size(), return FilePath());
    return m_filePaths.at(m_index);
}

void SpotlightIterator::scheduleKillProcess()
{
    QTimer::singleShot(0, m_process.get(), [this] { killProcess(); });
}

void SpotlightIterator::killProcess()
{
    if (!m_process)
        return;
    m_process->disconnect();
    QMutexLocker lock(&m_mutex);
    m_finished = true;
    m_waitForItems.wakeAll();
    if (m_process->state() == QProcess::NotRunning)
        m_process.reset();
    else
        Reaper::reap(m_process.release());
}

void SpotlightIterator::ensureNext()
{
    if (m_index + 1 < m_filePaths.size()) // nothing to do
        return;
    // check if there are items in the queue, otherwise wait for some
    m_mutex.lock();
    if (m_queue.isEmpty() && !m_finished)
        m_waitForItems.wait(&m_mutex);
    m_filePaths.append(m_queue);
    m_queue.clear();
    m_mutex.unlock();
}

// #pragma mark -- SpotlightLocatorFilter

SpotlightLocatorFilter::SpotlightLocatorFilter()
{
    setId("SpotlightFileNamesLocatorFilter");
    setDisplayName(tr("Spotlight File Name Index"));
    setShortcutString("md");
}

void SpotlightLocatorFilter::prepareSearch(const QString &entry)
{
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);
    if (fp.filePath.isEmpty()) {
        setFileIterator(new BaseFileFilter::ListIterator(Utils::FilePaths()));
    } else {
        // only pass the file name part to spotlight to allow searches like "somepath/*foo"
        int lastSlash = fp.filePath.lastIndexOf(QLatin1Char('/'));
        QString quoted = fp.filePath.mid(lastSlash + 1);
        quoted.replace('\\', "\\\\").replace('\'', "\\\'").replace('\"', "\\\"");
        setFileIterator(new SpotlightIterator(
            {"mdfind",
             QString("kMDItemFSName = '*%1*'%2")
                 .arg(quoted,
                      caseSensitivity(fp.filePath) == Qt::CaseInsensitive ? QString("c")
                                                                          : QString())}));
    }
    BaseFileFilter::prepareSearch(entry);
}

void SpotlightLocatorFilter::refresh(QFutureInterface<void> &future)
{
    Q_UNUSED(future)
}

} // Internal
} // Core
