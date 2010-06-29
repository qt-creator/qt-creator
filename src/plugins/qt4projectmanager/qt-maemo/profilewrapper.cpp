#include "profilewrapper.h"

#include <prowriter.h>
#include <qt4projectmanager/profilereader.h>

#include <QtCore/QFile>
#include <QtCore/QFileInfo>

namespace Qt4ProjectManager {
namespace Internal {

ProFileWrapper::ProFileWrapper(const QString &proFileName)
    : m_proFileName(proFileName), m_proDir(QFileInfo(m_proFileName).dir()),
      m_proFileOption(new ProFileOption)
{
    parseProFile(ParseFromFile);
}

ProFileWrapper::~ProFileWrapper() {}

QStringList ProFileWrapper::varValues(const QString &var) const
{
    return m_proFileReader->values(var, m_proFile);
}

bool ProFileWrapper::addVarValue(const QString &var, const QString &value)
{
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
    return QFileInfo(m_proFile->directoryName() + '/' + relFilePath)
        .canonicalFilePath();
}

void ProFileWrapper::parseProFile(ParseType type) const
{
    m_proFileReader.reset(new ProFileReader(m_proFileOption.data()));
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

} // namespace Internal
} // namespace Qt4ProjectManager
