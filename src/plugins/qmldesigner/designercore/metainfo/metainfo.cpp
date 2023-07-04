// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "metainfo.h"

#include "modelnode.h"
#include "metainforeader.h"
#include "iwidgetplugin.h"

#include <externaldependenciesinterface.h>
#include <invalidmetainfoexception.h>

#include <coreplugin/messagebox.h>
#include <coreplugin/icore.h>

#include <utils/environment.h>
#include <utils/filepath.h>

#include "pluginmanager/widgetpluginmanager.h"

#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QDirIterator>
#include <QMutex>

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {

static QString globalMetaInfoPath(const ExternalDependenciesInterface &externalDependecies)
{
#ifdef SHARE_QML_PATH
    if (Utils::qtcEnvironmentVariableIsSet("LOAD_QML_FROM_SOURCE"))
        return QLatin1String(SHARE_QML_PATH) + "/globalMetaInfo";
#endif
    return externalDependecies.resourcePath("qmldesigner/globalMetaInfo").toString();
}

Utils::FilePaths allGlobalMetaInfoFiles(const ExternalDependenciesInterface &externalDependecies)
{
    static Utils::FilePaths paths;

    if (!paths.isEmpty())
        return paths;

    QDirIterator it(globalMetaInfoPath(externalDependecies),
                    {"*.metainfo"},
                    QDir::Files,
                    QDirIterator::Subdirectories);
    while (it.hasNext())
        paths.append(Utils::FilePath::fromString(it.next()));

    return paths;
}

class MetaInfoPrivate
{
    Q_DISABLE_COPY(MetaInfoPrivate)
public:
    MetaInfoPrivate(MetaInfo *q);
    void clear();

    void initialize(const ExternalDependenciesInterface &externalDependencies);

    void parseItemLibraryDescriptions(const ExternalDependenciesInterface &externalDependencies);

    QScopedPointer<ItemLibraryInfo> m_itemLibraryInfo;

    MetaInfo *m_q;
    bool m_isInitialized;
};

MetaInfoPrivate::MetaInfoPrivate(MetaInfo *q)
    : m_itemLibraryInfo(new ItemLibraryInfo())
    , m_q(q)
    , m_isInitialized(false)
{
    if (!m_q->isGlobal())
        m_itemLibraryInfo->setBaseInfo(MetaInfo::global().itemLibraryInfo());
}

void MetaInfoPrivate::clear()
{
    m_itemLibraryInfo->clearEntries();
    m_isInitialized = false;
}

namespace {
bool enableParseItemLibraryDescriptions = true;
}

void MetaInfoPrivate::initialize(const ExternalDependenciesInterface &externalDependencies)
{
    if (enableParseItemLibraryDescriptions)
        parseItemLibraryDescriptions(externalDependencies);
    m_isInitialized = true;
}

void MetaInfoPrivate::parseItemLibraryDescriptions(const ExternalDependenciesInterface &externalDependencies)
{
    Internal::WidgetPluginManager pluginManager;
    for (const QString &pluginDir : std::as_const(m_q->s_pluginDirs))
        pluginManager.addPath(pluginDir);
    const QList<IWidgetPlugin *> widgetPluginList = pluginManager.instances();
    for (IWidgetPlugin *plugin : widgetPluginList) {
        Internal::MetaInfoReader reader(*m_q);
        try {
            reader.readMetaInfoFile(plugin->metaInfo());
        } catch ([[maybe_unused]] const InvalidMetaInfoException &e) {
#ifndef UNIT_TESTS
            qWarning() << e.description();
            const QString errorMessage = plugin->metaInfo() + QLatin1Char('\n') + QLatin1Char('\n')
                                         + reader.errors().join(QLatin1Char('\n'));
            Core::AsynchronousMessageBox::warning(
                QCoreApplication::translate("QmlDesigner::Internal::MetaInfoPrivate",
                                            "Invalid meta info"),
                errorMessage);
#endif
        }
    }

    const Utils::FilePaths allMetaInfoFiles = allGlobalMetaInfoFiles(externalDependencies);
    for (const Utils::FilePath &path : allMetaInfoFiles) {
        Internal::MetaInfoReader reader(*m_q);
        try {
            reader.readMetaInfoFile(path.toString());
        } catch ([[maybe_unused]] const InvalidMetaInfoException &e) {
#ifndef UNIT_TESTS
            qWarning() << e.description();
            const QString errorMessage = path.toString() + QLatin1Char('\n') + QLatin1Char('\n')
                                         + reader.errors().join(QLatin1Char('\n'));
            Core::AsynchronousMessageBox::warning(
                QCoreApplication::translate("QmlDesigner::Internal::MetaInfoPrivate",
                                            "Invalid meta info"),
                errorMessage);
#endif
        }
    }
}

} // namespace Internal

void MetaInfo::disableParseItemLibraryDescriptionsUgly()
{
    Internal::enableParseItemLibraryDescriptions = false;
}

using QmlDesigner::Internal::MetaInfoPrivate;

MetaInfo MetaInfo::s_global;
QMutex s_lock;
QStringList MetaInfo::s_pluginDirs;


/*!
\class QmlDesigner::MetaInfo
\ingroup CoreModel
\brief The MetaInfo class provides meta information about QML types and
properties.
*/

/*!
  Constructs a copy of \a metaInfo.
  */
MetaInfo::MetaInfo(const MetaInfo &metaInfo) = default;

/*!
  Creates a meta information object with just the QML types registered statically.
  You almost always want to use Model::metaInfo() instead.

  You almost certainly want to access the meta information for the model.

  \sa Model::metaInfo()
  */
MetaInfo::MetaInfo() :
        m_p(new MetaInfoPrivate(this))
{}

MetaInfo::~MetaInfo() = default;

/*!
  Assigns \a other to this meta information and returns a reference to this
  meta information.
  */
MetaInfo& MetaInfo::operator=(const MetaInfo &other) = default;

ItemLibraryInfo *MetaInfo::itemLibraryInfo() const
{
    return m_p->m_itemLibraryInfo.data();
}

void MetaInfo::initializeGlobal(const QStringList &pluginPaths,
                                const ExternalDependenciesInterface &externalDependencies)
{
    QMutexLocker locker(&s_lock);

    if (!s_global.m_p->m_isInitialized) {
        s_pluginDirs = pluginPaths,
        s_global.m_p = QSharedPointer<MetaInfoPrivate>(new MetaInfoPrivate(&s_global));
        s_global.m_p->initialize(externalDependencies);
    }
}

bool MetaInfo::isGlobal() const
{
    return (this->m_p == s_global.m_p);
}

MetaInfo MetaInfo::global()
{
    return s_global;
}

bool operator==(const MetaInfo &first, const MetaInfo &second)
{
    return first.m_p == second.m_p;
}

bool operator!=(const MetaInfo &first, const MetaInfo &second)
{
    return !(first == second);
}

} //namespace QmlDesigner
