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

#include "uicodecompletionsupport.h"

#include <QProcess>
#include <QFile>
#include <QFileInfo>

enum { debug = 0 };

using namespace CppTools;
using namespace CPlusPlus;

UiCodeModelSupport::UiCodeModelSupport(CppModelManagerInterface *modelmanager,
                                       const QString &source,
                                       const QString &uiHeaderFile)
    : AbstractEditorSupport(modelmanager),
      m_sourceName(source),
      m_fileName(uiHeaderFile),
      m_state(BARE)
{
    if (debug)
        qDebug()<<"ctor UiCodeModelSupport for"<<m_sourceName<<uiHeaderFile;
    connect(&m_process, SIGNAL(finished(int)),
            this, SLOT(finishProcess()));
}

UiCodeModelSupport::~UiCodeModelSupport()
{
    if (debug)
        qDebug()<<"dtor ~UiCodeModelSupport for"<<m_sourceName;
}

void UiCodeModelSupport::init() const
{
    if (m_state != BARE)
        return;
    QDateTime sourceTime = QFileInfo(m_sourceName).lastModified();
    QFileInfo uiHeaderFileInfo(m_fileName);
    QDateTime uiHeaderTime = uiHeaderFileInfo.exists() ? uiHeaderFileInfo.lastModified() : QDateTime();
    if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
        QFile file(m_fileName);
        if (file.open(QFile::ReadOnly | QFile::Text)) {
            if (debug)
                qDebug()<<"ui*h file is more recent then source file, using information from ui*h file"<<m_fileName;
            QTextStream stream(&file);
            m_contents = stream.readAll().toUtf8();
            m_cacheTime = uiHeaderTime;
            m_state = FINISHED;
            return;
        }
    }

    if (debug)
        qDebug()<<"ui*h file not found, or not recent enough, trying to create it on the fly";
    QFile file(m_sourceName);
    if (file.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream stream(&file);
        const QString contents = stream.readAll();
        if (runUic(contents)) {
            if (debug)
                qDebug()<<"created on the fly";
            return;
        } else {
            // uic run was unsuccesfull
            if (debug)
                qDebug()<<"uic run wasn't succesfull";
            m_cacheTime = QDateTime ();
            m_contents = QByteArray();
            m_state = FINISHED;
            return;
        }
    } else {
        if (debug)
            qDebug()<<"Could open "<<m_sourceName<<"needed for the cpp model";
        m_contents = QByteArray();
        m_state = FINISHED;
    }
}

QByteArray UiCodeModelSupport::contents() const
{
    // Check the common case first
    if (m_state == FINISHED)
        return m_contents;
    if (m_state == BARE)
        init();
    if (m_state == RUNNING)
        finishProcess();

    return m_contents;
}

QString UiCodeModelSupport::fileName() const
{
    return m_fileName;
}

void UiCodeModelSupport::setFileName(const QString &name)
{
    if (m_fileName == name && m_cacheTime.isValid())
        return;

    if (m_state == RUNNING)
        finishProcess();

    if (debug)
        qDebug() << "UiCodeModelSupport::setFileName"<<name;

    m_fileName = name;
    m_contents.clear();
    m_cacheTime = QDateTime();
    m_state = BARE;
}

bool UiCodeModelSupport::runUic(const QString &ui) const
{
    const QString uic = uicCommand();
    if (uic.isEmpty())
        return false;
    m_process.setEnvironment(environment());

    if (debug)
        qDebug() << "UiCodeModelSupport::runUic " << uic << " on " << ui.size() << " bytes";
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
    if (debug)
        qDebug() << "failed" << m_process.readAllStandardError();
    m_process.kill();
    m_state = FINISHED;
    return false;
}

void UiCodeModelSupport::updateFromEditor(const QString &formEditorContents)
{
    if (m_state == BARE)
        init();
    if (m_state == RUNNING)
        finishProcess();
    if (runUic(formEditorContents))
        if (finishProcess())
            updateDocument();
}

bool UiCodeModelSupport::finishProcess() const
{
    if (m_state != RUNNING)
        return false;
    if (!m_process.waitForFinished(3000)
            && m_process.exitStatus() != QProcess::NormalExit
            && m_process.exitCode() != 0) {
        if (m_state != RUNNING) // waitForFinished can recurse into finishProcess
            return false;

        if (debug)
            qDebug() << "failed" << m_process.readAllStandardError();
        m_process.kill();
        m_state = FINISHED;
        return false;
    }

    if (m_state != RUNNING) // waitForFinished can recurse into finishProcess
        return true;

    m_contents = m_process.readAllStandardOutput();
    m_cacheTime = QDateTime::currentDateTime();
    if (debug)
        qDebug() << "ok" << m_contents.size() << "bytes.";
    m_state = FINISHED;
    return true;
}

void UiCodeModelSupport::updateFromBuild()
{
    if (debug)
        qDebug()<<"UiCodeModelSupport::updateFromBuild() for file"<<m_sourceName;
    if (m_state == BARE)
        init();
    if (m_state == RUNNING)
        finishProcess();
    // This is mostly a fall back for the cases when uic couldn't be run
    // it pays special attention to the case where a ui_*h was newly created
    QDateTime sourceTime = QFileInfo(m_sourceName).lastModified();
    if (m_cacheTime.isValid() && m_cacheTime >= sourceTime) {
        if (debug)
            qDebug()<<"Cache is still more recent then source";
        return;
    } else {
        QFileInfo fi(m_fileName);
        QDateTime uiHeaderTime = fi.exists() ? fi.lastModified() : QDateTime();
        if (uiHeaderTime.isValid() && (uiHeaderTime > sourceTime)) {
            if (m_cacheTime >= uiHeaderTime)
                return;
            if (debug)
                qDebug()<<"found ui*h updating from it";

            QFile file(m_fileName);
            if (file.open(QFile::ReadOnly | QFile::Text)) {
                QTextStream stream(&file);
                m_contents = stream.readAll().toUtf8();
                m_cacheTime = uiHeaderTime;
                updateDocument();
                return;
            }
        }
        if (debug)
            qDebug()<<"ui*h not found or not more recent then source not changing anything";
    }
}

