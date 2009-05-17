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

#include "speinfo.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDebug>
#include <QtCore/QVariant>

using namespace Qt4ProjectManager::Internal;

bool SPEInfo::m_listsInitialized = false;
QList<SPEInfoItem*> SPEInfo::m_qtmoduleList;

QHash<QPair<SPEInfoItem::InfoKind, QString> ,SPEInfoItem*> SPEInfo::m_itemHash;

const QString SPEInfoItem::keyType("valuetype");
const QString SPEInfoItem::valueFile("file");
const QString SPEInfoItem::valuePath("path");
const QString SPEInfoItem::keyIncludedByDefault("includedbydefault");
const QString SPEInfoItem::keyImageFileName("imagefilename");

class InfoItemModulesCore : public SPEInfoItem
{
public:
    InfoItemModulesCore(): SPEInfoItem("core", QtModule)
    {
        m_data.insert(keyIncludedByDefault, true);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtCore Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Core non-GUI classes used by other modules");
    }
};

class InfoItemModulesGui : public SPEInfoItem
{
public:
    InfoItemModulesGui(): SPEInfoItem("gui", QtModule)
    {
        m_data.insert(keyIncludedByDefault, true);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtGui Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Graphical user interface components");
    }
};

class InfoItemModulesNetwork : public SPEInfoItem
{
public:
    InfoItemModulesNetwork(): SPEInfoItem("network", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtNetwork Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for network programming");
    }
};

class InfoItemModulesOpenGL : public SPEInfoItem
{
public:
    InfoItemModulesOpenGL(): SPEInfoItem("opengl", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtOpenGL Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "OpenGL support classes");
    }
};

class InfoItemModulesSql : public SPEInfoItem
{
public:
    InfoItemModulesSql(): SPEInfoItem("sql", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtSql Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for database integration using SQL");
    }
};

class InfoItemModulesScript : public SPEInfoItem
{
public:
    InfoItemModulesScript(): SPEInfoItem("script", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtScript Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for evaluating Qt Scripts");
    }
};

class InfoItemModulesSvg : public SPEInfoItem
{
public:
    InfoItemModulesSvg(): SPEInfoItem("svg", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtSvg Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for displaying the contents of SVG files");
    }
};

class InfoItemModulesWebKit : public SPEInfoItem
{
public:
    InfoItemModulesWebKit(): SPEInfoItem("webkit", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtWebKit Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for displaying and editing Web content");
    }
};

class InfoItemModulesXml : public SPEInfoItem
{
public:
    InfoItemModulesXml(): SPEInfoItem("xml", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtXml Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for handling XML");
    }
};

class InfoItemModulesXmlPatterns : public SPEInfoItem
{
public:
    InfoItemModulesXmlPatterns(): SPEInfoItem("xmlpatterns", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtXmlPatterns Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "An XQuery/XPath engine for XML and custom data models");
    }
};

class InfoItemModulesPhonon : public SPEInfoItem
{
public:
    InfoItemModulesPhonon(): SPEInfoItem("phonon", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "Phonon Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Multimedia framework classes");
    }
};

class InfoItemModulesQt3Support : public SPEInfoItem
{
public:
    InfoItemModulesQt3Support(): SPEInfoItem("qt3support", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "Qt3Support Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes that ease porting from Qt 3 to Qt 4");
    }
};

class InfoItemModulesTest : public SPEInfoItem
{
public:
    InfoItemModulesTest(): SPEInfoItem("testlib", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtTest Module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Tool classes for unit testing");
    }
};

class InfoItemModulesDBus : public SPEInfoItem
{
public:
    InfoItemModulesDBus(): SPEInfoItem("dbus", QtModule)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name() const { return QCoreApplication::translate("SimpleProEditor", "QtDBus module"); }
    QString description() const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for Inter-Process Communication using the D-Bus");
    }
};

SPEInfoItem::SPEInfoItem(const QString &id, InfoKind kind)
:   m_id(id)
,   m_infoKind(kind)
,   m_parentItem(0)
{
}

QString SPEInfoItem::name() const
{
    return "";
}

QString SPEInfoItem::description() const
{
    return "";
}

QVariant SPEInfoItem::data(const QString &key) const
{
    return m_data.value(key);
}

QString SPEInfoItem::id() const
{
    return m_id;
}

SPEInfoItem::InfoKind SPEInfoItem::infoKind() const
{
    return m_infoKind;
}

SPEInfo::~SPEInfo()
{
    deleteLists();
}

const QList<SPEInfoItem*> *SPEInfo::qtModulesList()
{
    if (!m_listsInitialized)
        initializeLists();
    return &m_qtmoduleList;
}

void SPEInfo::addListToHash(const QList<SPEInfoItem*> &list)
{
    foreach (SPEInfoItem *item, list)
        m_itemHash.insert(qMakePair(item->infoKind(), item->id()), item);
}

void SPEInfo::initializeLists()
{
    m_qtmoduleList
        << new InfoItemModulesCore
        << new InfoItemModulesGui
        << new InfoItemModulesNetwork
        << new InfoItemModulesOpenGL
        << new InfoItemModulesScript
        << new InfoItemModulesSql
        << new InfoItemModulesSvg
        << new InfoItemModulesWebKit
        << new InfoItemModulesXml
        << new InfoItemModulesXmlPatterns
        << new InfoItemModulesPhonon
        << new InfoItemModulesQt3Support
        << new InfoItemModulesTest
        << new InfoItemModulesDBus;
    addListToHash(m_qtmoduleList);

    m_listsInitialized = true;
}

void SPEInfo::deleteLists()
{
    m_itemHash.clear();

    qDeleteAll(m_qtmoduleList);
    m_qtmoduleList.clear();

    m_listsInitialized = false;
}

const SPEInfoItem *SPEInfo::infoOfKindForId(SPEInfoItem::InfoKind kind,
                                            const QString &id, const SPEInfoItem *defaultInfoItem)
{
    QPair<SPEInfoItem::InfoKind, QString > keyPair = qMakePair(kind, id);
    return m_itemHash.contains(keyPair)?m_itemHash.value(keyPair):defaultInfoItem;
}

static SPEInfo speInfoInstance; // it's destructor will call deleteLists()
