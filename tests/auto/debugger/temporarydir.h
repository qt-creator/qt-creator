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

// this file has been adapted from TemporaryDir implementation of Qt5

#ifndef TEMPORARYDIR_H_INCLUDED
#define TEMPORARYDIR_H_INCLUDED

#include <QtGlobal>

#if QT_VERSION >= 0x050000
    #include <QTemporaryDir>
#else // QT_VERSION < 0x050000

#include <QDir>
#include <stdlib.h> // mkdtemp

QT_BEGIN_NAMESPACE

//************* TemporaryDirPrivate
class TemporaryDirPrivate
{
public:
    TemporaryDirPrivate();
    ~TemporaryDirPrivate();

    void create(const QString &templateName);

    QString path;
    bool autoRemove;
    bool success;
};

TemporaryDirPrivate::TemporaryDirPrivate()
    : autoRemove(true),
      success(false)
{
}

TemporaryDirPrivate::~TemporaryDirPrivate()
{
}

static QString defaultTemplateName()
{
    QString baseName;
        baseName = QLatin1String("qt_temp");
    return QDir::tempPath() + QLatin1Char('/') + baseName + QLatin1String("-XXXXXX");
}

#ifdef Q_OS_WIN
static char *mkdtemp(char *templateName)
{
    static const char letters[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    const size_t length = strlen(templateName);
    char *XXXXXX = templateName + length - 6;
    if ((length < 6u) || strncmp(XXXXXX, "XXXXXX", 6))
        return 0;
    for (int i = 0; i < 256; ++i) {
        int v = qrand();
        /* Fill in the random bits.  */
        XXXXXX[0] = letters[v % 62];
        v /= 62;
        XXXXXX[1] = letters[v % 62];
        v /= 62;
        XXXXXX[2] = letters[v % 62];
        v /= 62;
        XXXXXX[3] = letters[v % 62];
        v /= 62;
        XXXXXX[4] = letters[v % 62];
        v /= 62;
        XXXXXX[5] = letters[v % 62];
        QString templateNameStr = QFile::decodeName(templateName);

        QFileInfo fileInfo(templateNameStr);
        if (fileInfo.absoluteDir().mkdir(fileInfo.fileName())) {
            if (!QFile::setPermissions(fileInfo.absoluteFilePath(), QFile::ReadOwner |
                                                                    QFile::WriteOwner |
                                                                    QFile::ExeOwner)) {
                continue;
            }
            return templateName;
        }
    }
    return 0;
}
#endif

void TemporaryDirPrivate::create(const QString &templateName)
{
    QByteArray buffer = QFile::encodeName(templateName);
    if (!buffer.endsWith("XXXXXX"))
        buffer += "XXXXXX";
    if (mkdtemp(buffer.data())) { // modifies buffer
        success = true;
        path = QFile::decodeName(buffer.constData());
    }
}

//************* TemporaryDir
static bool removeRecursively(QDir directory)
{
    if (!directory.exists())
        return true;
    bool success = true;
    QFileInfoList filesAndDirs = directory.entryInfoList(QDir::AllEntries | QDir::Hidden
                                                         | QDir::System | QDir::NoDotAndDotDot);
    foreach (QFileInfo fdInfo, filesAndDirs) {
        if (fdInfo.isDir())
            success &= removeRecursively(QDir(fdInfo.absoluteFilePath()));
        else
            success &= directory.remove(fdInfo.fileName());
    }
    if (success) {
        QDir parent(directory.absolutePath());
        success = parent.cdUp();
        if (success)
            success = parent.rmdir(directory.dirName());
    }
    return success;
}

class TemporaryDir
{
public:
    TemporaryDir();
    explicit TemporaryDir(const QString &templateName);
    ~TemporaryDir();

    bool autoRemove() {return d_ptr->autoRemove;}
    void setAutoRemove(bool b);
    bool remove();
    QString path() const;

private:
    QScopedPointer<TemporaryDirPrivate> d_ptr;

    Q_DISABLE_COPY(TemporaryDir)
};

TemporaryDir::TemporaryDir()
    : d_ptr(new TemporaryDirPrivate)
{
    d_ptr->create(defaultTemplateName());
}

TemporaryDir::TemporaryDir(const QString &templateName)
    : d_ptr(new TemporaryDirPrivate)
{
    if (templateName.isEmpty())
        d_ptr->create(defaultTemplateName());
    else
        d_ptr->create(templateName);
}

TemporaryDir::~TemporaryDir()
{
    if (d_ptr->autoRemove)
        remove();
}

QString TemporaryDir::path() const
{
    return d_ptr->path;
}

void TemporaryDir::setAutoRemove(bool b)
{
    d_ptr->autoRemove = b;
}

bool TemporaryDir::remove()
{
    if (!d_ptr->success)
        return false;
    Q_ASSERT(!path().isEmpty());
    Q_ASSERT(path() != QLatin1String("."));
    return removeRecursively(QDir(path()));
}

QT_END_NAMESPACE

typedef TemporaryDir QTemporaryDir;

#endif // QT_VERSION < 0x050000

#endif // TEMPORARYDIR_H_INCLUDED
