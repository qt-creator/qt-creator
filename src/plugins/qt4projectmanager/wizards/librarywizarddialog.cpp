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

#include "librarywizarddialog.h"

#include "consoleappwizard.h"
#include "filespage.h"
#include "libraryparameters.h"
#include "modulespage.h"

#include <utils/projectintropage.h>
#include <utils/qtcassert.h>

#include <QtGui/QComboBox>
#include <QtGui/QLabel>

#include <QtCore/QDebug>

enum { debugLibWizard = 0 };
enum { IntroPageId, ModulesPageId, FilePageId };

namespace Qt4ProjectManager {
namespace Internal {

struct PluginBaseClasses {
    const char *name;
    const char *module;
    // blank separated list or 0
    const char *dependentModules;
    const char *targetDirectory;
};

static const PluginBaseClasses pluginBaseClasses[] =
{
    { "QAccessiblePlugin", "QtGui", "QtCore", "accessible" },
    { "QDecorationPlugin", "QtGui", "QtCore", 0},
    { "QFontEnginePlugin", "QtGui", "QtCore", 0},
    { "QIconEnginePluginV2", "QtGui", "QtCore", "imageformats" },
    { "QImageIOPlugin", "QtGui", "QtCore", "imageformats" },
    { "QScriptExtensionPlugin", "QtScript", "QtCore", 0 },
    { "QSqlDriverPlugin", "QtSql", "QtCore", "sqldrivers" },
    { "QStylePlugin", "QtGui", "QtCore", "styles" },
    { "QTextCodecPlugin", "QtCore", 0, "codecs" }
};

enum { defaultPluginBaseClass = 7 };

static const PluginBaseClasses *findPluginBaseClass(const QString &name)
{
    const int pluginBaseClassCount = sizeof(pluginBaseClasses)/sizeof(PluginBaseClasses);
    for (int i = 0; i < pluginBaseClassCount; i++)
        if (name == QLatin1String(pluginBaseClasses[i].name))
            return pluginBaseClasses + i;
    return 0;
}

// return dependencies of a plugin as a line ready for the 'QT=' line in a pro
// file
static QString pluginDependencies(const PluginBaseClasses *plb)
{
    QString dependencies;
    const QChar blank = QLatin1Char(' ');
    // Find the module names and convert to ids
    QStringList pluginModules= plb->dependentModules ?
                               QString(QLatin1String(plb->dependentModules)).split(blank) :
                               QStringList();
    pluginModules.push_back(QLatin1String(plb->module));
    foreach (const QString &module, pluginModules) {
        if (!dependencies.isEmpty())
            dependencies += blank;
        dependencies += ModulesPage::idOfModule(module);
    }
    return dependencies;
}

// A Project intro page with an additional type chooser.
class LibraryIntroPage : public Core::Utils::ProjectIntroPage
{
    Q_DISABLE_COPY(LibraryIntroPage)
public:
    explicit LibraryIntroPage(QWidget *parent = 0);

    QtProjectParameters::Type type() const;

    virtual int nextId() const;

private:
    QComboBox *m_typeCombo;
};

LibraryIntroPage::LibraryIntroPage(QWidget *parent) :
    Core::Utils::ProjectIntroPage(parent),
    m_typeCombo(new QComboBox)
{
    m_typeCombo->setEditable(false);
    m_typeCombo->addItem(LibraryWizardDialog::tr("Shared library"),
                         QVariant(QtProjectParameters::SharedLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Statically linked library"),
                         QVariant(QtProjectParameters::StaticLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Qt 4 plugin"),
                         QVariant(QtProjectParameters::Qt4Plugin));
    insertControl(0, new QLabel(LibraryWizardDialog::tr("Type")), m_typeCombo);
}

QtProjectParameters::Type LibraryIntroPage::type() const
{
    return static_cast<QtProjectParameters::Type>(m_typeCombo->itemData(m_typeCombo->currentIndex()).toInt());
}

int LibraryIntroPage::nextId() const
{
    // The modules page is skipped in the case of a plugin since it knows its
    // dependencies by itself
    const int rc = type() == QtProjectParameters::Qt4Plugin ? FilePageId : ModulesPageId;
    if (debugLibWizard)
        qDebug() << Q_FUNC_INFO <<  "returns" << rc;
    return rc;
}

// ------------------- LibraryWizardDialog
LibraryWizardDialog::LibraryWizardDialog(const QString &templateName,
                                         const QIcon &icon,
                                         const QList<QWizardPage*> &extensionPages,
                                         QWidget *parent) :
    QWizard(parent),
    m_introPage(new LibraryIntroPage),
    m_modulesPage(new ModulesPage),
    m_filesPage(new FilesPage),
    m_pluginBaseClassesInitialized(false)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    Core::BaseFileWizard::setupWizard(this);

    // Note that QWizard::currentIdChanged() is emitted at strange times.
    // Use the intro page instead, set up initially
    m_introPage->setDescription(tr("This wizard generates a C++ library project."));

    setPage(IntroPageId, m_introPage);

    m_modulesPage->setModuleSelected(QLatin1String("core"));
    setPage(ModulesPageId, m_modulesPage);

    m_filesPage->setNamespacesEnabled(true);
    m_filesPage->setFormFileInputVisible(false);
    setPage(FilePageId, m_filesPage);

    connect(this, SIGNAL(currentIdChanged(int)), this, SLOT(slotCurrentIdChanged(int)));

    foreach (QWizardPage *p, extensionPages)
        addPage(p);
}

void LibraryWizardDialog::setSuffixes(const QString &header, const QString &source,  const QString &form)
{
    m_filesPage->setSuffixes(header, source, form);
}

void LibraryWizardDialog::setPath(const QString &path)
{
    m_introPage->setPath(path);
}

void  LibraryWizardDialog::setName(const QString &name)
{
    m_introPage->setName(name);
}

QtProjectParameters LibraryWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = m_introPage->type();
    rc.name = m_introPage->name();
    rc.path = m_introPage->path();
    if (rc.type == QtProjectParameters::Qt4Plugin) {
        // Plugin: Dependencies & Target directory
        if (const PluginBaseClasses *plb = findPluginBaseClass(m_filesPage->baseClassName())) {
            rc.selectedModules = pluginDependencies(plb);
            if (plb->targetDirectory) {
                rc.targetDirectory = QLatin1String("$$[QT_INSTALL_PLUGINS]/");
                rc.targetDirectory += QLatin1String(plb->targetDirectory);
            }
        }
    } else {
        // Modules from modules page
        rc.selectedModules =  m_modulesPage->selectedModules();
        rc.deselectedModules = m_modulesPage-> deselectedModules();
    }
    return rc;
}

void LibraryWizardDialog::slotCurrentIdChanged(int id)
{
    if (debugLibWizard)
        qDebug() << Q_FUNC_INFO << id;
    // Switching to files page: Set up base class accordingly (plugin)
    if (id != FilePageId)
        return;
    switch (m_introPage->type()) {
    case QtProjectParameters::Qt4Plugin:
        if (!m_pluginBaseClassesInitialized) {
             if (debugLibWizard)
                qDebug("initializing for plugins");
            QStringList baseClasses;
            const int pluginBaseClassCount = sizeof(pluginBaseClasses)/sizeof(PluginBaseClasses);
            Q_ASSERT(defaultPluginBaseClass < pluginBaseClassCount);
            for (int i = 0; i < pluginBaseClassCount; i++)
                baseClasses.push_back(QLatin1String(pluginBaseClasses[i].name));
            m_filesPage->setBaseClassChoices(baseClasses);
            m_filesPage->setBaseClassName(baseClasses.at(defaultPluginBaseClass));
            m_pluginBaseClassesInitialized = true;
        }
        m_filesPage->setBaseClassInputVisible(true);
        break;
    default: {
        // Urrm, figure out a good class name. Use project name this time
        QString name = m_introPage->name();
        name[0] = name.at(0).toUpper();
        m_filesPage->setClassName(name);
        m_filesPage->setBaseClassInputVisible(false);
    }
        break;
    }
}

LibraryParameters LibraryWizardDialog::libraryParameters() const
{
    LibraryParameters rc;
    rc.className = m_filesPage->className();
    rc.baseClassName = m_filesPage->baseClassName();
    rc.sourceFileName = m_filesPage->sourceFileName();
    rc.headerFileName = m_filesPage->headerFileName();
    if (!rc.baseClassName.isEmpty())
        if (const PluginBaseClasses *plb = findPluginBaseClass(rc.baseClassName)) {
            rc.baseClassModule = QLatin1String(plb->module);
        }
    return rc;
}

} // namespace Internal
} // namespace Qt4ProjectManager
