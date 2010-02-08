/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtuicodemodelsupport.h"
#include "qt4buildconfiguration.h"

#include "qt4project.h"
#include "qt4target.h"

#include <designer/formwindoweditor.h>

using namespace Qt4ProjectManager;
using namespace Internal;

Qt4UiCodeModelSupport::Qt4UiCodeModelSupport(CppTools::CppModelManagerInterface *modelmanager,
                                             Qt4Project *project,
                                             const QString &source,
                                             const QString &uiHeaderFile)
    : CppTools::AbstractEditorSupport(modelmanager),
      m_project(project),
      m_sourceName(source),
      m_fileName(uiHeaderFile),
      m_updateIncludingFiles(false)
{
//    qDebug()<<"ctor Qt4UiCodeModelSupport for"<<m_sourceName;
    init();
}

Qt4UiCodeModelSupport::~Qt4UiCodeModelSupport()
{
//    qDebug()<<"dtor ~Qt4UiCodeModelSupport for"<<m_sourceName;
}

void Qt4UiCodeModelSupport::init()
{
    QDateTime sourceTime = QFileInfo(m_sourceName).lastModified();
    QFileInfo uiHeaderFileInfo(m_fileName);
    QDateTime uiHeaderTime = uiHeaderFileInfo.exists() ? uiHeaderFileInfo.lastModified() : QDateTime();
    if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
        QFile file(m_fileName);
        if (file.open(QFile::ReadOnly)) {
//            qDebug()<<"ui*h file is more recent then source file, using information from ui*h file"<<m_fileName;
            QTextStream stream(&file);
            m_contents = stream.readAll().toUtf8();
            m_cacheTime = uiHeaderTime;
            return;
        }
    }

//    qDebug()<<"ui*h file not found, or not recent enough, trying to create it on the fly";
    QFile file(m_sourceName);
    if (file.open(QFile::ReadOnly)) {
        QTextStream stream(&file);
        const QString contents = stream.readAll();
        if (runUic(contents)) {
//            qDebug()<<"created on the fly";
            return;
        } else {
            // uic run was unsuccesfull
//            qDebug()<<"uic run wasn't succesfull";
            m_cacheTime = QDateTime();
            m_contents = QByteArray();
            // and if the header file wasn't there, next time we need to update
            // all of the files that include this header
            if (!uiHeaderFileInfo.exists())
                m_updateIncludingFiles = true;
            return;
        }
    } else {
//        qDebug()<<"Could open "<<m_sourceName<<"needed for the cpp model";
        m_contents = QByteArray();
    }
}

QByteArray Qt4UiCodeModelSupport::contents() const
{
    return m_contents;
}

QString Qt4UiCodeModelSupport::fileName() const
{
    return m_fileName;
}

void Qt4UiCodeModelSupport::setFileName(const QString &name)
{
    if (m_fileName == name && m_cacheTime.isValid())
        return;
    m_fileName = name;
    m_contents.clear();
    m_cacheTime = QDateTime();
    init();
}

bool Qt4UiCodeModelSupport::runUic(const QString &ui) const
{
    Qt4BuildConfiguration *qt4bc = m_project->activeTarget()->activeBuildConfiguration();
    QProcess uic;
    uic.setEnvironment(qt4bc->environment().toStringList());
    uic.start(qt4bc->qtVersion()->uicCommand(), QStringList(), QIODevice::ReadWrite);
    uic.waitForStarted();
    uic.write(ui.toUtf8());
    uic.closeWriteChannel();
    if (uic.waitForFinished()) {
        m_contents = uic.readAllStandardOutput();
        m_cacheTime = QDateTime::currentDateTime();
        return true;
    } else {
//        qDebug()<<"running uic failed"<<" using uic: "<<m_project->qtVersion(m_project->activeBuildConfiguration())->uicCommand();
//        qDebug()<<uic.readAllStandardError();
//        qDebug()<<uic.readAllStandardOutput();
//        qDebug()<<uic.errorString();
//        qDebug()<<uic.error();
        uic.kill();
    }
    return false;
}

void Qt4UiCodeModelSupport::updateFromEditor(Designer::FormWindowEditor *fw)
{
//    qDebug()<<"Qt4UiCodeModelSupport::updateFromEditor"<<fw;
    if (runUic(fw->contents())) {
//        qDebug()<<"runUic: success, updated on the fly";
        updateDocument();
    } else {
//        qDebug()<<"runUic: failed, not updated";
    }
}

void Qt4UiCodeModelSupport::updateFromBuild()
{
//    qDebug()<<"Qt4UiCodeModelSupport::updateFromBuild() for file"<<m_sourceName;
    // This is mostly a fall back for the cases when uic couldn't be run
    // it pays special attention to the case where a ui_*h was newly created
    QDateTime sourceTime = QFileInfo(m_sourceName).lastModified();
    if (m_cacheTime.isValid() && m_cacheTime >= sourceTime) {
//        qDebug()<<"Cache is still more recent then source";
        return;
    } else {
        QFileInfo fi(m_fileName);
        QDateTime uiHeaderTime = fi.exists() ? fi.lastModified() : QDateTime();
        if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
            if (m_cacheTime >= uiHeaderTime)
                return;
//            qDebug()<<"found ui*h updating from it";

            QFile file(m_fileName);
            if (file.open(QFile::ReadOnly)) {
                QTextStream stream(&file);
                m_contents = stream.readAll().toUtf8();
                m_cacheTime = uiHeaderTime;
                updateDocument();
                return;
            }
        }

//        qDebug()<<"ui*h not found or not more recent then source not changing anything";
    }
}

