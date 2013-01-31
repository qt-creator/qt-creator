/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "metainfo.h"

#include "abstractproperty.h"
#include "modelnode.h"
#include "invalidmodelnodeexception.h"
#include "invalidargumentexception.h"
#include "metainforeader.h"
#include "iwidgetplugin.h"

#include "pluginmanager/widgetpluginmanager.h"


#include <QDebug>
#include <QMessageBox>
#include <QPair>
#include <QtAlgorithms>

enum {
    debug = false
};

namespace QmlDesigner {
namespace Internal {

class MetaInfoPrivate
{
    Q_DISABLE_COPY(MetaInfoPrivate)
public:
    typedef QSharedPointer<MetaInfoPrivate> Pointer;
    typedef QWeakPointer<MetaInfoPrivate> WeakPointer;


    MetaInfoPrivate(MetaInfo *q);
    void clear();

    void initialize();

    void parseItemLibraryDescriptions();

    QScopedPointer<ItemLibraryInfo> m_itemLibraryInfo;

    MetaInfo *m_q;
    bool m_isInitialized;
};

MetaInfoPrivate::MetaInfoPrivate(MetaInfo *q) :
        m_itemLibraryInfo(new ItemLibraryInfo()),
        m_q(q),
        m_isInitialized(false)
{
    if (!m_q->isGlobal())
        m_itemLibraryInfo->setBaseInfo(MetaInfo::global().itemLibraryInfo());
}

void MetaInfoPrivate::clear()
{
    m_itemLibraryInfo->clearEntries();
    m_isInitialized = false;
}

void MetaInfoPrivate::initialize()
{
    parseItemLibraryDescriptions();
    m_isInitialized = true;
}

QString static inline stripPrefix(const QString &typeName)
{
    QStringList list = typeName.split('.');
    if (list.count() == 2)
        return list.last();
    return typeName;
}

static inline bool isDepricatedQtType(const QString &typeName)
{
    if (typeName.length() < 8)
        return false;

    return typeName.contains("Qt.");
}

void MetaInfoPrivate::parseItemLibraryDescriptions()
{
    Internal::WidgetPluginManager pluginManager;
    foreach (const QString &pluginDir, m_q->s_pluginDirs)
        pluginManager.addPath(pluginDir);
    QList<IWidgetPlugin *> widgetPluginList = pluginManager.instances();
    foreach (IWidgetPlugin *plugin, widgetPluginList) {
        Internal::MetaInfoReader reader(*m_q);
        try {
            reader.readMetaInfoFile(plugin->metaInfo());
        } catch (InvalidMetaInfoException &e) {
            qWarning() << e.description();
            const QString errorMessage = plugin->metaInfo() + QLatin1Char('\n') + QLatin1Char('\n') + reader.errors().join(QLatin1String("\n"));
            QMessageBox::critical(0,
                                  QCoreApplication::translate("QmlDesigner::Internal::MetaInfoPrivate", "Invalid meta info"),
                                  errorMessage);
        }
    }
}

} // namespace Internal

using QmlDesigner::Internal::MetaInfoPrivate;

MetaInfo MetaInfo::s_global;
QStringList MetaInfo::s_pluginDirs;


/*!
\class QmlDesigner::MetaInfo
\ingroup CoreModel
\brief The MetaInfo class provides meta information about qml types and properties.
*/

/*!
  \brief Constructs a copy of the given meta info.
  */
MetaInfo::MetaInfo(const MetaInfo &metaInfo) :
        m_p(metaInfo.m_p)
{
}

/*!
  \brief Creates a meta information object with just the qml types registered statically.
  You almost always want to use Model::metaInfo() instead!

  You almost certainly want to access the meta information for the model.

  \see Model::metaInfo()
  */
MetaInfo::MetaInfo() :
        m_p(new MetaInfoPrivate(this))
{
}

MetaInfo::~MetaInfo()
{
}

/*!
  \brief Assigns other to this meta information and returns a reference to this meta information.
  */
MetaInfo& MetaInfo::operator=(const MetaInfo &other)
{
    m_p = other.m_p;
    return *this;
}

ItemLibraryInfo *MetaInfo::itemLibraryInfo() const
{
    return m_p->m_itemLibraryInfo.data();
}

/*!
  \brief Access to the global meta information object.
  You almost always want to use Model::metaInfo() instead.

  Internally all meta information objects share this "global" object
  where static qml type information is stored.
  */
MetaInfo MetaInfo::global()
{
    if (!s_global.m_p->m_isInitialized) {
        s_global.m_p = QSharedPointer<MetaInfoPrivate>(new MetaInfoPrivate(&s_global));
        s_global.m_p->initialize();
    }
    return s_global;
}

/*!
  \brief Clears the global meta information object.

  This method should be called once on application shutdown to free static data structures.
  */
void MetaInfo::clearGlobal()
{
    if (s_global.m_p->m_isInitialized)
        s_global.m_p->clear();
}

void MetaInfo::setPluginPaths(const QStringList &paths)
{
    s_pluginDirs = paths;
}

bool MetaInfo::isGlobal() const
{
    return (this->m_p == s_global.m_p);
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
