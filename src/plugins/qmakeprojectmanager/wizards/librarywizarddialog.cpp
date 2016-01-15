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

#include "librarywizarddialog.h"
#include "filespage.h"
#include "libraryparameters.h"
#include "modulespage.h"

#include <utils/projectintropage.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <QDebug>

#include <QComboBox>
#include <QLabel>

enum { debugLibWizard = 0 };

namespace QmakeProjectManager {
namespace Internal {

struct PluginBaseClasses {
    const char *name;
    const char *module;
    // blank separated list or 0
    const char *dependentModules;
    const char *targetDirectory;
    const char *pluginInterface;
};

static const PluginBaseClasses pluginBaseClasses[] =
{
    { "QAccessiblePlugin", "QtGui", "QtCore", "accessible", "QAccessibleFactoryInterface" },
    { "QDecorationPlugin", "QtGui", "QtCore", 0, 0 }, // Qt 4 only.
    { "QGenericPlugin", "QtGui", "QtCore", "generic", "QGenericPluginFactoryInterface" },
    { "QIconEnginePluginV2", "QtGui", "QtCore", "imageformats", 0 }, // Qt 4 only.
    { "QIconEnginePlugin", "QtGui", "QtCore", "imageformats", "QIconEngineFactoryInterface" },
    { "QImageIOPlugin", "QtGui", "QtCore", "imageformats",  "QImageIOHandlerFactoryInterface" },
    { "QScriptExtensionPlugin", "QtScript", "QtCore", 0, "QScriptExtensionInterface" },
    { "QSqlDriverPlugin", "QtSql", "QtCore", "sqldrivers", "QSqlDriverFactoryInterface" },
    { "QStylePlugin", "QtGui", "QtCore", "styles", "QStyleFactoryInterface" },
    { "QTextCodecPlugin", "QtCore", 0, "codecs", 0 } // Qt 4 only.
};

enum { defaultPluginBaseClass = 2 };

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
static QStringList pluginDependencies(const PluginBaseClasses *plb)
{
    QStringList dependencies;
    const QChar blank = QLatin1Char(' ');
    // Find the module names and convert to ids
    QStringList pluginModules= plb->dependentModules ?
                               QString::fromLatin1(plb->dependentModules).split(blank) :
                               QStringList();
    pluginModules.push_back(QLatin1String(plb->module));
    foreach (const QString &module, pluginModules) {
        dependencies.append(ModulesPage::idOfModule(module));
    }
    return dependencies;
}

// A Project intro page with an additional type chooser.
class LibraryIntroPage : public Utils::ProjectIntroPage
{
public:
    explicit LibraryIntroPage(QWidget *parent = 0);

    QtProjectParameters::Type type() const;

private:
    QComboBox *m_typeCombo;
};

LibraryIntroPage::LibraryIntroPage(QWidget *parent) :
    Utils::ProjectIntroPage(parent),
    m_typeCombo(new QComboBox)
{
    m_typeCombo->setEditable(false);
    m_typeCombo->addItem(LibraryWizardDialog::tr("Shared Library"),
                         QVariant(QtProjectParameters::SharedLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Statically Linked Library"),
                         QVariant(QtProjectParameters::StaticLibrary));
    m_typeCombo->addItem(LibraryWizardDialog::tr("Qt Plugin"),
                         QVariant(QtProjectParameters::Qt4Plugin));
    insertControl(0, new QLabel(LibraryWizardDialog::tr("Type")), m_typeCombo);
}

QtProjectParameters::Type LibraryIntroPage::type() const
{
    return static_cast<QtProjectParameters::Type>(m_typeCombo->itemData(m_typeCombo->currentIndex()).toInt());
}

// ------------------- LibraryWizardDialog
LibraryWizardDialog::LibraryWizardDialog(const Core::BaseFileWizardFactory *factory,
                                         const QString &templateName,
                                         const QIcon &icon,
                                         QWidget *parent,
                                         const Core::WizardDialogParameters &parameters) :
    BaseQmakeProjectWizardDialog(factory, true, new LibraryIntroPage, -1, parent, parameters),
    m_filesPage(new FilesPage),
    m_pluginBaseClassesInitialized(false),
    m_filesPageId(-1), m_modulesPageId(-1), m_targetPageId(-1)
{
    setWindowIcon(icon);
    setWindowTitle(templateName);
    setSelectedModules(QLatin1String("core"));

    // Note that QWizard::currentIdChanged() is emitted at strange times.
    // Use the intro page instead, set up initially
    setIntroDescription(tr("This wizard generates a C++ Library project."));

    if (!parameters.extraValues().contains(QLatin1String(ProjectExplorer::Constants::PROJECT_KIT_IDS)))
        m_targetPageId = addTargetSetupPage();

    m_modulesPageId = addModulesPage();

    m_filesPage->setNamespacesEnabled(true);
    m_filesPage->setFormFileInputVisible(false);
    m_filesPage->setClassTypeComboVisible(false);

    m_filesPageId = addPage(m_filesPage);

    Utils::WizardProgressItem *introItem = wizardProgress()->item(startId());
    Utils::WizardProgressItem *targetItem = 0;
    if (m_targetPageId != -1)
        targetItem = wizardProgress()->item(m_targetPageId);
    Utils::WizardProgressItem *modulesItem = wizardProgress()->item(m_modulesPageId);
    Utils::WizardProgressItem *filesItem = wizardProgress()->item(m_filesPageId);
    filesItem->setTitle(tr("Details"));

    if (targetItem) {
        if (m_targetPageId != -1) {
            targetItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                     << modulesItem << filesItem);
            targetItem->setNextShownItem(0);
        } else {
            introItem->setNextItems(QList<Utils::WizardProgressItem *>()
                                    << modulesItem << filesItem);
            introItem->setNextShownItem(0);
        }
    }

    connect(this, &QWizard::currentIdChanged, this, &LibraryWizardDialog::slotCurrentIdChanged);

    addExtensionPages(extensionPages());
}

void LibraryWizardDialog::setSuffixes(const QString &header, const QString &source,  const QString &form)
{
    m_filesPage->setSuffixes(header, source, form);
}

void LibraryWizardDialog::setLowerCaseFiles(bool l)
{
    m_filesPage->setLowerCaseFiles(l);
}

QtProjectParameters::Type  LibraryWizardDialog::type() const
{
    return static_cast<const LibraryIntroPage*>(introPage())->type();
}

bool LibraryWizardDialog::isModulesPageSkipped() const
{
    // When leaving the intro or target page, the modules page is skipped
    // in the case of a plugin since it knows its dependencies by itself.
    return type() == QtProjectParameters::Qt4Plugin;
}

int LibraryWizardDialog::skipModulesPageIfNeeded() const
{
    if (isModulesPageSkipped())
        return m_filesPageId;
    return m_modulesPageId;
}

int LibraryWizardDialog::nextId() const
{
    if (m_targetPageId != -1) {
        if (currentId() == m_targetPageId)
            return skipModulesPageIfNeeded();
    } else if (currentId() == startId()) {
        return skipModulesPageIfNeeded();
    }

    return BaseQmakeProjectWizardDialog::nextId();
}

void LibraryWizardDialog::initializePage(int id)
{
    if (m_targetPageId != -1 && id == m_targetPageId) {
        Utils::WizardProgressItem *targetsItem = wizardProgress()->item(m_targetPageId);
        Utils::WizardProgressItem *modulesItem = wizardProgress()->item(m_modulesPageId);
        Utils::WizardProgressItem *filesItem = wizardProgress()->item(m_filesPageId);
        if (isModulesPageSkipped())
            targetsItem->setNextShownItem(filesItem);
        else
            targetsItem->setNextShownItem(modulesItem);

    }
    BaseQmakeProjectWizardDialog::initializePage(id);
}

void LibraryWizardDialog::cleanupPage(int id)
{
    if (m_targetPageId != -1 && id == m_targetPageId) {
        Utils::WizardProgressItem *targetsItem = wizardProgress()->item(m_targetPageId);
        targetsItem->setNextShownItem(0);
    }
    BaseQmakeProjectWizardDialog::cleanupPage(id);
}

QtProjectParameters LibraryWizardDialog::parameters() const
{
    QtProjectParameters rc;
    rc.type = type();
    rc.fileName = projectName();
    rc.path = path();
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
        rc.selectedModules = selectedModulesList();
        rc.deselectedModules = deselectedModulesList();
    }
    return rc;
}

void LibraryWizardDialog::slotCurrentIdChanged(int id)
{
    if (debugLibWizard)
        qDebug() << Q_FUNC_INFO << id;
    if (id == m_filesPageId)
        setupFilesPage();// Switching to files page: Set up base class accordingly (plugin)
}

void LibraryWizardDialog::setupFilesPage()
{
    switch (type()) {
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
    default:
        if (!m_filesPage->isComplete()) {
            // Urrm, figure out a good class name. Use project name this time
            QString className = projectName();
            if (!className.isEmpty())
                className[0] = className.at(0).toUpper();
            m_filesPage->setClassName(className);
            m_filesPage->setBaseClassInputVisible(false);
        }
        break;
    }
}

LibraryParameters LibraryWizardDialog::libraryParameters() const
{
    LibraryParameters rc;
    rc.className = m_filesPage->className();
    rc.baseClassName = type() == QtProjectParameters::Qt4Plugin ?
                       m_filesPage->baseClassName() : QString();
    rc.sourceFileName = m_filesPage->sourceFileName();
    rc.headerFileName = m_filesPage->headerFileName();
    return rc;
}

QString LibraryWizardDialog::pluginInterface(const QString &baseClass)
{
    if (const PluginBaseClasses *plb = findPluginBaseClass(baseClass))
        if (plb->pluginInterface)
            return QLatin1String("org.qt-project.Qt.") + QLatin1String(plb->pluginInterface);
    return QString();
}


} // namespace Internal
} // namespace QmakeProjectManager
