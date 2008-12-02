/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/

#include "qinstaller.h"

#include <QtCore/QBuffer>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QProcess>
#include <QtCore/QSettings>

#ifdef Q_OS_WIN
#include "qt_windows.h"
#include <shlobj.h>
#endif

QT_BEGIN_NAMESPACE

/*
 FIXME: Documentation

NAME = "Name";
DISPLAY_NAME = "DisplayName";
DESCRIPTION = "Description";
TASK_COUNT = "TaskCount";
SIZE = "ComponentSize";
OUTPUT_FILE = "OutputFile";
WANTED_STATE = "WantedState";
SUGGESTED_STATE = "SuggestedState";
PRODUCT_NAME = "ProductName";
GUI_START_PAGE = "GuiStartPage";
RUN_PROGRAM = "RunProgram";
COMMENTS = "Comments";
CONTACT = "Contact";
DISPLAY_VERSION = "DisplayVersion";
ESTIMATED_SIZE = "EstimatedSize";
HELP_LINK = "HelpLink";
INSTALL_DATE = "InstallDate";
INSTALL_LOCATION = "InstallLocation";
NO_MODIFY = "NoModify";
NO_REPAIR = "NoRepair";
PUBLISHER = "Publisher";
UNINSTALL_STRING = "UninstallString";
URL_INFO_ABOUT = "UrlInfoAbout";
LOGO_PIXMAP
WATERMARK_PIXMAP
*/


//static qint64 magicInstallerMarker   = (0xdea0d345UL << 32) + 0x12023233UL;
//static qint64 magicUninstallerMarker = (0xdea0d345UL << 32) + 0x12023234UL;
static const qint64 magicInstallerMarker      =  0x12023233UL;
static const qint64 magicUninstallerMarker    =  0x12023234UL;
static const qint64 magicTempUninstallerMarker =  0x12023235UL;

// Installer Layout:
//
//   0000:                            <binary: installer code>
//     ...
//   $comptask[0]:                    <int:  $ctc[0]: 1st component task count>
//     $comptask[0][0]:               <task: first component task data>
//     ...
//     $comptask[0][$ctc0-1]:         <task: first component task data>
//    ...
//   $comptask[$ncomp-1]:             <int : $cnn: last component task data>
//     $comptask[$ncomp-1][0]:        <task: last component task data>
//     ...
//     $comtaskp[$ncomp-1][$ctc-1]:   <task: last component task data>
//   $compvars[0]:                    <dict:  $cn0: first component variables>
//     ...
//   $compvars[$ncomp-1]:             <dict:  $cnn: last component var data>
//   $comptaskoffsets:                <int:  $comptask[0]: offset>
//                                    <int:  $comptask[1]: offset>
//     ...
//   $compvarsoffsets:                <int:  $compvars[0]: offset>
//                                    <int:  $compvars[1]: offset>
//     ...
//   $installervars:                  <dict: installer variable data>
//     ...
//   end - 7:                         <int:  $comptask[0]: start of comp.tasks>
//   end - 6:                         <int:  $compvars[0]: start of .vars>
//   end - 5:                         <int:  $ncomp: number of components>
//   end - 4:                         <int:  $comptaskoffsets>
//   end - 3:                         <int:  $compvarsoffsets>
//   end - 2:                         <int:  $installervars: offset installer vars>
//   end - 1:                         <int:  magic marker>
//
// The stuff after the binary is not present in the "Creator" incarnation

////////////////////////////////////////////////////////////////////
//
// Misc helpers
//
////////////////////////////////////////////////////////////////////

namespace {

#define ifVerbose(s) if (!installer()->isVerbose()) {} else { qDebug() << s; }


QDebug &operator<<(QDebug &os, QInstallerTask *task)
{
    task->dump(os);
    return os;
}

class Dictionary : public QHash<QString, QString>
{
public:
    typedef QHash<QString, QString> BaseType;

    void setValue(const QString &key, const QString &val)
    {
        insert(key, val);
    }

    void removeTemporaryKeys()
    {
        foreach (const QString &key, keys())
            if (key.startsWith('@'))
                remove(key);
    }
};

//////////////////////////////////////////////////////////////////////

class Error : public QInstallerError
{
public:
    Error(const QString &m)
        : QInstallerError(m) {}
    // convenience
    Error(const char *m, int n)
        : QInstallerError(QString(QLatin1String(m)).arg(n)) {}
    Error(const char *m, const QString & n)
        : QInstallerError(QString(QLatin1String(m)).arg(n)) {}
private:
private:
    QString m_message;
};

} // anonymous namespace

static void openForWrite(QFile &file)
{
    if (!file.open(QIODevice::WriteOnly))
        throw Error("Cannot open file %1 for writing", file.fileName());
}

static void openForRead(QFile &file)
{
    if (!file.open(QIODevice::ReadOnly))
        throw Error("Cannot open file %1 for reading", file.fileName());
}

static void rawWrite(QIODevice *out, const char *buffer, qint64 size)
{
    while (size > 0) {
        qint64 n = out->write(buffer, size);
        if (n == -1)
            throw Error("RAW WRITE FAILED: %1", size);
        size -= n;
    }
}

static void rawRead(QIODevice *in, char *buffer, qint64 size)
{
    while (size > 0) {
        qint64 n = in->read(buffer, size);
        size -= n;
        buffer += n;
        if (size != 0)
            qDebug() << "COULD ONLY READ " << n << "OF" << size + n << "BYTES";
    }
}

static inline QByteArray &theBuffer(int size)
{
    static QByteArray b;
    if (size > b.size())
        b.reserve(size * 3 / 2);
    return b;
}


#if 0
// Faster or not?
static void appendFileData(QIODevice *out, const QString &fileName)
{
    QFile file(fileName);
    openForRead(file);
    qint64 size = file.size();
    QInstaller::appendInt(out, size);
    if (size == 0)
        return;
    uchar *data = file.map(0, size);
    if (!data)
        throw Error(QInstaller::tr("Cannot map file %1").arg(file.fileName()));
    rawWrite(out, (const char *)data, size);
    if (!file.unmap(data))
        throw Error(QInstaller::tr("Cannot unmap file %1").arg(file.fileName()));
}
#endif

static void appendFileData(QIODevice *out, QIODevice *in)
{
    Q_ASSERT(!in->isSequential());
    qint64 size = in->size();
    QByteArray &b = theBuffer(size);
    rawRead(in, b.data(), size);
    QInstaller::appendInt(out, size);
    rawWrite(out, b.constData(), size);
}

static void retrieveFileData(QIODevice *out, QIODevice *in)
{
    qint64 size = QInstaller::retrieveInt(in);
    QByteArray &b = theBuffer(size);
    rawRead(in, b.data(), size);
    rawWrite(out, b.constData(), size);
}

static void appendData(QIODevice *out, QIODevice *in, qint64 size)
{
    QByteArray &b = theBuffer(size);
    rawRead(in, b.data(), size);
    rawWrite(out, b.constData(), size);
}

static void appendInt(QIODevice *out, qint64 n)
{
    rawWrite(out, (char*)&n, sizeof(n));
}

static void appendString(QIODevice *out, const QString &str)
{
    int n = str.size();
    appendInt(out, n);
    rawWrite(out, (char*)str.utf16(), n * sizeof(QChar));
}

static void appendByteArray(QIODevice *out, const QByteArray &ba)
{
    appendInt(out, ba.size());
    rawWrite(out, ba.constData(), ba.size());
}

static void appendDictionary(QIODevice *out, const Dictionary &dict)
{
    appendInt(out, dict.size());
    foreach (const QString &key, dict.keys()) {
        appendString(out, key);
        appendString(out, dict.value(key));
    }
}

static qint64 retrieveInt(QIODevice *in)
{
    qint64 n = 0;
    in->read((char*)&n, sizeof(n));
    return n;
}

static QString retrieveString(QIODevice *in)
{
    static QByteArray b;
    qint64 n = retrieveInt(in);
    if (n * int(sizeof(QChar)) > b.size())
        b.reserve(n * sizeof(QChar) * 3 / 2);
    in->read(b.data(), n * sizeof(QChar));
    QString str((QChar *)b.data(), n);
    return str;
}

static QByteArray retrieveByteArray(QIODevice *in)
{
    QByteArray ba;
    qint64 n = retrieveInt(in);
    ba.resize(n);
    rawRead(in, ba.data(), n);
    return ba;
}

static Dictionary retrieveDictionary(QIODevice *in)
{
    Dictionary dict;
    for (qint64 i = retrieveInt(in); --i >= 0; ) {
        QString key = retrieveString(in);
        dict.insert(key, retrieveString(in));
    }
    return dict;
}

////////////////////////////////////////////////////////////////////
//
// QInstallerComponent::Private
//
////////////////////////////////////////////////////////////////////

class QInstallerComponent::Private
{
public:
    QInstaller *m_installer;
    Dictionary m_vars;
    QList<QInstallerTask *> m_tasks;

    // filled before intaller runs
    qint64 m_offsetInInstaller;
};

#if 0

// this is dead slow as QDirIterator::Private::advance needlessly
// creates tons of QFileInfo objects

static void listDir
    (const QString &sourcePath0, const QString &targetPath0,
    QList<QInstallerTask *> *copyTasks,
    QList<QInstallerTask *> *linkTasks,
    int sourcePathLength)
{
    Q_UNUSED(sourcePathLength);
    QDirIterator it(sourcePath0, QDir::Files, QDirIterator::Subdirectories);
    const int pos = sourcePath0.size();
    while (it.hasNext()) {
        QString sourcePath = it.next();
        QFileInfo sourceInfo = it.fileInfo();
        if (sourceInfo.isSymLink()) {
            QFileInfo absSourceInfo(sourceInfo.absoluteFilePath());
            //QString linkTarget = sourceInfo.symLinkTarget();
            QString absLinkTarget = absSourceInfo.symLinkTarget();
            //QString relPath = sourceInfo.dir().relativeFilePath(linkTarget);
            QString absRelPath = absSourceInfo.dir().relativeFilePath(absLinkTarget);
            if (0) {
                ifVerbose("\n\nCREATING LINK: "
                    << "\nSOURCE : " << sourceInfo.filePath()
                    << "\nSOURCE ABS: " << absSourceInfo.filePath()
                    //<< "\nLINK : " << linkTarget
                    << "\nLINK ABS: " << absLinkTarget
                    //<< "\nREL : " << relPath
                    << "\nREL ABS: " << absRelPath);
            }
            QString targetPath = targetPath0;
            targetPath += sourcePath.midRef(pos);
            //qDebug() << "LINK " << absRelPath << targetPath << targetPath0;
            QInstallerLinkFileTask *task = new QInstallerLinkFileTask(m_installer);
            task->setLinkTargetPath(absRelPath);
            task->setTargetPath(targetPath);
            task->setPermissions(sourceInfo.permissions());
            linkTasks->append(task);
        } else {
            QInstallerCopyFileTask *task = new QInstallerCopyFileTask(m_installer);
            QString targetPath = targetPath0;
            targetPath += sourcePath.midRef(pos);
            //qDebug() << "COPY " << sourcePath << targetPath << targetPath0;
            task->setSourcePath(sourcePath);
            task->setTargetPath(targetPath);
            task->setPermissions(sourceInfo.permissions());
            copyTasks.append(task);
        }
    }
}
#else

static void listDir
    (const QString &sourcePath, const QString &targetPath0,
    QList<QInstallerTask *> *copyTasks,
    QList<QInstallerTask *> *linkTasks,
    QInstaller *installer,
    int sourcePathLength = -1)

{
    if (sourcePathLength == -1)
        sourcePathLength = sourcePath.size();
    QFileInfo sourceInfo(sourcePath);
    if (sourceInfo.isDir()) {
        QDir dir(sourcePath);
        dir.setSorting(QDir::Unsorted);
        foreach (const QFileInfo &fi, dir.entryInfoList()) {
            QString sourceFile = fi.fileName();
            if (sourceFile == QLatin1String(".")
                    || sourceFile == QLatin1String(".."))
                continue;
            listDir(sourcePath + '/' + sourceFile, targetPath0,
                copyTasks, linkTasks, installer, sourcePathLength);
        }
    } else if (sourceInfo.isSymLink()) {
        QFileInfo absSourceInfo(sourceInfo.absoluteFilePath());
        QString absLinkTarget = absSourceInfo.symLinkTarget();
        QString absRelPath = absSourceInfo.dir().relativeFilePath(absLinkTarget);
        QString targetPath = targetPath0;
        targetPath += sourcePath.midRef(sourcePathLength);
        //qDebug() << "LINK " << absRelPath << targetPath << targetPath0;
        QInstallerLinkFileTask *task = new QInstallerLinkFileTask(installer);
        task->setLinkTargetPath(absRelPath);
        task->setTargetPath(targetPath);
        task->setPermissions(sourceInfo.permissions());
        linkTasks->append(task);
    } else {
        QInstallerCopyFileTask *task = new QInstallerCopyFileTask(installer);
        QString targetPath = targetPath0;
        targetPath += sourcePath.midRef(sourcePathLength);
        //qDebug() << "COPY " << sourcePath << targetPath << targetPath0;
        task->setSourcePath(sourcePath);
        task->setTargetPath(targetPath);
        task->setPermissions(sourceInfo.permissions());
        copyTasks->append(task);
    }
}
#endif


////////////////////////////////////////////////////////////////////
//
// QInstallerComponent
//
////////////////////////////////////////////////////////////////////

QInstallerComponent::QInstallerComponent(QInstaller *installer)
  : d(new QInstallerComponent::Private)
{
    d->m_installer = installer;
}


QInstallerComponent::~QInstallerComponent()
{
    qDeleteAll(d->m_tasks);
    d->m_tasks.clear();
    delete d;
}

QString QInstallerComponent::value(const QString &key,
    const QString &defaultValue) const
{
    return d->m_vars.value(key, defaultValue);
}

void QInstallerComponent::setValue(const QString &key, const QString &value)
{
    d->m_vars[key] = value;
}

void QInstallerComponent::appendDirectoryTasks
    (const QString &sourcePath0, const QString &targetPath)
{
    QList<QInstallerTask *> copyTasks;
    QList<QInstallerTask *> linkTasks;
    QString sourcePath = d->m_installer->replaceVariables(sourcePath0);
    listDir(sourcePath, targetPath, &copyTasks, &linkTasks, d->m_installer);
    d->m_tasks += copyTasks;
    d->m_tasks += linkTasks;
}

void QInstallerComponent::appendSettingsTask
    (const QString &key, const QString &value)
{
    QInstallerWriteSettingsTask *task =
        new QInstallerWriteSettingsTask(d->m_installer);
    task->setKey(key);
    task->setValue(value);
    appendTask(task);
}

void QInstallerComponent::appendTask(QInstallerTask *task)
{
    d->m_tasks.append(task);
}

int QInstallerComponent::taskCount() const
{
    return d->m_tasks.size();
}

QInstallerTask *QInstallerComponent::task(int i) const
{
    return d->m_tasks.at(i);
}

////////////////////////////////////////////////////////////////////
//
// QInstaller::Private
//
////////////////////////////////////////////////////////////////////

QInstallerTask *createCopyFileTask(QInstaller *installer)
{
    return new QInstallerCopyFileTask(installer);
}

QInstallerTask *createLinkFileTask(QInstaller *installer)
{
    return new QInstallerLinkFileTask(installer);
}

QInstallerTask *createWriteSettingsTask(QInstaller *installer)
{
    return new QInstallerWriteSettingsTask(installer);
}

QInstallerTask *createPatchFileTask(QInstaller *installer)
{
    return new QInstallerPatchFileTask(installer);
}

QInstallerTask *createMenuShortcutTask(QInstaller *installer)
{
    return new QInstallerMenuShortcutTask(installer);
}



class QInstaller::Private : public QObject
{
    Q_OBJECT;

public:
    explicit Private(QInstaller *);
    ~Private();

    void initialize();

    QInstallerTask *createTaskFromCode(int code);
    void undo(const QList<QInstallerTask *> &tasks);
    void writeUninstaller(const QList<QInstallerTask *> &tasks);
    bool statusCanceledOrFailed() const;

    void writeInstaller(QIODevice *out);
    void writeInstaller();
    void appendCode(QIODevice *out);
    void runInstaller();
    void runUninstaller();
    void deleteUninstaller();
    QString uninstallerName() const;
    QString replaceVariables(const QString &str) const;
    QByteArray replaceVariables(const QByteArray &str) const;
    QString registerPath() const;
    void registerInstaller();
    void unregisterInstaller();
    QString installerBinaryPath() const;
    bool isCreator() const;
    bool isInstaller() const;
    bool isUninstaller() const;
    bool isTempUninstaller() const;
    QInstaller *installer() const { return q; }
    bool restartTempUninstaller(const QStringList &args);
    void setInstallationProgress(qint64 progress); // relative to m_totalProgress

signals:
    void installationStarted();
    void installationFinished();
    void uninstallationStarted();
    void uninstallationFinished();

public:
    QInstaller *q;

    Dictionary m_vars;
    QInstaller::InstallerStatus m_status;
    bool m_verbose;

    qint64 m_codeSize;
    qint64 m_tasksStart;
    qint64 m_variablesStart;
    qint64 m_componentCount;
    qint64 m_tasksOffsetsStart;
    qint64 m_variablesOffsetsStart;
    qint64 m_variableDataStart;
    qint64 m_magicMarker;
    
    int m_installationProgress;
    int m_totalProgress;
    QString m_installationProgressText;

    // Owned. Indexed by component name
    QList<QInstallerComponent *> m_components;
    QList<QInstaller::TaskCreator> m_taskCreators;
};

QInstaller::Private::Private(QInstaller *q_)
  : q(q_), m_status(QInstaller::InstallerUnfinished), m_verbose(false)
{
    connect(this, SIGNAL(installationStarted()),
        q, SIGNAL(installationStarted()));
    connect(this, SIGNAL(installationFinished()),
        q, SIGNAL(installationFinished()));
    connect(this, SIGNAL(uninstallationStarted()),
        q, SIGNAL(uninstallationStarted()));
    connect(this, SIGNAL(uninstallationFinished()),
        q, SIGNAL(uninstallationFinished()));
}

QInstaller::Private::~Private()
{
    qDeleteAll(m_components);
    m_components.clear();
}


void QInstaller::Private::initialize()
{
    m_installationProgress = 0;
    m_totalProgress = 100;

    m_vars["ProductName"] = "Unknown Product";
    m_vars["LogoPixmap"] = ":/resources/logo.png";
    m_vars["WatermarkPixmap"] = ":/resources/watermark.png";

    QFile in(installerBinaryPath());
    openForRead(in);
    m_codeSize = in.size();

    // this reads bogus values for 'creators', but it does not harm
    in.seek(in.size() - 7 * sizeof(qint64));
    m_tasksStart = retrieveInt(&in);
    m_variablesStart = retrieveInt(&in);
    m_componentCount = retrieveInt(&in);
    m_tasksOffsetsStart = retrieveInt(&in);
    m_variablesOffsetsStart = retrieveInt(&in);
    m_variableDataStart = retrieveInt(&in);
    m_magicMarker = retrieveInt(&in);

    if (isCreator()) {
        // valgrind complains otherwise
        m_tasksStart = 0;
        m_variablesStart = 0;
        m_componentCount = 0;
        m_tasksOffsetsStart = 0;
        m_variablesOffsetsStart = 0;
        m_variableDataStart = 0;
        m_magicMarker = 0;
    } else {
        // fix code size
        m_codeSize = m_tasksStart;

        // merge stored variables
        in.seek(m_variablesStart);

        for (int i = 0; i != m_componentCount; ++i) {
            QInstallerComponent *component = new QInstallerComponent(q);
            component->d->m_vars = retrieveDictionary(&in);
            qDebug() << "DICT  " << i << component->d->m_vars;
            m_components.append(component);
        }

        // read installer variables
        Dictionary dict = retrieveDictionary(&in);
        if (m_verbose)
            qDebug() << "READ VARIABLES FROM INSTALLER:" << dict;
        foreach (const QString &key, dict.keys()) {
            if (!m_vars.contains(key))
                m_vars.insert(key, dict.value(key));
        }
        if (m_verbose)
            qDebug() << "MERGED VARIABLES:" << m_vars;
    }
}

void QInstaller::Private::setInstallationProgress(qint64 progress)
{
    // from 0 to m_totalProgress
    int percent = progress * 100 / m_totalProgress;
    if (percent == m_installationProgress)
        return;
    //qDebug() << "setting progress to " << progress
    //    << " of " << m_totalProgress << " " << percent << "%";
    m_installationProgress = percent;
    qApp->processEvents();
}

QString QInstaller::Private::installerBinaryPath() const
{
    return qApp->arguments().at(0);
}

bool QInstaller::Private::isCreator() const
{
    return !isInstaller() && !isUninstaller() && !isTempUninstaller();
}

bool QInstaller::Private::isInstaller() const
{
    return m_magicMarker == magicInstallerMarker;
}

bool QInstaller::Private::isUninstaller() const
{
    return m_magicMarker == magicUninstallerMarker;
}

bool QInstaller::Private::isTempUninstaller() const
{
    return m_magicMarker == magicTempUninstallerMarker;
}

void QInstaller::Private::writeInstaller()
{
    QString fileName = m_vars["OutputFile"];
#ifdef Q_OS_WIN
    if (!fileName.endsWith(QLatin1String(".exe")))
        fileName += QLatin1String(".exe");
#endif
    QFile out;
    out.setFileName(fileName);
    openForWrite(out);
    writeInstaller(&out);
    out.setPermissions(out.permissions() | QFile::WriteUser
        | QFile::ExeOther | QFile::ExeGroup | QFile::ExeUser);
}

void QInstaller::Private::writeInstaller(QIODevice *out)
{
    appendCode(out);

    QList<qint64> tasksOffsets;
    QList<qint64> variablesOffsets;

    // write component task data
    foreach (QInstallerComponent *component, m_components) {
        qint64 componentStart = out->size();
        tasksOffsets.append(out->size()); // record start of tasks
        // pack the component as a whole
        QBuffer buffer;
        buffer.open(QIODevice::WriteOnly);
        foreach (QInstallerTask *task, component->d->m_tasks) {
            appendInt(&buffer, q->indexOfTaskType(task->creator()));
            task->writeToInstaller(&buffer);
        }
        buffer.close();
        QByteArray compressed = qCompress(buffer.buffer());
        int uncompressedSize = buffer.buffer().size();
        int compressedSize = compressed.size();
        appendByteArray(out, compressed);
        qDebug() << "COMPRESS: " << uncompressedSize << compressedSize;
        component->setValue("TaskCount", QString::number(component->d->m_tasks.size()));
        component->setValue("ComponentStart", QString::number(componentStart));
        component->setValue("CompressedSize", QString::number(compressedSize));
        component->setValue("UncompressedSize", QString::number(uncompressedSize));
    }

    // write component variables
    foreach (QInstallerComponent *component, m_components) {
        variablesOffsets.append(out->size()); // record start of variables
        appendDictionary(out, component->d->m_vars);
    }

    // append variables except temporary ones
    qint64 variableDataStart = out->size();
    Dictionary dict = m_vars;
    dict.removeTemporaryKeys();
    appendDictionary(out, dict);

    // append recorded list of component task offsets
    qint64 taskOffsetsStart = out->size();
    foreach (qint64 offset, tasksOffsets)
        appendInt(out, offset);

    // append recorded list of component varaibles offsets
    qint64 variablesOffsetsStart = out->size();
    foreach (qint64 offset, variablesOffsets)
        appendInt(out, offset);

    // append trailer
    appendInt(out, tasksOffsets[0]);
    appendInt(out, variablesOffsets[0]);
    appendInt(out, m_components.size());
    appendInt(out, taskOffsetsStart);
    appendInt(out, variablesOffsetsStart);
    appendInt(out, variableDataStart);
    appendInt(out, magicInstallerMarker);
}

bool QInstaller::Private::statusCanceledOrFailed() const
{
    return m_status == QInstaller::InstallerCanceledByUser
        || m_status == QInstaller::InstallerFailed;
}

QInstallerTask *QInstaller::Private::createTaskFromCode(int code)
{
    if (code >= 0 && code < m_taskCreators.size())
        return m_taskCreators[code](q);
    throw Error("NO TASK WITH CODE %1  REGISTERED");
}

void QInstaller::Private::undo(const QList<QInstallerTask *> &tasks)
{
    //qDebug() << "REMOVING" << files.size();
    // tasks.size() corresponds to m_installationProgress;
    m_totalProgress = tasks.size() * m_installationProgress / 100 + 1;
    for (int i = tasks.size(); --i >= 0; ) {
        QInstallerTask *task = tasks.at(i);
        setInstallationProgress(i);
        task->undo();
    }
    setInstallationProgress(0);
}

void QInstaller::Private::appendCode(QIODevice *out)
{
    QFile in(installerBinaryPath());
    openForRead(in);
    if (m_verbose)
        qDebug() << "CODE SIZE: " << m_codeSize;
    appendData(out, &in, m_codeSize);
    in.close();
}

QString QInstaller::Private::replaceVariables(const QString &str) const
{
    QString res;
    int pos = 0;
    while (true) {
        int pos1 = str.indexOf('@', pos);
        if (pos1 == -1)
            break;
        int pos2 = str.indexOf('@', pos1 + 1);
        if (pos2 == -1)
            break;
        res += str.mid(pos, pos1 - pos);
        QString name = str.mid(pos1 + 1, pos2 - pos1 - 1);
        res += m_vars.value(name);
        pos = pos2 + 1;
    }
    res += str.mid(pos);
    return res;
}

QByteArray QInstaller::Private::replaceVariables(const QByteArray &ba) const
{
    QByteArray res;
    int pos = 0;
    while (true) {
        int pos1 = ba.indexOf('@', pos);
        if (pos1 == -1)
            break;
        int pos2 = ba.indexOf('@', pos1 + 1);
        if (pos2 == -1)
            break;
        res += ba.mid(pos, pos1 - pos);
        QString name = QString::fromLocal8Bit(ba.mid(pos1 + 1, pos2 - pos1 - 1));
        res += m_vars.value(name).toLocal8Bit();
        pos = pos2 + 1;
    }
    res += ba.mid(pos);
    return res;
}

QString QInstaller::Private::uninstallerName() const
{
    QString name = m_vars["TargetDir"];
    name += "/uninstall";
#ifdef Q_OS_WIN
    name += QLatin1String(".exe");
#endif
    return name;
}

void QInstaller::Private::writeUninstaller(const QList<QInstallerTask *> &tasks)
{
    QFile out(uninstallerName());
    try {
        ifVerbose("CREATING UNINSTALLER " << tasks.size());
        // Create uninstaller. this is basically a clone of ourselves
        // with a few changed variables
        openForWrite(out);
        appendCode(&out);
        qint64 tasksStart = out.size();
        appendInt(&out, tasks.size());

        for (int i = tasks.size(); --i >= 0; ) {
            QInstallerTask *task = tasks.at(i);
            appendInt(&out, m_taskCreators.indexOf(task->creator()));
            task->writeToUninstaller(&out); // might throw
        }

        // append variables except temporary ones
        qint64 variableDataStart = out.size();
        Dictionary dict = m_vars;
        dict.removeTemporaryKeys();
        dict.setValue(QLatin1String("UninstallerPath"), uninstallerName());
        appendDictionary(&out, dict);

        // append trailer
        appendInt(&out, tasksStart);
        appendInt(&out, variableDataStart); // variablesStart
        appendInt(&out, 0); // componentCount
        appendInt(&out, 0); // tasksOffsetsStart
        appendInt(&out, 0); // variablesOffsetsStart
        appendInt(&out, variableDataStart);
        appendInt(&out, magicUninstallerMarker);

        out.setPermissions(out.permissions() | QFile::WriteUser
            | QFile::ExeOther | QFile::ExeGroup | QFile::ExeUser);
    }
    catch (const QInstallerError &err) {
        m_status = QInstaller::InstallerFailed;
        // local roll back
        qDebug() << "WRITING TO UNINSTALLER FAILED: " << err.message();
        out.close();
        out.remove();
        throw;
    }
}

QString QInstaller::Private::registerPath() const
{
    QString productName = m_vars["ProductName"];
    if (productName.isEmpty())
        throw QInstallerError("ProductName should be set");
    QString path;
    if (m_vars["AllUsers"] == "true")
        path += "HKEY_LOCAL_MACHINE";
    else
        path += "HKEY_CURRENT_USER";
    path += "\\Software\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\";
    path += productName;
    return path;
}

void QInstaller::Private::registerInstaller()
{
#ifdef Q_OS_WIN
    QSettings settings(registerPath(), QSettings::NativeFormat);
    settings.beginGroup("CurrentVersion");
    settings.beginGroup("Uninstall");
    settings.beginGroup(m_vars["ProductName"]);
    settings.setValue("Comments", m_vars["Comments"]);
    settings.setValue("Contact", m_vars["Contact"]);
    settings.setValue("DisplayName", m_vars["ProductName"]);
    settings.setValue("DisplayVersion", m_vars["DisplayVersion"]);
    settings.setValue("EstimatedSize", "X4957efb0");
    settings.setValue("HelpLink", m_vars["HelpLink"]);
    settings.setValue("InstallDate", QDateTime::currentDateTime().toString());
    settings.setValue("InstallLocation", m_vars["TargetDir"]);
    settings.setValue("NoModify", "1");
    settings.setValue("NoRepair", "1");
    settings.setValue("Publisher", m_vars["Publisher"]);
    settings.setValue("UninstallString", uninstallerName());
    settings.setValue("UrlInfoAbout", m_vars["UrlInfoAbout"]);
#endif
}

void QInstaller::Private::unregisterInstaller()
{
#ifdef Q_OS_WIN
    QSettings settings(registerPath(), QSettings::NativeFormat);
    settings.remove(QString());
#endif
}

void QInstaller::Private::runInstaller()
{
    QList<QInstallerTask *> tasks;

    try {
        emit installationStarted();
        if (!m_vars.contains("TargetDir"))
            throw QInstallerError(QLatin1String("Variable 'TargetDir' not set."));

        QFile in(installerBinaryPath());
        openForRead(in);

        m_totalProgress = 0;
        QList<QInstallerComponent *> componentsToInstall;

        for (int i = 0; i != m_componentCount; ++i) {
            QInstallerComponent *comp = m_components.at(i);
            QString wantedState = comp->value("WantedState");
            ifVerbose("HANDLING COMPONENT" << i << "WANTED: " << wantedState);
            if (wantedState == "Uninstalled") {
                qDebug() << "SKIPPING COMPONENT" << comp->value("DisplayName");
                continue;
            }
            componentsToInstall.append(comp);
            m_totalProgress += comp->value("UncompressedSize").toInt();
        }
            
        qDebug() << "Install size: " << m_totalProgress
            << "in " << componentsToInstall.size() << "components";

        qint64 lastProgressBase = 0;
        foreach (QInstallerComponent *comp, componentsToInstall) {
            int taskCount = comp->value("TaskCount").toInt();
            quint64 componentStart = comp->value("ComponentStart").toInt();
            in.seek(componentStart);
            if (statusCanceledOrFailed())
                throw Error("Installation canceled by user");
            m_installationProgressText =
                tr("Decompressing component %1").arg(comp->value("DisplayName"));
            qApp->processEvents();
            QByteArray compressed = retrieveByteArray(&in);
            QByteArray uncompressed = qUncompress(compressed);
            if (uncompressed.isEmpty()) {
                qDebug() << "SIZE: " << compressed.size() << " TASK COUNT: " << taskCount
                    << uncompressed.size();
                throw Error("DECOMPRESSION FAILED");
            }
            QBuffer buffer(&uncompressed);
            buffer.open(QIODevice::ReadOnly);
            for (int j = 0; j != taskCount; ++j) {
                int code = retrieveInt(&buffer);
                QInstallerTask *task = createTaskFromCode(code); // might throw
                task->readAndExecuteFromInstaller(&buffer); // might throw
                tasks.append(task);
                if (statusCanceledOrFailed())
                    throw Error("Installation canceled by user");
                setInstallationProgress(lastProgressBase + buffer.pos());
            }
            comp->setValue("CurrentState", "Installed");
            lastProgressBase += uncompressed.size();
        }
        in.close();

        registerInstaller();
        writeUninstaller(tasks);

        m_status = InstallerSucceeded;
        m_installationProgressText = tr("Installation finished!");
        qApp->processEvents();
        emit installationFinished();
    }
    catch (const QInstallerError &err) {
        installer()->showWarning(err.message());
        qDebug() << "INSTALLER FAILED: " << err.message() << "\nROLLING BACK";
        undo(tasks);
        m_installationProgressText = tr("Installation aborted");
        qApp->processEvents();
        emit installationFinished();
        throw;
    }
}

bool QInstaller::Private::restartTempUninstaller(const QStringList &args)
{
#ifdef Q_OS_WIN
    ifVerbose("Running uninstaller on Windows.");
    if (isUninstaller()) {
        QString uninstallerFile = installerBinaryPath();
        QDir tmpDir = QDir::temp();
        QString tmpDirName = QLatin1String("qtcreator_uninst");
        QString tmpAppName = QLatin1String("uninst.exe");
        if (!tmpDir.exists(tmpDirName)) {
            tmpDir.mkdir(tmpDirName);
            if (!tmpDir.exists(tmpDirName)) {
                ifVerbose("Could not create temporary folder!");
                return false;
            }
            tmpDir.cd(tmpDirName);
        }

        if (tmpDir.exists(tmpAppName) && !tmpDir.remove(tmpAppName)) {
            ifVerbose("Could not remove old temporary uninstaller!");
            return false;
        }

        QString tmpUninstaller = tmpDir.absoluteFilePath(tmpAppName);

        QFile in(uninstallerFile);
        if (!in.open(QIODevice::ReadOnly)) {
            ifVerbose("Cannot open uninstall.exe!");
            return false;
        }

        QFile out(tmpUninstaller);
        if (!out.open(QIODevice::WriteOnly)) {
            ifVerbose("Cannot open temporary uninstall.exe!");
            return false;
        }

        QByteArray ba = in.readAll();
        QBuffer buf(&ba);
        buf.open(QIODevice::ReadWrite);
        buf.seek(buf.size() - sizeof(qint64));
        appendInt(&buf, magicTempUninstallerMarker);
        if (in.size() != out.write(buf.data())) {
            ifVerbose("Could not copy uninstaller!");
            return false;
        }

        in.close();
        out.close();

        MoveFileExW((TCHAR*)tmpUninstaller.utf16(),
            0, MOVEFILE_DELAY_UNTIL_REBOOT|MOVEFILE_REPLACE_EXISTING);

        STARTUPINFOW sInfo;
        PROCESS_INFORMATION pInfo;
        memset(&sInfo, 0, sizeof(sInfo));
        memset(&pInfo, 0, sizeof(pInfo));
        sInfo.cb = sizeof(sInfo);

        QString cmd = QString("\"%1\"").arg(tmpUninstaller);
        foreach (const QString &s, args)
            cmd.append(QLatin1String(" \"") + s + QLatin1String("\""));
        if (CreateProcessW(0, (TCHAR*)cmd.utf16(), 0, 0, false, 0, 0, 0, &sInfo, &pInfo)) {
            CloseHandle(pInfo.hThread);
            CloseHandle(pInfo.hProcess);
            ifVerbose("Started temp uninstaller.");
        } else {
            ifVerbose("Cannot launch uninstaller!");
            return false;
        }
    }
#else
    Q_UNUSED(args);
#endif
    return true;
}

void QInstaller::Private::runUninstaller()
{
    QFile uninstaller(installerBinaryPath());
    openForRead(uninstaller);
    QByteArray ba = uninstaller.readAll();
    uninstaller.close();

    emit uninstallationStarted();
#ifndef Q_OS_WIN
    // remove uninstaller binary itself. Also necessary for successful
    // removal of the application directory.
    uninstaller.remove();
#else
    if (m_vars.contains(QLatin1String("UninstallerPath"))) {
        QFile orgUninstaller(m_vars.value(QLatin1String("UninstallerPath")));
        orgUninstaller.remove();
    }
#endif

    // read file
    QBuffer in(&ba);
    in.open(QIODevice::ReadOnly);
    in.seek(m_tasksStart);
    qint64 taskCount = retrieveInt(&in);
    ifVerbose("FOUND " << taskCount << "UNINSTALLER TASKS");

    m_totalProgress = m_variablesStart;
    for (int i = 0; i != taskCount; ++i) {
        int code = retrieveInt(&in);
        QInstallerTask *task = createTaskFromCode(code);
        task->readAndExecuteFromUninstaller(&in);
        setInstallationProgress(in.pos());
    }
    in.close();

    unregisterInstaller();

    m_installationProgressText = tr("Deinstallation finished");
    qApp->processEvents();
    emit uninstallationFinished();
}


////////////////////////////////////////////////////////////////////
//
// QInstaller
//
////////////////////////////////////////////////////////////////////

QInstaller::QInstaller()
{
    d = new Private(this);
    d->initialize();
    registerTaskType(createCopyFileTask);
    registerTaskType(createLinkFileTask);
    registerTaskType(createPatchFileTask);
    registerTaskType(createWriteSettingsTask);
    registerTaskType(createMenuShortcutTask);
}

QInstaller::~QInstaller()
{
    delete d;
}

void QInstaller::appendComponent(QInstallerComponent *component)
{
    d->m_components.append(component);
}

int QInstaller::componentCount() const
{
    return d->m_components.size();
}

QInstallerComponent *QInstaller::component(int i) const
{
    return d->m_components.at(i);
}

void QInstaller::registerTaskType(TaskCreator tc)
{
    d->m_taskCreators.append(tc);
}

int QInstaller::indexOfTaskType(TaskCreator tc) const
{
    return d->m_taskCreators.indexOf(tc);
}

QString QInstaller::value(const QString &key, const QString &defaultValue) const
{
    return d->m_vars.value(key, defaultValue);
}

void QInstaller::setValue(const QString &key, const QString &value)
{
    d->m_vars[key] = value;
}

bool QInstaller::containsValue(const QString &key) const
{
    return d->m_vars.contains(key);
}

bool QInstaller::isVerbose() const
{
    return d->m_verbose;
}

void QInstaller::setVerbose(bool on)
{
    d->m_verbose = on;
}

QInstaller::InstallerStatus QInstaller::status() const
{
    return d->m_status;
}

void QInstaller::interrupt()
{
    qDebug() << "INTERRUPT INSTALLER";
    d->m_status = InstallerCanceledByUser;
}

QString QInstaller::replaceVariables(const QString &str) const
{
    return d->replaceVariables(str);
}

QByteArray QInstaller::replaceVariables(const QByteArray &ba) const
{
    return d->replaceVariables(ba);
}

int QInstaller::installationProgress() const
{
    return d->m_installationProgress;
}

void QInstaller::setInstallationProgressText(const QString &value)
{
    d->m_installationProgressText = value;
}

QString QInstaller::installationProgressText() const
{
    return d->m_installationProgressText;
}

QString QInstaller::installerBinaryPath() const
{
    return d->installerBinaryPath();
}

bool QInstaller::isCreator() const
{
    return d->isCreator();
}

bool QInstaller::isInstaller() const
{
    return d->isInstaller();
}

bool QInstaller::isUninstaller() const
{
    return d->isUninstaller();
}

bool QInstaller::isTempUninstaller() const
{
    return d->isTempUninstaller();
}

bool QInstaller::runInstaller()
{
    try { d->runInstaller(); return true; } catch (...) { return false; }
}

bool QInstaller::runUninstaller()
{
    try { d->runUninstaller(); return true; } catch (...) { return false; }
}

void QInstaller::showWarning(const QString &str)
{
    emit warning(str);
}

void QInstaller::dump() const
{
    qDebug() << "COMMAND LINE VARIABLES:" << d->m_vars;
}

void QInstaller::appendInt(QIODevice *out, qint64 n)
{
    QT_PREPEND_NAMESPACE(appendInt)(out, n);
}

void QInstaller::appendString(QIODevice *out, const QString &str)
{
    QT_PREPEND_NAMESPACE(appendString)(out, str);
}

void QInstaller::appendByteArray(QIODevice *out, const QByteArray &str)
{
    QT_PREPEND_NAMESPACE(appendByteArray)(out, str);
}

qint64 QInstaller::retrieveInt(QIODevice *in)
{
    return QT_PREPEND_NAMESPACE(retrieveInt)(in);
}

QString QInstaller::retrieveString(QIODevice *in)
{
    return QT_PREPEND_NAMESPACE(retrieveString)(in);
}

QByteArray QInstaller::retrieveByteArray(QIODevice *in)
{
    return QT_PREPEND_NAMESPACE(retrieveByteArray)(in);
}

bool QInstaller::run()
{
    try {
        if (isCreator()) {
            createTasks(); // implemented in derived classes
            d->writeInstaller();
        } else if (isInstaller()) {
            d->runInstaller();
        } else if (isUninstaller() || isTempUninstaller()) {
            runUninstaller();
        }
        return true;
    } catch (const QInstallerError &err) {
        qDebug() << "Caught Installer Error: " << err.message();
        return false;
    }
}

QString QInstaller::uninstallerName() const
{
    return d->uninstallerName();
}

QString QInstaller::libraryName(const QString &baseName, const QString &version)
{
#ifdef Q_OS_WIN
    return baseName + QLatin1String(".dll");
#elif Q_OS_MAC
    return QString("lib%1.dylib").arg(baseName);
#else
    return QString("lib%1.so.%2").arg(baseName).arg(version);    
#endif
}

bool QInstaller::restartTempUninstaller(const QStringList &args)
{
    return d->restartTempUninstaller(args);
}


////////////////////////////////////////////////////////////////////
//
// QInstallerTask
//
////////////////////////////////////////////////////////////////////

QInstallerTask::QInstallerTask(QInstaller *parent)
  : m_installer(parent)
{}

QInstaller *QInstallerTask::installer() const
{
    return m_installer;
}


////////////////////////////////////////////////////////////////////
//
// QInstallerCopyFileTask
//
////////////////////////////////////////////////////////////////////

QInstaller::TaskCreator QInstallerCopyFileTask::creator() const
{
    return &createCopyFileTask;
}

void QInstallerCopyFileTask::writeToInstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendInt(out, m_permissions);
    QFile file(m_sourcePath);
    openForRead(file);
    appendFileData(out, &file);
}

static int createParentDirs(const QString &absFileName)
{
    QFileInfo fi(absFileName);
    if (fi.isDir())
        return 0;
    QString dirName = fi.path();
    int n = createParentDirs(dirName);
    QDir dir(dirName);
    dir.mkdir(fi.fileName());
    return n + 1;
}

void QInstallerCopyFileTask::readAndExecuteFromInstaller(QIODevice *in)
{
    m_targetPath = installer()->replaceVariables(retrieveString(in));
    m_permissions = retrieveInt(in);
    ifVerbose("EXECUTE COPY FILE, TARGET " << m_targetPath);

    QString path = QDir::cleanPath(QFileInfo(m_targetPath).absolutePath());
    m_parentDirCount = createParentDirs(path);
    QString msg = QInstaller::tr("Copying file %1").arg(m_targetPath);
    installer()->setInstallationProgressText(msg);

    QFile file(m_targetPath);
    bool res = file.open(QIODevice::WriteOnly);
    if (!res) {
        // try to make it writeable, and try again
        bool res1 = file.setPermissions(file.permissions()|QFile::WriteOwner);
        ifVerbose("MAKE WRITABLE: " << res1);
        res = file.open(QIODevice::WriteOnly);
    }
    if (!res) {
        // try to remove it, and try again
        bool res1 = file.remove();
        ifVerbose("REMOVING FILE: " << res1);
        res = file.open(QIODevice::WriteOnly);
    }

    if (!res) {
        QString msg = QInstaller::tr("The file %1 is not writeable.")
            .arg(m_targetPath);
        installer()->showWarning(msg);
        throw Error(msg);
    }
    retrieveFileData(&file, in);
    QFile::Permissions perms(m_permissions | QFile::WriteOwner);
    file.close();
    file.setPermissions(perms);
}

void QInstallerCopyFileTask::writeToUninstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendInt(out, m_parentDirCount);
}

void QInstallerCopyFileTask::readAndExecuteFromUninstaller(QIODevice *in)
{
    m_targetPath = retrieveString(in);
    m_parentDirCount = retrieveInt(in);
    undo();
}

void QInstallerCopyFileTask::undo()
{
    ifVerbose("UNLINKING FILE" << m_targetPath << m_parentDirCount);
    QString msg = QInstaller::tr("Removing %1").arg(m_targetPath);
    installer()->setInstallationProgressText(msg);

    QFileInfo fi(m_targetPath);
    QDir dir(fi.path());

    QFile file(m_targetPath);
    bool res = file.remove();
    if (!res) {
        // try to make it writeable, and try again
        bool res1 = file.setPermissions(file.permissions()|QFile::WriteOwner);
        ifVerbose("MAKE WRITABLE: " << res1);
        res = file.remove();
    }
    
    while (res && --m_parentDirCount >= 0) {
        QString dirName = dir.dirName();
        dir.cdUp();
        res = dir.rmdir(dirName);
        msg = QInstaller::tr("Removing file %1").arg(m_targetPath);
        installer()->setInstallationProgressText(msg);
        ifVerbose("REMOVING DIR " << dir.path() << dirName << res);
    }
}

void QInstallerCopyFileTask::dump(QDebug & os) const
{
    os << "c|" + sourcePath() + '|' + targetPath();
}


////////////////////////////////////////////////////////////////////
//
// QInstallerLinkFileTask
//
////////////////////////////////////////////////////////////////////

QInstaller::TaskCreator QInstallerLinkFileTask::creator() const
{
    return &createLinkFileTask;
}

void QInstallerLinkFileTask::writeToInstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendString(out, m_linkTargetPath);
    appendInt(out, m_permissions);
}

void QInstallerLinkFileTask::readAndExecuteFromInstaller(QIODevice *in)
{
    m_targetPath = installer()->replaceVariables(retrieveString(in));
    m_linkTargetPath = installer()->replaceVariables(retrieveString(in));
    m_permissions = retrieveInt(in);

    ifVerbose("LINK " << m_targetPath << " TARGET " << m_linkTargetPath);

    QFile file(m_linkTargetPath);
    if (file.link(m_targetPath))
        return;

    // ok. linking failed. try to remove targetPath and link again
    bool res1 = QFile::remove(m_targetPath);
    ifVerbose("TARGET EXITS, REMOVE: " << m_targetPath << res1);
    if (file.link(m_targetPath))
        return;

    // nothing helped.
    throw Error(QInstaller::tr("Cannot link file %1 to %2:\n")
            .arg(m_linkTargetPath).arg(m_targetPath));
}

void QInstallerLinkFileTask::writeToUninstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
}

void QInstallerLinkFileTask::readAndExecuteFromUninstaller(QIODevice *in)
{
    m_targetPath = retrieveString(in);
    ifVerbose("UNLINKING LINK" << m_targetPath);
    undo();
}

void QInstallerLinkFileTask::undo()
{
    QFile file(m_targetPath);
    file.remove();
}

void QInstallerLinkFileTask::dump(QDebug & os) const
{
     os << "l|" + targetPath() + '|' + linkTargetPath();
}

////////////////////////////////////////////////////////////////////
//
// QInstallerWriteSettingsTask
//
////////////////////////////////////////////////////////////////////

QInstaller::TaskCreator QInstallerWriteSettingsTask::creator() const
{
    return &createWriteSettingsTask;
}

void QInstallerWriteSettingsTask::writeToInstaller(QIODevice *out) const
{
    appendString(out, m_key);
    appendString(out, m_value);
}

void QInstallerWriteSettingsTask::readAndExecuteFromInstaller(QIODevice *in)
{
    m_key = installer()->replaceVariables(retrieveString(in));
    m_value = installer()->replaceVariables(retrieveString(in));
    QSettings settings;
    settings.setValue(m_key, m_value);
}

void QInstallerWriteSettingsTask::writeToUninstaller(QIODevice *out) const
{
    appendString(out, m_key);
    appendString(out, m_value);
}

void QInstallerWriteSettingsTask::readAndExecuteFromUninstaller(QIODevice *in)
{
    m_key = installer()->replaceVariables(retrieveString(in));
    m_value = installer()->replaceVariables(retrieveString(in));
    undo();
}

void QInstallerWriteSettingsTask::undo()
{
    QSettings settings;
    settings.setValue(m_key, QString());
}

void QInstallerWriteSettingsTask::dump(QDebug & os) const
{
    os << "s|" + key() + '|' + value();
}


////////////////////////////////////////////////////////////////////
//
// QInstallerPatchFileTask
//
////////////////////////////////////////////////////////////////////

QInstaller::TaskCreator QInstallerPatchFileTask::creator() const
{
    return &createPatchFileTask;
}

void QInstallerPatchFileTask::writeToInstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendByteArray(out, m_needle);
    appendByteArray(out, m_replacement);
}

void QInstallerPatchFileTask::readAndExecuteFromInstaller(QIODevice *in)
{
    m_targetPath = installer()->replaceVariables(retrieveString(in));
    m_needle = retrieveByteArray(in);
    m_replacement = installer()->replaceVariables(retrieveByteArray(in));
    ifVerbose("PATCHING" << m_replacement << m_needle << m_targetPath);

    QFile file;
    file.setFileName(m_targetPath);
    if (!file.open(QIODevice::ReadWrite))
        throw Error(QInstaller::tr("Cannot open file %1 for reading").arg(file.fileName()));

    uchar *data = file.map(0, file.size());
    if (!data)
        throw Error(QInstaller::tr("Cannot map file %1").arg(file.fileName()));
    QByteArray ba = QByteArray::fromRawData((const char *)data, file.size());
    int pos = ba.indexOf(m_needle);
    if (pos != -1) {
        for (int i = m_replacement.size(); --i >= 0; )
            data[pos + i] = m_replacement.at(i);
    }
    if (!file.unmap(data))
        throw Error(QInstaller::tr("Cannot unmap file %1").arg(file.fileName()));
    file.close();
}

void QInstallerPatchFileTask::dump(QDebug & os) const
{
     os << "p|" + targetPath() + '|' + needle() + '|' + replacement();
}


////////////////////////////////////////////////////////////////////
//
// QInstallerMenuShortcutTask
//
//
// Usage:
//
// static const struct
// {
//     const char *target;
//     const char *linkTarget;
// } menuShortcuts[] = {
//     {"Qt Creator by Nokia\\Run Qt Creator", "bin\\qtcreator.exe"},
//     {"Qt Creator by Nokia\\Readme", "readme.txt"},
//     {"Qt Creator by Nokia\\Uninstall", "uninstall.exe"}
// };
//
// for (int i = 0; i != sizeof(menuShortcuts) / sizeof(menuShortcuts[0]); ++i) {
//     QInstallerMenuShortcutTask *task = new QInstallerMenuShortcutTask(this);
//     task->setTargetPath(menuShortcuts[i].target);
//     task->setLinkTargetPath(QLatin1String("@TargetDir@\\") + menuShortcuts[i].linkTarget);
// }
//
////////////////////////////////////////////////////////////////////

QInstaller::TaskCreator QInstallerMenuShortcutTask::creator() const
{
    return &createMenuShortcutTask;
}

void QInstallerMenuShortcutTask::writeToInstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendString(out, m_linkTargetPath);
}

void QInstallerMenuShortcutTask::readAndExecuteFromInstaller(QIODevice *in)
{
    m_targetPath = installer()->replaceVariables(retrieveString(in));
    m_linkTargetPath = installer()->replaceVariables(retrieveString(in));

#ifdef Q_OS_WIN
    QString workingDir = installer()->value(QLatin1String("TargetDir"));
    bool res = false;
    HRESULT hres;
    IShellLink *psl;
    bool neededCoInit = false;

    ifVerbose("CREATE MENU SHORTCUT: " << m_targetPath << " TARGET " << m_linkTargetPath);

    if (installer()->value(QLatin1String("AllUsers")) == "true") {
        QSettings registry(QLatin1String("HKEY_LOCAL_MACHINE\\Software\\Microsoft\\Windows"
            "\\CurrentVersion\\Explorer\\Shell Folders"), QSettings::NativeFormat);
        m_startMenuPath = registry.value(QLatin1String("Common Programs"), QString()).toString();
    } else {
        QSettings registry(QLatin1String("HKEY_CURRENT_USER\\Software\\Microsoft\\Windows"
            "\\CurrentVersion\\Explorer\\Shell Folders"), QSettings::NativeFormat);
        m_startMenuPath = registry.value(QLatin1String("Programs"), QString()).toString();
    }
    if (m_startMenuPath.isEmpty()) {
        ifVerbose("CREATE MENU SHORTCUT: Cannot find start menu folder!");
        return;
    }

    if (!m_targetPath.isEmpty()) {
        if (!m_targetPath.endsWith(QLatin1String(".lnk")))
            m_targetPath.append(QLatin1String(".lnk"));
        m_targetPath = m_targetPath.replace('/', '\\');
        int i = m_targetPath.lastIndexOf('\\');
        if (i > -1) {
            QDir dir(m_startMenuPath);
            if (!dir.exists(m_targetPath.left(i)))
                dir.mkpath(m_targetPath.left(i));
        }

        if (m_linkTargetPath.isEmpty())
            return;

        QString trgt = m_linkTargetPath;
        if (trgt.startsWith('\"')) {
            trgt = trgt.left(trgt.indexOf('\"', 1) + 1);
        } else {
            trgt = trgt.left(trgt.indexOf(' '));
        }
        if (trgt.isEmpty())
            trgt = m_linkTargetPath;

        hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink, (void **)&psl);
        if (hres == CO_E_NOTINITIALIZED) {
            neededCoInit = true;
            CoInitialize(NULL);
            hres = CoCreateInstance(CLSID_ShellLink, NULL, CLSCTX_INPROC_SERVER, IID_IShellLink,
                (void **)&psl);
        }
        if (SUCCEEDED(hres)) {
            hres = psl->SetPath((wchar_t *)trgt.utf16());
            if (SUCCEEDED(hres)) {
                hres = psl->SetArguments((wchar_t *)m_linkTargetPath.mid(trgt.length()).utf16());
                if (SUCCEEDED(hres)) {
                    hres = psl->SetWorkingDirectory((wchar_t *)workingDir.utf16());
                    if (SUCCEEDED(hres)) {
                        IPersistFile *ppf;
                        hres = psl->QueryInterface(IID_IPersistFile, (void **)&ppf);
                        if (SUCCEEDED(hres)) {
                            hres = ppf->Save((TCHAR*)QString(m_startMenuPath
                                + QDir::separator() + m_targetPath).utf16(), TRUE);
                            if (SUCCEEDED(hres))
                                res = true;
                            ppf->Release();
                        }
                    }
                }
            }
            psl->Release();
        }
        if (neededCoInit)
            CoUninitialize();
    }
    if (!res) {
        QString msg = QInstaller::tr("Cannot create menu shortcut %1 to %2:\n")
            .arg(m_linkTargetPath).arg(m_targetPath);
        installer()->showWarning(msg);
        return;
    }
#endif
}

void QInstallerMenuShortcutTask::writeToUninstaller(QIODevice *out) const
{
    appendString(out, m_targetPath);
    appendString(out, m_startMenuPath);
}

void QInstallerMenuShortcutTask::readAndExecuteFromUninstaller(QIODevice *in)
{
    m_targetPath = retrieveString(in);
    m_startMenuPath = retrieveString(in);
    ifVerbose("REMOVE MENU SHORTCUT: " << m_targetPath);
    undo();
}

void QInstallerMenuShortcutTask::undo()
{
#ifdef Q_OS_WIN
    QFileInfo fi(m_startMenuPath + QDir::separator() + m_targetPath);
    QString path = fi.absoluteFilePath();
    if (fi.isFile()) {
        path = fi.absolutePath();
        QFile file(fi.absoluteFilePath());
        file.remove();
    }
    QDir dir(m_startMenuPath);
    dir.rmpath(path);
#endif
}

void QInstallerMenuShortcutTask::dump(QDebug & os) const
{
     os << "msc|" + targetPath() + '|' + linkTargetPath();
}

QT_END_NAMESPACE

#include "qinstaller.moc"
