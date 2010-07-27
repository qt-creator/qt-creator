#include "profilewrapper.h"

#include <prowriter.h>
#include <qt4projectmanager/profilereader.h>

#include <QtCore/QCryptographicHash>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace Qt4ProjectManager {
namespace Internal {

namespace {
    QString pathVar(const QString &var)
    {
        return var + QLatin1String(".path");
    }

    QString filesVar(const QString &var)
    {
        return var + QLatin1String(".files");
    }

    const QLatin1String InstallsVar("INSTALLS");
    const QLatin1String TargetVar("target");
}


ProFileWrapper::ProFileWrapper(const QString &proFileName,
    const QSharedPointer<ProFileOption> &proFileOption)
    : m_proFileName(proFileName), m_proDir(QFileInfo(m_proFileName).dir()),
      m_proFileOption(proFileOption)
{
    parseProFile(ParseFromFile);
}

ProFileWrapper::~ProFileWrapper() {}

void ProFileWrapper::reload()
{
    parseProFile(ParseFromFile);
}

ProFileWrapper::InstallsList ProFileWrapper::installs() const
{
    InstallsList list;

    const QStringList &elemList = varValues(InstallsVar);
    foreach (const QString &elem, elemList) {
        const QStringList &paths = varValues(pathVar(elem));
        if (paths.count() != 1) {
            qWarning("Error: Variable %s has %d values.",
                qPrintable(pathVar(elem)), paths.count());
            continue;
        }

        const QStringList &files = varValues(filesVar(elem));

        if (elem == TargetVar) {
            if (!list.targetPath.isEmpty()) {
                qWarning("Error: More than one target in INSTALLS list.");
                continue;
            }
            list.targetPath = paths.first();
        } else {
            if (files.isEmpty()) {
                qWarning("Error: Variable %s has no RHS.",
                    qPrintable(filesVar(elem)));
                continue;
            }
            list.normalElems << InstallsElem(elem, paths.first(), files);
        }
    }

    return list;
}

bool ProFileWrapper::addInstallsElem(const QString &path,
    const QString &absFilePath, const QString &var)
{
    QString varName = var;
    if (varName.isEmpty()) {
        QCryptographicHash elemHash(QCryptographicHash::Md5);
        elemHash.addData(absFilePath.toUtf8());
        varName = QString::fromAscii(elemHash.result().toHex());
    }

    // TODO: Use lower-level calls here to make operation atomic.
    if (varName != TargetVar && !addFile(filesVar(varName), absFilePath))
        return false;
    return addVarValue(pathVar(varName), path)
        && addVarValue(InstallsVar, varName);
}

bool ProFileWrapper::addInstallsTarget(const QString &path)
{
    return addInstallsElem(path, QString(), TargetVar);
}

bool ProFileWrapper::removeInstallsElem(const QString &path,
    const QString &file)
{
    const InstallsElem &elem = findInstallsElem(path, file);
    if (elem.varName.isEmpty())
        return false;

    // TODO: Use lower-level calls here to make operation atomic.
    if (elem.varName != TargetVar && !removeFile(filesVar(elem.varName), file))
        return false;
    if (elem.files.count() <= 1) {
        if (!removeVarValue(pathVar(elem.varName), path))
            return false;
        if (!removeVarValue(InstallsVar, elem.varName))
            return false;
    }
    return true;
}

bool ProFileWrapper::replaceInstallPath(const QString &oldPath,
    const QString &file, const QString &newPath)
{
    const InstallsElem &elem = findInstallsElem(oldPath, file);
    if (elem.varName.isEmpty())
        return false;

    // Simple case: Variable has only one file, so just replace the path.
    if (elem.varName == TargetVar || elem.files.count() == 1)
        return replaceVarValue(pathVar(elem.varName), oldPath, newPath);

    // Complicated case: Variable has other files, so remove our file from it
    // and introduce a new one.
    if (!removeInstallsElem(oldPath, file))
        return false;
    return addInstallsElem(newPath, file);
}

QStringList ProFileWrapper::varValues(const QString &var) const
{
    return m_proFileReader->values(var, m_proFile);
}

bool ProFileWrapper::addVarValue(const QString &var, const QString &value)
{
    if (varValues(var).contains(value))
        return true;

    if (!readProFileContents())
        return false;
    ProWriter::addVarValues(m_proFile, &m_proFileContents, m_proDir,
        QStringList(value), var);
    parseProFile(ParseFromLines);
    return writeProFileContents();
}

bool ProFileWrapper::addFile(const QString &var, const QString &absFilePath)
{
    if (!readProFileContents())
        return false;
    ProWriter::addFiles(m_proFile, &m_proFileContents, m_proDir,
        QStringList(absFilePath), var);
    parseProFile(ParseFromLines);
    return writeProFileContents();
}

bool ProFileWrapper::removeVarValue(const QString &var, const QString &value)
{
    if (!readProFileContents())
        return false;
    const bool success = ProWriter::removeVarValues(m_proFile,
        &m_proFileContents, m_proDir, QStringList(value), QStringList(var))
        .isEmpty();
    if (success) {
        parseProFile(ParseFromLines);
        return writeProFileContents();
    } else {
        parseProFile(ParseFromFile);
        return false;
    }
}

bool ProFileWrapper::removeFile(const QString &var, const QString &absFilePath)
{
    if (!readProFileContents())
        return false;
    const bool success = ProWriter::removeFiles(m_proFile, &m_proFileContents,
        m_proDir, QStringList(absFilePath), QStringList(var)).isEmpty();
    if (success) {
        parseProFile(ParseFromLines);
        return writeProFileContents();
    } else {
        parseProFile(ParseFromFile);
        return false;
    }
}

bool ProFileWrapper::replaceVarValue(const QString &var,
    const QString &oldValue, const QString &newValue)
{
    if (!readProFileContents())
        return false;
    const bool success = ProWriter::removeVarValues(m_proFile,
        &m_proFileContents, m_proDir, QStringList(oldValue), QStringList(var))
        .isEmpty();
    if (!success) {
        parseProFile(ParseFromFile);
        return false;
    }
    ProWriter::addVarValues(m_proFile, &m_proFileContents, m_proDir,
        QStringList(newValue), var);
    parseProFile(ParseFromLines);
    return writeProFileContents();
}

QString ProFileWrapper::absFilePath(const QString &relFilePath) const
{
    // I'd rather use QDir::cleanPath(), but that doesn't work well
    // enough for redundant ".." dirs.
    QFileInfo fi(relFilePath);
    return fi.isAbsolute() ? fi.canonicalFilePath()
        : QFileInfo(m_proFile->directoryName() + '/' + relFilePath)
        .canonicalFilePath();
}

void ProFileWrapper::parseProFile(ParseType type) const
{
    m_proFileReader.reset(new ProFileReader(m_proFileOption.data()));
    m_proFileReader->setCumulative(false);
    // TODO: Set output dir to build dir?
    if (type == ParseFromLines) {
        m_proFile = m_proFileReader->parsedProFile(m_proFileName, false,
            m_proFileContents.join("\n"));
    } else {
        m_proFileContents.clear();
        m_proFile = m_proFileReader->parsedProFile(m_proFileName);
    }

    if (!m_proFile) {
        qWarning("Fatal: Could not parse .pro file '%s'.",
            qPrintable(m_proFileName));
        return;
    }

    m_proFileReader->accept(m_proFile);
    m_proFile->deref();
}

bool ProFileWrapper::writeProFileContents()
{
    QFile proFileOnDisk(m_proFileName);
    if (!proFileOnDisk.open(QIODevice::WriteOnly)) {
        parseProFile(ParseFromFile);
        return false;
    }

    // TODO: Disconnect and reconnect FS watcher here.
    proFileOnDisk.write(m_proFileContents.join("\n").toLatin1());
    proFileOnDisk.close();
    if (proFileOnDisk.error() != QFile::NoError) {
        parseProFile(ParseFromFile);
        return false;
    }
    return true;
}

bool ProFileWrapper::readProFileContents()
{
    if (!m_proFileContents.isEmpty())
        return true;

    QFile proFileOnDisk(m_proFileName);
    if (!proFileOnDisk.open(QIODevice::ReadOnly))
        return false;
    const QString proFileContents
        = QString::fromLatin1(proFileOnDisk.readAll());
    if (proFileOnDisk.error() != QFile::NoError)
        return false;
    m_proFileContents = proFileContents.split('\n');
    return true;
}

ProFileWrapper::InstallsElem ProFileWrapper::findInstallsElem(const QString &path,
    const QString &file) const
{
    const QStringList &elems = varValues(InstallsVar);
    foreach (const QString &elem, elems) {
        const QStringList &elemPaths = varValues(pathVar(elem));
        if (elemPaths.count() != 1 || elemPaths.first() != path)
            continue;
        if (elem == TargetVar)
            return InstallsElem(elem, path, QStringList());
        const QStringList &elemFiles = varValues(filesVar(elem));
        foreach (const QString &elemFile, elemFiles) {
            if (absFilePath(elemFile) == file)
                return InstallsElem(elem, path, elemFiles);
        }
    }
    return InstallsElem(QString(), QString(), QStringList());
}

} // namespace Internal
} // namespace Qt4ProjectManager
