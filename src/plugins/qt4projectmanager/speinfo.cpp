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
QList<SPEInfoItem*> SPEInfo::m_configurationList;
QList<SPEInfoItem*> SPEInfo::m_platformList;
QList<SPEInfoItem*> SPEInfo::m_variableList;
QList<SPEInfoItem*> SPEInfo::m_qtmoduleList;
QList<SPEInfoItem*> SPEInfo::m_templateList;
QList<SPEInfoItem*> SPEInfo::m_operatorList;

QHash<QPair<SPEInfoItem::InfoKind, QString> ,SPEInfoItem*> SPEInfo::m_itemHash;

const QString SPEInfoItem::keyType("valuetype");
const QString SPEInfoItem::valueFile("file");
const QString SPEInfoItem::valuePath("path");
const QString SPEInfoItem::keyIncludedByDefault("includedbydefault");
const QString SPEInfoItem::keyImageFileName("imagefilename");

// Configurations (Debug, Release, ...)
class InfoItemConfigurationCross : public SPEInfoItem
{
public:
    InfoItemConfigurationCross(): SPEInfoItem("", Configuration) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Debug and Release"); }
};

class InfoItemConfigurationDebug : public SPEInfoItem
{
public:
    InfoItemConfigurationDebug(): SPEInfoItem("debug", Configuration) {}
    QString name(void) const {return QCoreApplication::translate("SimpleProEditor", "Debug specific");}
};

class InfoItemConfigurationRelease : public SPEInfoItem
{
public:
    InfoItemConfigurationRelease(): SPEInfoItem("release", Configuration) {}
    QString name(void) const {return QCoreApplication::translate("SimpleProEditor", "Release specific");}
};


// Platforms (Windows, Mac, ...)
class InfoItemPlatformCross : public SPEInfoItem
{
public:
    InfoItemPlatformCross(): SPEInfoItem("", Platform) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "All platforms"); }
};

class InfoItemPlatformWindows : public SPEInfoItem
{
public:
    InfoItemPlatformWindows(): SPEInfoItem("win32", Platform) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "MS Windows specific"); }
};

class InfoItemPlatformUnix : public SPEInfoItem
{
public:
    InfoItemPlatformUnix(): SPEInfoItem("unix", Platform) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Linux/Unix specific"); }
};

class InfoItemPlatformOSX : public SPEInfoItem
{
public:
    InfoItemPlatformOSX(): SPEInfoItem("macx", Platform) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Mac OSX specific"); }
};


// Variables (Target options, Libraries, Defines, ...)
class InfoItemVariableTargetOptions : public SPEInfoItem
{
public:
    InfoItemVariableTargetOptions(): SPEInfoItem("TEMPLATE", Variable)
    {
        m_data.insert(keyImageFileName, ":/variableimages/images/target.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Target Options");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Type and name of the target.");
    }
};

class InfoItemVariableDefines : public SPEInfoItem
{
public:
    InfoItemVariableDefines(): SPEInfoItem("DEFINES", Variable)
    {
        m_data.insert(keyImageFileName, ":/variableimages/images/defines.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Preprocessor Definitions");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Setting of the preprocessor definitions.");
    }
};

class InfoItemVariableIncludePath : public SPEInfoItem
{
public:
    InfoItemVariableIncludePath(): SPEInfoItem("INCLUDEPATH", Variable)
    {
        m_data.insert(keyType, valuePath);
        m_data.insert(keyImageFileName, ":/variableimages/images/includes.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Include path"); }
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Setting of the pathes where the header files are located.");
    }
};

class InfoItemVariableLibs : public SPEInfoItem
{
public:
    InfoItemVariableLibs(): SPEInfoItem("LIBS", Variable)
    {
        m_data.insert(keyImageFileName, ":/variableimages/images/libs.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Libraries");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Defining the libraries to link the target against and the pathes where these are located.");
    }
};

class InfoItemVariableSources : public SPEInfoItem
{
public:
    InfoItemVariableSources(): SPEInfoItem("SOURCES", Variable)
    {
        m_data.insert(keyType, valueFile);
        m_data.insert(keyImageFileName, ":/variableimages/images/sources.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Source Files");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "");
    }
};

class InfoItemVariableHeaders : public SPEInfoItem
{
public:
    InfoItemVariableHeaders(): SPEInfoItem("HEADERS", Variable)
    {
        m_data.insert(keyType, valueFile);
        m_data.insert(keyImageFileName, ":/variableimages/images/headers.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Header Files");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "");
    }
};

class InfoItemVariableForms : public SPEInfoItem
{
public:
    InfoItemVariableForms(): SPEInfoItem("FORMS", Variable)
    {
        m_data.insert(keyType, valueFile);
        m_data.insert(keyImageFileName, ":/variableimages/images/forms.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Forms");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "");
    }
};

class InfoItemVariableQtModules : public SPEInfoItem
{
public:
    InfoItemVariableQtModules(): SPEInfoItem("QT", Variable)
    {
        m_data.insert(keyImageFileName, ":/variableimages/images/qtmodules.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Qt Modules");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Setting up which of the Qt modules will be used in the target application.");
    }
};

class InfoItemVariableResources : public SPEInfoItem
{
public:
    InfoItemVariableResources(): SPEInfoItem("RESOURCES", Variable)
    {
        m_data.insert(keyType, valueFile);
        m_data.insert(keyImageFileName, ":/variableimages/images/resources.png");
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Resource files");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "");
    }
};

class InfoItemVariableTarget : public SPEInfoItem
{
public:
    InfoItemVariableTarget(): SPEInfoItem("TARGET", Variable) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Target name");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "The name of the resulting target.");
    }
};

class InfoItemVariableConfig : public SPEInfoItem
{
public:
    InfoItemVariableConfig(): SPEInfoItem("CONFIG", Variable) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Configuration");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Configuration.");
    }
};

class InfoItemVariableDestdir : public SPEInfoItem
{
public:
    InfoItemVariableDestdir(): SPEInfoItem("DESTDIR", Variable) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Destination directory");}
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Where the resulting target will be created.");
    }
};


// Qt modules
class InfoItemModulesCore : public SPEInfoItem
{
public:
    InfoItemModulesCore(): SPEInfoItem("core", QtModule)
    {
        m_data.insert(keyIncludedByDefault, true);
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtCore Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtGui Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtNetwork Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtOpenGL Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtSql Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtScript Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtSvg Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtWebKit Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtXml Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtXmlPatterns Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Phonon Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Qt3Support Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtTest Module"); }
    QString description(void) const
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

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "QtDBus module"); }
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Classes for Inter-Process Communication using the D-Bus");
    }
};


// Target templates
class InfoItemTemplatesApp : public SPEInfoItem
{
public:
    InfoItemTemplatesApp(): SPEInfoItem("app", Template)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Application"); }
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Create a standalone application");
    }
};

class InfoItemTemplatesDynamicLib : public SPEInfoItem
{
public:
    InfoItemTemplatesDynamicLib(): SPEInfoItem("lib", Template)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Dynamic Library"); }
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Create a dynamic library for usage in other applications");
    }
};

class InfoItemTemplatesStaticLib : public SPEInfoItem
{
public:
    InfoItemTemplatesStaticLib(): SPEInfoItem("staticlib", Template)
    {
        m_data.insert(keyIncludedByDefault, false);
    }

    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Static Library"); }
    QString description(void) const
    {
        return QCoreApplication::translate("SimpleProEditor",
            "Create a static library for usage in other applications");
    }
};

// Variable operators
class InfoItemOperatorsAdd : public SPEInfoItem
{
public:
    InfoItemOperatorsAdd(): SPEInfoItem("+=", Operator) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Add Operator"); }
};

class InfoItemOperatorsRemove : public SPEInfoItem
{
public:
    InfoItemOperatorsRemove(): SPEInfoItem("-=", Operator) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Remove Operator"); }
};

class InfoItemOperatorsReplace : public SPEInfoItem
{
public:
    InfoItemOperatorsReplace(): SPEInfoItem("~=", Operator) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Replace Operator"); }
};

class InfoItemOperatorsSet : public SPEInfoItem
{
public:
    InfoItemOperatorsSet(): SPEInfoItem("=", Operator) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Set Operator"); }
};

class InfoItemOperatorsUniqueAdd : public SPEInfoItem
{
public:
    InfoItemOperatorsUniqueAdd(): SPEInfoItem("*=", Operator) {}
    QString name(void) const { return QCoreApplication::translate("SimpleProEditor", "Unique Add Operator"); }
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

const SPEInfoItem *SPEInfoItem::parentItem(void) const
{
    return m_parentItem;
}

void SPEInfoItem::setParentItem(const SPEInfoItem *parentItem)
{
    m_parentItem = parentItem;
}

bool SPEInfoItem::isAncestorOf(const SPEInfoItem *successor) const
{
    const SPEInfoItem *ancestorCursor = successor;

    while ((ancestorCursor = ancestorCursor->parentItem()) != NULL)
        if (ancestorCursor == this)
            return true;

    return false;
}

QString SPEInfoItem::id() const
{
    return m_id;
}

SPEInfoItem::InfoKind SPEInfoItem::infoKind(void) const
{
    return m_infoKind;
}

SPEInfo::~SPEInfo()
{
    deleteLists();
}

const QList<SPEInfoItem*> *SPEInfo::list(SPEInfoItem::InfoKind kind)
{
    if (!m_listsInitialized)
        initializeLists();
    return
            kind == SPEInfoItem::Configuration?&m_configurationList
            :kind == SPEInfoItem::Platform?&m_platformList
            :kind == SPEInfoItem::Variable?&m_variableList
            :kind == SPEInfoItem::QtModule?&m_qtmoduleList
            :kind == SPEInfoItem::Template?&m_templateList
            :/*kind == SPEInfoItem::Operator?*/&m_operatorList
            ;
}

const SPEInfoItem *SPEInfo::defaultInfoOfKind(SPEInfoItem::InfoKind kind)
{
    return list(kind)->at(0);
}

void SPEInfo::addListToHash(const QList<SPEInfoItem*> &list)
{
    foreach (SPEInfoItem *item, list)
        m_itemHash.insert(qMakePair(item->infoKind(), item->id()), item);
}

void SPEInfo::initializeLists(void)
{
    InfoItemConfigurationCross *infoItemConfigurationCross = new InfoItemConfigurationCross;
    InfoItemConfigurationDebug *infoItemConfigurationDebug = new InfoItemConfigurationDebug;
    infoItemConfigurationDebug->setParentItem(infoItemConfigurationCross);
    InfoItemConfigurationRelease *infoItemConfigurationRelease = new InfoItemConfigurationRelease;
    infoItemConfigurationRelease->setParentItem(infoItemConfigurationCross);
    m_configurationList
        << infoItemConfigurationCross
        << infoItemConfigurationDebug
        << infoItemConfigurationRelease;
    addListToHash(m_configurationList);

    InfoItemPlatformCross *infoItemPlatformCross = new InfoItemPlatformCross;
    InfoItemPlatformWindows *infoItemPlatformWindows = new InfoItemPlatformWindows;
    infoItemPlatformWindows->setParentItem(infoItemPlatformCross);
    InfoItemPlatformUnix *infoItemPlatformUnix = new InfoItemPlatformUnix;
    infoItemPlatformUnix->setParentItem(infoItemPlatformCross);
    InfoItemPlatformOSX *infoItemPlatformOSX = new InfoItemPlatformOSX;
    infoItemPlatformOSX->setParentItem(infoItemPlatformUnix);
    m_platformList
        << infoItemPlatformCross
        << infoItemPlatformWindows
        << infoItemPlatformUnix
        << infoItemPlatformOSX;
    addListToHash(m_platformList);

    m_variableList
        << new InfoItemVariableTargetOptions
        << new InfoItemVariableDefines
        << new InfoItemVariableLibs
        << new InfoItemVariableIncludePath
        << new InfoItemVariableSources
        << new InfoItemVariableHeaders
        << new InfoItemVariableForms
        << new InfoItemVariableQtModules
        << new InfoItemVariableResources
        << new InfoItemVariableTarget
        << new InfoItemVariableConfig
        << new InfoItemVariableDestdir;
    addListToHash(m_variableList);

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

    m_templateList
        << new InfoItemTemplatesApp
        << new InfoItemTemplatesDynamicLib
        << new InfoItemTemplatesStaticLib;
    addListToHash(m_templateList);

    m_operatorList
        << new InfoItemOperatorsAdd
        << new InfoItemOperatorsRemove
        << new InfoItemOperatorsReplace
        << new InfoItemOperatorsSet
        << new InfoItemOperatorsUniqueAdd;
    addListToHash(m_operatorList);

    m_listsInitialized = true;
}

void SPEInfo::deleteLists(void)
{
    m_itemHash.clear();

    static QList<SPEInfoItem*> *lists[] = {
        &m_configurationList,
        &m_platformList,
        &m_variableList,
        &m_qtmoduleList,
        &m_templateList,
        &m_operatorList
    };

    for (size_t i = 0; i < sizeof(lists)/sizeof(lists[0]); i++) {
        qDeleteAll(*lists[i]);
        lists[i]->clear();
    }

    m_listsInitialized = false;
}

const SPEInfoItem *SPEInfo::infoOfKindForId(SPEInfoItem::InfoKind kind,
                                            const QString &id, const SPEInfoItem *defaultInfoItem)
{
    QPair<SPEInfoItem::InfoKind, QString > keyPair = qMakePair(kind, id);
    return m_itemHash.contains(keyPair)?m_itemHash.value(keyPair):defaultInfoItem;
}

const SPEInfoItem *SPEInfo::platformInfoForId(const QString &id)
{
    return infoOfKindForId(SPEInfoItem::Platform, id, SPEInfo::defaultInfoOfKind(SPEInfoItem::Platform));
}

const SPEInfoItem *SPEInfo::configurationInfoForId(const QString &id)
{
    return infoOfKindForId(SPEInfoItem::Configuration, id, SPEInfo::defaultInfoOfKind(SPEInfoItem::Configuration));
}

static SPEInfo speInfoInstance; // it's destructor will call deleteLists()
