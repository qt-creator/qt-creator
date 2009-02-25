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

#include "formeditorplugin.h"
#include "formeditorfactory.h"
#include "formeditorw.h"
#include "formwizard.h"

#ifdef CPP_ENABLED
#  include "formclasswizard.h"
#  include <cppeditor/cppeditorconstants.h>
#endif

#include "designerconstants.h"
#if QT_VERSION < 0x040500
#    include "settings.h"
#endif

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>

#include <QtCore/QtPlugin>
#include <QtCore/QDebug>

#ifdef CPP_ENABLED
#    include <QtGui/QAction>
#    include <QtGui/QWizard>
#    include <QtGui/QMainWindow>
#endif

using namespace Designer::Internal;
using namespace Designer::Constants;

FormEditorPlugin::FormEditorPlugin() :
    m_factory(0),
    m_formWizard(0),
    m_formClassWizard(0)
{
}

FormEditorPlugin::~FormEditorPlugin()
{
    if (m_factory)
        removeObject(m_factory);
    if (m_formWizard)
        removeObject(m_formWizard);
    if (m_formClassWizard)
        removeObject(m_formClassWizard);
    delete m_factory;
    delete m_formWizard;
    delete m_formClassWizard;
    FormEditorW::deleteInstance();
}

////////////////////////////////////////////////////
//
// INHERITED FROM ExtensionSystem::Plugin
//
////////////////////////////////////////////////////
bool FormEditorPlugin::initialize(const QStringList &arguments, QString *error)
{
    Q_UNUSED(arguments);
    Q_UNUSED(error);

    Core::ICore *core = Core::ICore::instance();
    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/formeditor/Designer.mimetypes.xml"), error))
        return false;

    if (!initializeTemplates(error))
        return false;

    const int uid = core->uniqueIDManager()->uniqueIdentifier(QLatin1String(C_FORMEDITOR));
    const QList<int> context = QList<int>() << uid;

    m_factory = new FormEditorFactory;
    addObject(m_factory);

    // Make sure settings pages and action shortcuts are registered
    // TODO we don't want to do a full initialization here,
    // we actually want to call ensureInitStage(FormEditorW::RegisterPlugins)
    // But due to a bug in kde 4.2.0 this crashes then when opening the file dialog
    // This should be removed after 4.2.1 is out
    FormEditorW::ensureInitStage(FormEditorW::FullyInitialized);

    error->clear();
    return true;
}

void FormEditorPlugin::extensionsInitialized()
{
}

////////////////////////////////////////////////////
//
// PRIVATE methods
//
////////////////////////////////////////////////////

bool FormEditorPlugin::initializeTemplates(QString *error)
{
    Q_UNUSED(error);
    FormWizard::BaseFileWizardParameters wizardParameters(Core::IWizard::FileWizard);
    wizardParameters.setCategory(QLatin1String("Qt"));
    wizardParameters.setTrCategory(tr("Qt"));
    const QString formFileType = QLatin1String(Constants::FORM_FILE_TYPE);
    wizardParameters.setName(tr("Qt Designer Form"));
    wizardParameters.setDescription(tr("This creates a new Qt Designer form file."));
    m_formWizard = new FormWizard(wizardParameters, this);
    addObject(m_formWizard);

#ifdef CPP_ENABLED
    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setName(tr("Qt Designer Form Class"));
    wizardParameters.setDescription(tr("This creates a new Qt Designer form class."));
    m_formClassWizard = new FormClassWizard(wizardParameters, this);
    addObject(m_formClassWizard);
#endif
    return true;
}

Q_EXPORT_PLUGIN(FormEditorPlugin)
