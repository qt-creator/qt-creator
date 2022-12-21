// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "abstractsettings.h"

#include "beautifierconstants.h"
#include "beautifierplugin.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/genericconstants.h>
#include <utils/mimeutils.h>
#include <utils/qtcprocess.h>

#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QVersionNumber>
#include <QXmlStreamReader>

using namespace Utils;

namespace Beautifier {
namespace Internal {

namespace {
const char COMMAND[]        = "command";
const char SUPPORTED_MIME[] = "supportedMime";
}

class VersionUpdater
{
public:
    VersionUpdater()
    {
        QObject::connect(&m_process, &QtcProcess::done, &m_process, [this] {
            if (m_process.result() != ProcessResult::FinishedWithSuccess)
                return;

            m_versionNumber = parseVersion(m_process.cleanedStdOut());
            if (m_versionNumber.isNull())
                m_versionNumber = parseVersion(m_process.cleanedStdErr());
        });
    }

    void setVersionRegExp(const QRegularExpression &versionRegExp)
    {
        m_versionRegExp = versionRegExp;
    }

    void update(const FilePath &executable)
    {
        m_versionNumber = {};
        if (m_versionRegExp.pattern().isEmpty())
            return;
        m_process.close();
        m_process.setCommand({executable, {"--version"}});
        m_process.start();
    }

    QVersionNumber version() const
    {
        if (m_process.state() != QProcess::NotRunning)
            m_process.waitForFinished(-1);
        return m_versionNumber;
    }

private:
    QVersionNumber parseVersion(const QString &text) const
    {
        const QRegularExpressionMatch match = m_versionRegExp.match(text);
        if (!match.hasMatch())
            return {};

        return {match.captured(1).toInt(), match.captured(2).toInt()};
    }

    QRegularExpression m_versionRegExp;
    mutable QtcProcess m_process;
    QVersionNumber m_versionNumber;
};

AbstractSettings::AbstractSettings(const QString &name, const QString &ending)
    : m_ending(ending)
    , m_styleDir(Core::ICore::userResourcePath(Beautifier::Constants::SETTINGS_DIRNAME)
                     .pathAppended(name)
                     .toString())
    , m_name(name)
    , m_versionUpdater(new VersionUpdater)
{
}

AbstractSettings::~AbstractSettings() = default;

QStringList AbstractSettings::completerWords()
{
    return QStringList();
}

QStringList AbstractSettings::styles() const
{
    QStringList list = m_styles.keys();
    list.sort(Qt::CaseInsensitive);
    return list;
}

QString AbstractSettings::style(const QString &key) const
{
    return m_styles.value(key);
}

bool AbstractSettings::styleExists(const QString &key) const
{
    return m_styles.contains(key);
}

bool AbstractSettings::styleIsReadOnly(const QString &key)
{
    const QFileInfo fi(m_styleDir.absoluteFilePath(key + m_ending));
    if (!fi.exists()) {
        // newly added style which was not saved yet., thus it is not read only.
        //TODO In a later version when we have predefined styles in Core::ICore::resourcePath()
        //     we need to check if it is a read only global config file...
        return false;
    }

    return !fi.isWritable();
}

void AbstractSettings::setStyle(const QString &key, const QString &value)
{
    m_styles.insert(key, value);
    m_changedStyles.insert(key);
}

void AbstractSettings::removeStyle(const QString &key)
{
    m_styles.remove(key);
    m_stylesToRemove << key;
}

void AbstractSettings::replaceStyle(const QString &oldKey, const QString &newKey,
                                    const QString &value)
{
    // Set value regardles if keys are equal
    m_styles.insert(newKey, value);

    if (oldKey != newKey)
        removeStyle(oldKey);

    m_changedStyles.insert(newKey);
}

QString AbstractSettings::styleFileName(const QString &key) const
{
    return m_styleDir.absoluteFilePath(key + m_ending);
}

FilePath AbstractSettings::command() const
{
    return FilePath::fromString(m_command);
}

void AbstractSettings::setCommand(const QString &cmd)
{
    if (cmd == m_command)
        return;

    m_command = cmd;
    m_versionUpdater->update(command());
}

QVersionNumber AbstractSettings::version() const
{
    return m_versionUpdater->version();
}

void AbstractSettings::setVersionRegExp(const QRegularExpression &versionRegExp)
{
    m_versionUpdater->setVersionRegExp(versionRegExp);
}

QString AbstractSettings::supportedMimeTypesAsString() const
{
    return m_supportedMimeTypes.join("; ");
}

void AbstractSettings::setSupportedMimeTypes(const QString &mimes)
{
    const QStringList stringTypes = mimes.split(';');
    QStringList types;
    for (const QString &type : stringTypes) {
        const MimeType mime = mimeTypeForName(type.trimmed());
        if (!mime.isValid())
            continue;
        const QString canonicalName = mime.name();
        if (!types.contains(canonicalName))
            types << canonicalName;
    }

    if (m_supportedMimeTypes != types) {
        m_supportedMimeTypes = types;
        emit supportedMimeTypesChanged();
    }
}

bool AbstractSettings::isApplicable(const Core::IDocument *document) const
{
    if (!document)
        return false;

    if (m_supportedMimeTypes.isEmpty())
        return true;

    const MimeType documentMimeType = mimeTypeForName(document->mimeType());
    return anyOf(m_supportedMimeTypes, [&documentMimeType](const QString &mime) {
        return documentMimeType.inherits(mime);
    });
}

QStringList AbstractSettings::options()
{
    if (m_options.isEmpty())
        readDocumentation();

    return m_options.keys();
}

QString AbstractSettings::documentation(const QString &option) const
{
    const int index = m_options.value(option, -1);
    if (index != -1)
        return m_docu.at(index);
    else
        return QString();
}

void AbstractSettings::save()
{
    // Save settings, except styles
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP);
    s->beginGroup(m_name);
    QMap<QString, QVariant>::const_iterator iSettings = m_settings.constBegin();
    while (iSettings != m_settings.constEnd()) {
        s->setValue(iSettings.key(), iSettings.value());
        ++iSettings;
    }
    s->setValue(COMMAND, m_command);
    s->setValue(SUPPORTED_MIME, supportedMimeTypesAsString());
    s->endGroup();
    s->endGroup();

    // Save styles
    if (m_stylesToRemove.isEmpty() && m_styles.isEmpty())
        return;

    // remove old files and possible subfolder
    for (const QString &key : std::as_const(m_stylesToRemove)) {
        const QFileInfo fi(styleFileName(key));
        QFile::remove(fi.absoluteFilePath());
        if (fi.absoluteDir() != m_styleDir)
            m_styleDir.rmdir(fi.absolutePath());
    }
    m_stylesToRemove.clear();

    QMap<QString, QString>::const_iterator iStyles = m_styles.constBegin();
    while (iStyles != m_styles.constEnd()) {
        // Only save changed styles.
        if (!m_changedStyles.contains(iStyles.key())) {
            ++iStyles;
            continue;
        }

        const QFileInfo fi(styleFileName(iStyles.key()));
        if (!(m_styleDir.mkpath(fi.absolutePath()))) {
            BeautifierPlugin::showError(tr("Cannot save styles. %1 does not exist.")
                                        .arg(fi.absolutePath()));
            continue;
        }

        FileSaver saver(FilePath::fromUserInput(fi.absoluteFilePath()));
        if (saver.hasError()) {
            BeautifierPlugin::showError(tr("Cannot open file \"%1\": %2.")
                                        .arg(saver.filePath().toUserOutput())
                                        .arg(saver.errorString()));
        } else {
            saver.write(iStyles.value().toLocal8Bit());
            if (!saver.finalize()) {
                BeautifierPlugin::showError(tr("Cannot save file \"%1\": %2.")
                                            .arg(saver.filePath().toUserOutput())
                                            .arg(saver.errorString()));
            }
        }
        ++iStyles;
    }

    m_changedStyles.clear();
}

void AbstractSettings::createDocumentationFile() const
{
    // Could be reimplemented to create a documentation file.
}

void AbstractSettings::read()
{
    // Set default values
    setSupportedMimeTypes("text/x-c++src;text/x-c++hdr;text/x-csrc;text/x-chdr;text/x-objcsrc;"
                          "text/x-objc++src");

    // Read settings, except styles
    QSettings *s = Core::ICore::settings();
    s->beginGroup(Utils::Constants::BEAUTIFIER_SETTINGS_GROUP);
    s->beginGroup(m_name);
    const QStringList keys = s->allKeys();
    for (const QString &key : keys) {
        if (key == COMMAND)
            setCommand(s->value(key).toString());
        else if (key == SUPPORTED_MIME)
            setSupportedMimeTypes(s->value(key).toString());
        else if (m_settings.contains(key))
            m_settings[key] = s->value(key);
        else
            s->remove(key);
    }
    s->endGroup();
    s->endGroup();

    m_styles.clear();
    m_changedStyles.clear();
    m_stylesToRemove.clear();
    readStyles();
}

void AbstractSettings::readDocumentation()
{
    const QString filename = documentationFilePath();
    if (filename.isEmpty()) {
        BeautifierPlugin::showError(tr("No documentation file specified."));
        return;
    }

    QFile file(filename);
    if (!file.exists())
        createDocumentationFile();

    if (!file.open(QIODevice::ReadOnly)) {
        BeautifierPlugin::showError(tr("Cannot open documentation file \"%1\".").arg(filename));
        return;
    }

    QXmlStreamReader xml(&file);
    if (!xml.readNextStartElement())
        return;
    if (xml.name() != QLatin1String(Constants::DOCUMENTATION_XMLROOT)) {
        BeautifierPlugin::showError(tr("The file \"%1\" is not a valid documentation file.")
                                    .arg(filename));
        return;
    }

    // We do not use QHash<QString, QString> since e.g. in Artistic Style there are many different
    // keys with the same (long) documentation text. By splitting the keys from the documentation
    // text we save a huge amount of memory.
    m_options.clear();
    m_docu.clear();
    QStringList keys;
    while (!(xml.atEnd() || xml.hasError())) {
        if (xml.readNext() == QXmlStreamReader::StartElement) {
            QStringView name = xml.name();
            if (name == QLatin1String(Constants::DOCUMENTATION_XMLENTRY)) {
                keys.clear();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLKEY)) {
                if (xml.readNext() == QXmlStreamReader::Characters)
                    keys << xml.text().toString();
            } else if (name == QLatin1String(Constants::DOCUMENTATION_XMLDOC)) {
                if (xml.readNext() == QXmlStreamReader::Characters) {
                    m_docu << xml.text().toString();
                    const int index = m_docu.size() - 1;
                    for (const QString &key : std::as_const(keys))
                        m_options.insert(key, index);
                }
            }
        }
    }

    if (xml.hasError()) {
        BeautifierPlugin::showError(tr("Cannot read documentation file \"%1\": %2.")
                                    .arg(filename).arg(xml.errorString()));
    }
}

void AbstractSettings::readStyles()
{
    if (!m_styleDir.exists())
        return;

    const QStringList files
            = m_styleDir.entryList({'*' + m_ending},
                                   QDir::Files | QDir::Readable | QDir::NoDotAndDotDot);
    for (const QString &filename : files) {
        // do not allow empty file names
        if (filename == m_ending)
            continue;

        QFile file(m_styleDir.absoluteFilePath(filename));
        if (file.open(QIODevice::ReadOnly)) {
            m_styles.insert(filename.left(filename.length() - m_ending.length()),
                            QString::fromLocal8Bit(file.readAll()));
        }
    }
}

} // namespace Internal
} // namespace Beautifier
