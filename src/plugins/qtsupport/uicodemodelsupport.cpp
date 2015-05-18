/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "uicodemodelsupport.h"

#include "qtkitinformation.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/idocument.h>
#include <cpptools/cppmodelmanager.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildmanager.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/session.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QFile>
#include <QFileInfo>
#include <QLoggingCategory>

using namespace ProjectExplorer;
using namespace CPlusPlus;

// Test for form editor (loosely coupled)
static inline bool isFormWindowDocument(const QObject *o)
{
    return o && !qstrcmp(o->metaObject()->className(), "Designer::Internal::FormWindowFile");
}

// Return contents of form editor (loosely coupled)
static inline QString formWindowEditorContents(const QObject *editor)
{
    const QVariant contentV = editor->property("contents");
    QTC_ASSERT(contentV.isValid(), return QString());
    return contentV.toString();
}

namespace QtSupport {

UiCodeModelSupport::UiCodeModelSupport(CppTools::CppModelManager *modelmanager,
                                       ProjectExplorer::Project *project,
                                       const QString &uiFile,
                                       const QString &uiHeaderFile)
    : CppTools::AbstractEditorSupport(modelmanager),
      m_project(project),
      m_uiFileName(uiFile),
      m_headerFileName(uiHeaderFile),
      m_state(BARE)
{
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    qCDebug(log) << "ctor UiCodeModelSupport for" << m_uiFileName << uiHeaderFile;
    connect(&m_process, SIGNAL(finished(int)),
            this, SLOT(finishProcess()));
    init();
}

UiCodeModelSupport::~UiCodeModelSupport()
{
    disconnect(&m_process, SIGNAL(finished(int)),
               this, SLOT(finishProcess()));
    m_process.kill();
    CppTools::CppModelManager::instance()->emitAbstractEditorSupportRemoved(m_headerFileName);
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    qCDebug(log) << "dtor ~UiCodeModelSupport for" << m_uiFileName;
}

void UiCodeModelSupport::init() const
{
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    if (m_state != BARE)
        return;
    QDateTime sourceTime = QFileInfo(m_uiFileName).lastModified();
    QFileInfo uiHeaderFileInfo(m_headerFileName);
    QDateTime uiHeaderTime = uiHeaderFileInfo.exists() ? uiHeaderFileInfo.lastModified() : QDateTime();
    if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
        QFile file(m_headerFileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            qCDebug(log) << "init: ui*h file is more recent then source file, using information from ui*h file" << m_headerFileName;
            QTextStream stream(&file);
            m_contents = stream.readAll().toUtf8();
            m_cacheTime = uiHeaderTime;
            m_state = FINISHED;
            notifyAboutUpdatedContents();
            return;
        }
    }

    qCDebug(log) << "ui*h file not found, or not recent enough, trying to create it on the fly";
    QFile file(m_uiFileName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        const QString contents = stream.readAll();
        if (runUic(contents)) {
            qCDebug(log) << "created on the fly";
            return;
        } else {
            // uic run was unsuccesfull
            qCDebug(log) << "uic run wasn't succesfull";
            m_cacheTime = QDateTime ();
            m_contents.clear();
            m_state = FINISHED;
            notifyAboutUpdatedContents();
            return;
        }
    } else {
        qCDebug(log) << "Could not open " << m_uiFileName << "needed for the cpp model";
        m_contents.clear();
        m_state = FINISHED;
        notifyAboutUpdatedContents();
    }
}

QByteArray UiCodeModelSupport::contents() const
{
    return m_contents;
}

QString UiCodeModelSupport::uiFileName() const
{
    return m_uiFileName;
}

QString UiCodeModelSupport::fileName() const
{
    return m_headerFileName;
}

void UiCodeModelSupport::setHeaderFileName(const QString &name)
{
    if (m_headerFileName == name && m_cacheTime.isValid())
        return;

    if (m_state == RUNNING) {
        m_state = ABORTING;
        m_process.kill();
        m_process.waitForFinished(3000);
    }

    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    qCDebug(log) << "UiCodeModelSupport::setFileName" << name;

    m_headerFileName = name;
    m_contents.clear();
    m_cacheTime = QDateTime();
    m_state = BARE;
    init();
}

bool UiCodeModelSupport::runUic(const QString &ui) const
{
    const QString uic = uicCommand();
    if (uic.isEmpty())
        return false;
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    m_process.setEnvironment(environment());

    qCDebug(log) << "  UiCodeModelSupport::runUic " << uic << " on " << ui.size() << " bytes";
    m_process.start(uic, QStringList(), QIODevice::ReadWrite);
    if (!m_process.waitForStarted())
        return false;
    m_process.write(ui.toUtf8());
    if (!m_process.waitForBytesWritten(3000))
        goto error;
    m_process.closeWriteChannel();
    m_state = RUNNING;
    return true;

error:
    qCDebug(log) << "failed" << m_process.readAllStandardError();
    m_process.kill();
    m_state = FINISHED;
    return false;
}

void UiCodeModelSupport::updateFromEditor(const QString &formEditorContents)
{
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    qCDebug(log) << "updating from editor" << m_uiFileName;
    if (m_state == RUNNING) {
        m_state = ABORTING;
        m_process.kill();
        m_process.waitForFinished(3000);
    }
    runUic(formEditorContents);
}

void UiCodeModelSupport::updateFromBuild()
{
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    qCDebug(log) << "UiCodeModelSupport::updateFromBuild() for " << m_uiFileName;

    // This is mostly a fall back for the cases when uic couldn't be run
    // it pays special attention to the case where a ui_*h was newly created
    QDateTime sourceTime = QFileInfo(m_uiFileName).lastModified();
    if (m_cacheTime.isValid() && m_cacheTime >= sourceTime) {
        qCDebug(log) << "Cache is still more recent then source";
        return;
    } else {
        QFileInfo fi(m_headerFileName);
        QDateTime uiHeaderTime = fi.exists() ? fi.lastModified() : QDateTime();
        if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
            if (m_cacheTime >= uiHeaderTime)
                return;
            qCDebug(log) << "found ui*h updating from it";

            QFile file(m_headerFileName);
            if (file.open(QFile::ReadOnly | QFile::Text)) {
                QTextStream stream(&file);
                m_contents = stream.readAll().toUtf8();
                m_cacheTime = uiHeaderTime;
                notifyAboutUpdatedContents();
                updateDocument();
                return;
            }
        }
        qCDebug(log) << "ui*h not found or not more recent then source not changing anything";
    }
}

QString UiCodeModelSupport::uicCommand() const
{
    QtSupport::BaseQtVersion *version;
    if (m_project->needsConfiguration()) {
        version = QtSupport::QtKitInformation::qtVersion(ProjectExplorer::KitManager::defaultKit());
    } else {
        ProjectExplorer::Target *target = m_project->activeTarget();
        version = QtSupport::QtKitInformation::qtVersion(target->kit());
    }
    return version ? version->uicCommand() : QString();
}

QStringList UiCodeModelSupport::environment() const
{
    if (m_project->needsConfiguration()) {
        return Utils::Environment::systemEnvironment().toStringList();
    } else {
        ProjectExplorer::Target *target = m_project->activeTarget();
        if (!target)
            return QStringList();
        ProjectExplorer::BuildConfiguration *bc = target->activeBuildConfiguration();
        return bc ? bc->environment().toStringList() : QStringList();
    }
}

bool UiCodeModelSupport::finishProcess()
{
    if (m_state != RUNNING)
        return false;
    QLoggingCategory log("qtc.qtsupport.uicodemodelsupport");
    if (!m_process.waitForFinished(3000)
            && m_process.exitStatus() != QProcess::NormalExit
            && m_process.exitCode() != 0) {

        qCDebug(log) << "finish process: failed" << m_process.readAllStandardError();
        m_process.kill();
        m_state = FINISHED;
        return false;
    }

    // As far as I can discover in the UIC sources, it writes out local 8-bit encoding. The
    // conversion below is to normalize both the encoding, and the line terminators.
    QString normalized = QString::fromLocal8Bit(m_process.readAllStandardOutput());
    m_contents = normalized.toUtf8();
    m_cacheTime = QDateTime::currentDateTime();
    qCDebug(log) << "finish process: ok" << m_contents.size() << "bytes.";
    m_state = FINISHED;
    notifyAboutUpdatedContents();
    updateDocument();
    return true;
}

UiCodeModelManager *UiCodeModelManager::m_instance = 0;

UiCodeModelManager::UiCodeModelManager() :
    m_lastEditor(0),
    m_dirty(false)
{
    m_instance = this;
    connect(BuildManager::instance(), SIGNAL(buildStateChanged(ProjectExplorer::Project*)),
            this, SLOT(buildStateHasChanged(ProjectExplorer::Project*)));
    connect(SessionManager::instance(),
            SIGNAL(projectRemoved(ProjectExplorer::Project*)),
            this, SLOT(projectWasRemoved(ProjectExplorer::Project*)));

    connect(Core::EditorManager::instance(), SIGNAL(editorAboutToClose(Core::IEditor*)),
            this, SLOT(editorIsAboutToClose(Core::IEditor*)));
    connect(Core::EditorManager::instance(), SIGNAL(currentEditorChanged(Core::IEditor*)),
            this, SLOT(editorWasChanged(Core::IEditor*)));
}

UiCodeModelManager::~UiCodeModelManager()
{
    m_instance = 0;
}

static UiCodeModelSupport *findUiFile(const QList<UiCodeModelSupport *> &range, const QString &uiFile)
{
    return Utils::findOrDefault(range, Utils::equal(&UiCodeModelSupport::uiFileName, uiFile));
}

void UiCodeModelManager::update(ProjectExplorer::Project *project, QHash<QString, QString> uiHeaders)
{
    CppTools::CppModelManager *mm = CppTools::CppModelManager::instance();

    // Find support to add/update:
    QList<UiCodeModelSupport *> oldSupport = m_instance->m_projectUiSupport.value(project);
    QList<UiCodeModelSupport *> newSupport;
    QHash<QString, QString>::const_iterator it;
    for (it = uiHeaders.constBegin(); it != uiHeaders.constEnd(); ++it) {
        if (UiCodeModelSupport *support = findUiFile(oldSupport, it.key())) {
            support->setHeaderFileName(it.value());
            oldSupport.removeOne(support);
            newSupport.append(support);
        } else {
            UiCodeModelSupport *cms = new UiCodeModelSupport(mm, project, it.key(), it.value());
            newSupport.append(cms);
            mm->addExtraEditorSupport(cms);
        }
    }

    // Remove old:
    foreach (UiCodeModelSupport *support, oldSupport) {
        mm->removeExtraEditorSupport(support);
        delete support;
    }

    // Update state:
    m_instance->m_projectUiSupport.insert(project, newSupport);
}

void UiCodeModelManager::updateContents(const QString &uiFileName, const QString &contents)
{
    QHash<Project *, QList<UiCodeModelSupport *> >::iterator i;
    for (i = m_instance->m_projectUiSupport.begin();
         i != m_instance->m_projectUiSupport.end(); ++i) {
        foreach (UiCodeModelSupport *support, i.value()) {
            if (support->uiFileName() == uiFileName)
                support->updateFromEditor(contents);
        }
    }
}

void UiCodeModelManager::buildStateHasChanged(Project *project)
{
    if (BuildManager::isBuilding(project))
        return;

    QList<UiCodeModelSupport *> projectSupport = m_projectUiSupport.value(project);
    foreach (UiCodeModelSupport *const i, projectSupport)
        i->updateFromBuild();
}

void UiCodeModelManager::projectWasRemoved(Project *project)
{
    CppTools::CppModelManager *mm = CppTools::CppModelManager::instance();

    QList<UiCodeModelSupport *> projectSupport = m_projectUiSupport.value(project);
    foreach (UiCodeModelSupport *const i, projectSupport) {
        mm->removeExtraEditorSupport(i);
        delete i;
    }

    m_projectUiSupport.remove(project);
}

void UiCodeModelManager::editorIsAboutToClose(Core::IEditor *editor)
{
    if (m_lastEditor == editor) {
        // Oh no our editor is going to be closed
        // get the content first
        if (isFormWindowDocument(m_lastEditor->document())) {
            disconnect(m_lastEditor->document(), SIGNAL(changed()), this, SLOT(uiDocumentContentsHasChanged()));
            if (m_dirty) {
                updateContents(m_lastEditor->document()->filePath().toString(),
                               formWindowEditorContents(m_lastEditor));
                m_dirty = false;
            }
        }
        m_lastEditor = 0;
    }
}

void UiCodeModelManager::editorWasChanged(Core::IEditor *editor)
{
    // Handle old editor
    if (m_lastEditor && isFormWindowDocument(m_lastEditor->document())) {
        disconnect(m_lastEditor->document(), SIGNAL(changed()), this, SLOT(uiDocumentContentsHasChanged()));

        if (m_dirty) {
            updateContents(m_lastEditor->document()->filePath().toString(),
                           formWindowEditorContents(m_lastEditor));
            m_dirty = false;
        }
    }

    m_lastEditor = editor;

    // Handle new editor
    if (m_lastEditor && isFormWindowDocument(m_lastEditor->document()))
        connect(m_lastEditor->document(), SIGNAL(changed()), this, SLOT(uiDocumentContentsHasChanged()));
}

void UiCodeModelManager::uiDocumentContentsHasChanged()
{
    QTC_ASSERT(isFormWindowDocument(sender()), return);
    m_dirty = true;
}

} // namespace QtSupport
