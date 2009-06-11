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

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/uniqueidmanager.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QtPlugin>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>

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

    if (qgetenv("KDE_SESSION_VERSION") == QByteArray("4")) {
        // KDE 4, possibly dangerous...
        // KDE 4.2.0 had a nasty bug, which resulted in the File/Open Dialog crashing
        // so check for that an fully load the plugins
        QProcess proc;
        proc.start("kde4-config", QStringList() << "--version");
        proc.waitForFinished();
        QString output = proc.readAll();
        if (output.contains("KDE: 4.2.0"))
            FormEditorW::ensureInitStage(FormEditorW::FullyInitialized);
    } else {
        FormEditorW::ensureInitStage(FormEditorW::RegisterPlugins);
    }

    QString locale = qApp->property("qtc_locale").toString();
    if (!locale.isEmpty()) {
        QTranslator *qtr = new QTranslator(this);
        const QString &creatorTrPath =
                Core::ICore::instance()->resourcePath() + QLatin1String("/translations");
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &trFile = QLatin1String("designer_") + locale;
        if (qtr->load(trFile, qtTrPath) || qtr->load(trFile, creatorTrPath))
            qApp->installTranslator(qtr);
    }

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
    wizardParameters.setDescription(tr("Creates a Qt Designer form file (.ui)."));
    m_formWizard = new FormWizard(wizardParameters, this);
    addObject(m_formWizard);

#ifdef CPP_ENABLED
    wizardParameters.setKind(Core::IWizard::ClassWizard);
    wizardParameters.setName(tr("Qt Designer Form Class"));
    wizardParameters.setDescription(tr("Creates a Qt Designer form file (.ui) with a matching class."));
    m_formClassWizard = new FormClassWizard(wizardParameters, this);
    addObject(m_formClassWizard);
#endif
    return true;
}

Q_EXPORT_PLUGIN(FormEditorPlugin)
