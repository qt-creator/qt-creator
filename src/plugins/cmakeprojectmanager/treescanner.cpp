/****************************************************************************
**
** Copyright (C) 2016 Alexander Drozdov.
** Contact: Alexander Drozdov (adrozdoff@gmail.com)
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

#include "treescanner.h"

#include <projectexplorer/projectexplorerconstants.h>

#include <cpptools/cpptoolsconstants.h>

#include <utils/qtcassert.h>
#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <memory>

using namespace ProjectExplorer;

namespace CMakeProjectManager {
namespace Internal {

TreeScanner::TreeScanner(QObject *parent) : QObject(parent)
{
    m_factory = TreeScanner::genericFileType;
    m_filter = [](const Utils::MimeType &mimeType, const Utils::FileName &fn) {
        return isWellKnownBinary(mimeType, fn) && isMimeBinary(mimeType, fn);
    };

    connect(&m_futureWatcher, &FutureWatcher::finished, this, &TreeScanner::finished);
}

TreeScanner::~TreeScanner()
{
    if (!m_futureWatcher.isFinished()) {
        m_futureWatcher.cancel();
        m_futureWatcher.waitForFinished();
    }
}

bool TreeScanner::asyncScanForFiles(const Utils::FileName &directory)
{
    if (!m_futureWatcher.isFinished())
        return false;

    auto fi = new FutureInterface();
    m_scanFuture = fi->future();
    m_futureWatcher.setFuture(m_scanFuture);

    Utils::runAsync([this, fi, directory]() { TreeScanner::scanForFiles(fi, directory, m_filter, m_factory); });

    return true;
}

void TreeScanner::setFilter(TreeScanner::FileFilter filter)
{
    if (isFinished())
        m_filter = filter;
}

void TreeScanner::setTypeFactory(TreeScanner::FileTypeFactory factory)
{
    if (isFinished())
        m_factory = factory;
}

TreeScanner::Future TreeScanner::future() const
{
    return m_scanFuture;
}

bool TreeScanner::isFinished() const
{
    return m_futureWatcher.isFinished();
}

TreeScanner::Result TreeScanner::result() const
{
    if (isFinished())
        return m_scanFuture.result();
    return Result();
}

TreeScanner::Result TreeScanner::release()
{
    if (isFinished()) {
        auto result = m_scanFuture.result();
        m_scanFuture = Future();
        return result;
    }
    return Result();
}

void TreeScanner::reset()
{
    if (isFinished())
        m_scanFuture = Future();
}

bool TreeScanner::isWellKnownBinary(const Utils::MimeType & /*mdb*/, const Utils::FileName &fn)
{
    return fn.endsWith(QLatin1String(".a")) ||
            fn.endsWith(QLatin1String(".o")) ||
            fn.endsWith(QLatin1String(".d")) ||
            fn.endsWith(QLatin1String(".exe")) ||
            fn.endsWith(QLatin1String(".dll")) ||
            fn.endsWith(QLatin1String(".obj")) ||
            fn.endsWith(QLatin1String(".elf"));
}

bool TreeScanner::isMimeBinary(const Utils::MimeType &mimeType, const Utils::FileName &/*fn*/)
{
    bool isBinary = false;
    if (mimeType.isValid()) {
        QStringList mimes;
        mimes << mimeType.name() << mimeType.allAncestors();
        isBinary = !mimes.contains(QLatin1String("text/plain"));
    }
    return isBinary;
}

FileType TreeScanner::genericFileType(const Utils::MimeType &mimeType, const Utils::FileName &/*fn*/)
{
    FileType type = FileType::Unknown;
    if (mimeType.isValid()) {
        const QString mt = mimeType.name();
        if (mt == CppTools::Constants::C_SOURCE_MIMETYPE
            || mt == CppTools::Constants::CPP_SOURCE_MIMETYPE
            || mt == CppTools::Constants::OBJECTIVE_C_SOURCE_MIMETYPE
            || mt == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE
            || mt == CppTools::Constants::QDOC_MIMETYPE
            || mt == CppTools::Constants::MOC_MIMETYPE)
            type = FileType::Source;
        else if (mt == CppTools::Constants::C_HEADER_MIMETYPE
                 || mt == CppTools::Constants::CPP_HEADER_MIMETYPE)
            type = FileType::Header;
        else if (mt == ProjectExplorer::Constants::FORM_MIMETYPE)
            type = FileType::Form;
        else if (mt == ProjectExplorer::Constants::RESOURCE_MIMETYPE)
            type = FileType::Resource;
        else if (mt == ProjectExplorer::Constants::SCXML_MIMETYPE)
            type = FileType::StateChart;
        else if (mt == ProjectExplorer::Constants::QML_MIMETYPE)
            type = FileType::QML;
    }
    return type;
}

void TreeScanner::scanForFiles(FutureInterface *fi, const Utils::FileName& directory, const FileFilter &filter, const FileTypeFactory &factory)
{
    std::unique_ptr<FutureInterface> fip(fi);
    fip->reportStarted();
    Utils::MimeDatabase mdb;

    Result nodes
            = FileNode::scanForFiles(directory,
                                     [&mdb,&filter,&factory](const Utils::FileName &fn) -> FileNode * {
        QTC_ASSERT(!fn.isEmpty(), return nullptr);

        const Utils::MimeType mimeType = mdb.mimeTypeForFile(fn.toString());

        // Skip some files during scan.
        // Filter out nullptr records after.
        if (filter && filter(mimeType, fn))
            return nullptr;

        // Type detection
        FileType type = FileType::Unknown;
        if (factory)
            type = factory(mimeType, fn);

        return new FileNode(fn, type, false);
    },
    fip.get());

    // Clean up nodes and keep it sorted
    Result tmp = Utils::filtered(nodes, [](const FileNode *fn) -> bool {
        // Simple skip null entries
        // TODO: fix Node::scanForFiles() to skip null factory results
        return fn;
    });
    Utils::sort(tmp, ProjectExplorer::Node::sortByPath);

    fip->setProgressValue(fip->progressMaximum());
    fip->reportResult(tmp);
    fip->reportFinished();
}

} // namespace Internal
} // namespace CMakeProjectManager
