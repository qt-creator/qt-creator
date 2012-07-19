/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

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
      m_updateIncludingFiles(false),
      m_initialized(false)
{
    if (debug)
        qDebug()<<"ctor UiCodeModelSupport for"<<m_sourceName<<uiHeaderFile;
}

UiCodeModelSupport::~UiCodeModelSupport()
{
    if (debug)
        qDebug()<<"dtor ~UiCodeModelSupport for"<<m_sourceName;
}

void UiCodeModelSupport::init() const
{
    m_initialized = true;
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
            // and if the header file wasn't there, next time we need to update
            // all of the files that include this header
            if (!uiHeaderFileInfo.exists())
                m_updateIncludingFiles = true;
            return;
        }
    } else {
        if (debug)
            qDebug()<<"Could open "<<m_sourceName<<"needed for the cpp model";
        m_contents = QByteArray();
    }
}

QByteArray UiCodeModelSupport::contents() const
{
    if (!m_initialized)
        init();

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

    if (debug)
        qDebug() << "UiCodeModelSupport::setFileName"<<name;

    m_fileName = name;
    m_contents.clear();
    m_cacheTime = QDateTime();
    init();
}

bool UiCodeModelSupport::runUic(const QString &ui) const
{
    QProcess process;
    const QString uic = uicCommand();
    if (uic.isEmpty())
        return false;
    process.setEnvironment(environment());

    if (debug)
        qDebug() << "UiCodeModelSupport::runUic " << uic << " on " << ui.size() << " bytes";
    process.start(uic, QStringList(), QIODevice::ReadWrite);
    if (!process.waitForStarted())
        return false;
    process.write(ui.toUtf8());
    if (!process.waitForBytesWritten(3000))
        goto error;
    process.closeWriteChannel();
    if (!process.waitForFinished(3000) && process.exitStatus() != QProcess::NormalExit && process.exitCode() != 0)
        goto error;

    m_contents = process.readAllStandardOutput();
    m_cacheTime = QDateTime::currentDateTime();
    if (debug)
        qDebug() << "ok" << m_contents.size() << "bytes.";
    return true;

error:
    if (debug)
        qDebug() << "failed" << process.readAllStandardError();
    process.kill();
    return false;
}

void UiCodeModelSupport::updateFromEditor(const QString &formEditorContents)
{
    if (runUic(formEditorContents)) {
        updateDocument();
    }
}

void UiCodeModelSupport::updateFromBuild()
{
    if (debug)
        qDebug()<<"UiCodeModelSupport::updateFromBuild() for file"<<m_sourceName;
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

