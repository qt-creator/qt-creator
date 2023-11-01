// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sdkpersistentsettings.h"

#include "operation.h" // for cleanPath()

#include <QCoreApplication>
#include <QDataStream>
#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QRect>
#include <QRegularExpression>
#include <QStack>
#include <QTextStream>
#include <QXmlStreamAttributes>
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTemporaryFile>

#ifdef Q_OS_WIN
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <sys/stat.h>
#endif


#define QTC_ASSERT_STRINGIFY_HELPER(x) #x
#define QTC_ASSERT_STRINGIFY(x) QTC_ASSERT_STRINGIFY_HELPER(x)
#define QTC_ASSERT_STRING(cond) writeAssertLocation(\
    "\"" cond"\" in " __FILE__ ":" QTC_ASSERT_STRINGIFY(__LINE__))

// The 'do {...} while (0)' idiom is not used for the main block here to be
// able to use 'break' and 'continue' as 'actions'.

#define QTC_ASSERT(cond, action) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); action; } do {} while (0)
#define QTC_CHECK(cond) if (Q_LIKELY(cond)) {} else { QTC_ASSERT_STRING(#cond); } do {} while (0)
#define QTC_GUARD(cond) ((Q_LIKELY(cond)) ? true : (QTC_ASSERT_STRING(#cond), false))

void writeAssertLocation(const char *msg)
{
    const QByteArray time = QTime::currentTime().toString(Qt::ISODateWithMs).toLatin1();
    static bool goBoom = qEnvironmentVariableIsSet("QTC_FATAL_ASSERTS");
    if (goBoom)
        qFatal("SOFT ASSERT [%s] made fatal: %s", time.data(), msg);
    else
        qDebug("SOFT ASSERT [%s]: %s", time.data(), msg);
}

static QFile::Permissions m_umask;

class SdkSaveFile : public QFile
{
public:
    explicit SdkSaveFile(const QString &filePath) : m_finalFilePath(filePath) {}
    ~SdkSaveFile() override;

    bool open(OpenMode flags = QIODevice::WriteOnly) override;

    void rollback();
    bool commit();

    static void initializeUmask();

private:
    const QString m_finalFilePath;
    std::unique_ptr<QTemporaryFile> m_tempFile;
    bool m_finalized = true;
};

SdkSaveFile::~SdkSaveFile()
{
    if (!m_finalized)
        rollback();
}

bool SdkSaveFile::open(OpenMode flags)
{
    if (m_finalFilePath.isEmpty()) {
        qWarning("Save file path empty");
        return false;
    }

    QFile ofi(m_finalFilePath);
    // Check whether the existing file is writable
    if (ofi.exists() && !ofi.open(QIODevice::ReadWrite)) {
        setErrorString(ofi.errorString());
        return false;
    }

    m_tempFile = std::make_unique<QTemporaryFile>(m_finalFilePath);
    m_tempFile->setAutoRemove(false);
    if (!m_tempFile->open())
        return false;
    setFileName(m_tempFile->fileName());

    if (!QFile::open(flags))
        return false;

    m_finalized = false; // needs clean up in the end
    if (ofi.exists()) {
        setPermissions(ofi.permissions()); // Ignore errors
    } else {
        Permissions permAll = QFile::ReadOwner
                | QFile::ReadGroup
                | QFile::ReadOther
                | QFile::WriteOwner
                | QFile::WriteGroup
                | QFile::WriteOther;

        // set permissions with respect to the current umask
        setPermissions(permAll & ~m_umask);
    }

    return true;
}

void SdkSaveFile::rollback()
{
    close();
    if (m_tempFile)
        m_tempFile->remove();
    m_finalized = true;
}

static QString resolveSymlinks(QString current)
{
    int links = 16;
    while (links--) {
        const QFileInfo info(current);
        if (!info.isSymLink())
            return current;
        current = info.symLinkTarget();
    }
    return current;
}

bool SdkSaveFile::commit()
{
    QTC_ASSERT(!m_finalized && m_tempFile, return false;);
    m_finalized = true;

    if (!flush()) {
        close();
        m_tempFile->remove();
        return false;
    }
#ifdef Q_OS_WIN
    FlushFileBuffers(reinterpret_cast<HANDLE>(_get_osfhandle(handle())));
#elif _POSIX_SYNCHRONIZED_IO > 0
    fdatasync(handle());
#else
    fsync(handle());
#endif
    close();
    m_tempFile->close();
    if (error() != NoError) {
        m_tempFile->remove();
        return false;
    }

    QString finalFileName = resolveSymlinks(m_finalFilePath);

#ifdef Q_OS_WIN
    // Release the file lock
    m_tempFile.reset();
    bool result = ReplaceFile(finalFileName.toStdWString().data(),
                              fileName().toStdWString().data(),
                              nullptr, REPLACEFILE_IGNORE_MERGE_ERRORS, nullptr, nullptr);
    if (!result) {
        DWORD replaceErrorCode = GetLastError();
        QString errorStr;
        if (!QFileInfo::exists(finalFileName)) {
            // Replace failed because finalFileName does not exist, try rename.
            if (!(result = rename(finalFileName)))
                errorStr = errorString();
        } else {
            if (replaceErrorCode == ERROR_UNABLE_TO_REMOVE_REPLACED) {
                // If we do not get the rights to remove the original final file we still might try
                // to replace the file contents
                result = MoveFileEx(fileName().toStdWString().data(),
                                    finalFileName.toStdWString().data(),
                                    MOVEFILE_COPY_ALLOWED
                                    | MOVEFILE_REPLACE_EXISTING
                                    | MOVEFILE_WRITE_THROUGH);
                if (!result)
                    replaceErrorCode = GetLastError();
            }
            if (!result) {
                wchar_t messageBuffer[256];
                FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               nullptr, replaceErrorCode,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               messageBuffer, sizeof(messageBuffer), nullptr);
                errorStr = QString::fromWCharArray(messageBuffer);
            }
        }
        if (!result) {
            remove();
            setErrorString(errorStr);
        }
    }

    return result;
#else
    const QString backupName = finalFileName + '~';

    // Back up current file.
    // If it's opened by another application, the lock follows the move.
    if (QFileInfo::exists(finalFileName)) {
        // Kill old backup. Might be useful if creator crashed before removing backup.
        QFile::remove(backupName);
        QFile finalFile(finalFileName);
        if (!finalFile.rename(backupName)) {
            m_tempFile->remove();
            setErrorString(finalFile.errorString());
            return false;
        }
    }

    bool result = true;
    if (!m_tempFile->rename(finalFileName)) {
        // The case when someone else was able to create finalFileName after we've renamed it.
        // Higher level call may try to save this file again but here we do nothing and
        // return false while keeping the error string from last rename call.
        const QString &renameError = m_tempFile->errorString();
        m_tempFile->remove();
        setErrorString(renameError);
        QFile::rename(backupName, finalFileName); // rollback to backup if possible ...
        return false; // ... or keep the backup copy at least
    }

    QFile::remove(backupName);

    return result;
#endif
}

void SdkSaveFile::initializeUmask()
{
#ifdef Q_OS_WIN
    m_umask = QFile::WriteGroup | QFile::WriteOther;
#else
    // Get the current process' file creation mask (umask)
    // umask() is not thread safe so this has to be done by single threaded
    // application initialization
    mode_t mask = umask(0); // get current umask
    umask(mask); // set it back

    m_umask = ((mask & S_IRUSR) ? QFile::ReadOwner  : QFlags<QFile::Permission>())
            | ((mask & S_IWUSR) ? QFile::WriteOwner : QFlags<QFile::Permission>())
            | ((mask & S_IXUSR) ? QFile::ExeOwner   : QFlags<QFile::Permission>())
            | ((mask & S_IRGRP) ? QFile::ReadGroup  : QFlags<QFile::Permission>())
            | ((mask & S_IWGRP) ? QFile::WriteGroup : QFlags<QFile::Permission>())
            | ((mask & S_IXGRP) ? QFile::ExeGroup   : QFlags<QFile::Permission>())
            | ((mask & S_IROTH) ? QFile::ReadOther  : QFlags<QFile::Permission>())
            | ((mask & S_IWOTH) ? QFile::WriteOther : QFlags<QFile::Permission>())
            | ((mask & S_IXOTH) ? QFile::ExeOther   : QFlags<QFile::Permission>());
#endif
}

class SdkFileSaverBase
{
public:
    SdkFileSaverBase() = default;
    virtual ~SdkFileSaverBase() = default;

    QString filePath() const { return m_filePath; }
    bool hasError() const { return m_hasError; }
    QString errorString() const { return m_errorString; }
    virtual bool finalize();
    bool finalize(QString *errStr);

    bool write(const char *data, int len);
    bool write(const QByteArray &bytes);
    bool setResult(QTextStream *stream);
    bool setResult(QDataStream *stream);
    bool setResult(QXmlStreamWriter *stream);
    bool setResult(bool ok);

    QFile *file() { return m_file.get(); }

protected:
    std::unique_ptr<QFile> m_file;
    QString m_filePath;
    QString m_errorString;
    bool m_hasError = false;
};

bool SdkFileSaverBase::finalize()
{
    m_file->close();
    setResult(m_file->error() == QFile::NoError);
    m_file.reset();
    return !m_hasError;
}

bool SdkFileSaverBase::finalize(QString *errStr)
{
    if (finalize())
        return true;
    if (errStr)
        *errStr = errorString();
    return false;
}

bool SdkFileSaverBase::write(const char *data, int len)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(data, len) == len);
}

bool SdkFileSaverBase::write(const QByteArray &bytes)
{
    if (m_hasError)
        return false;
    return setResult(m_file->write(bytes) == bytes.size());
}

bool SdkFileSaverBase::setResult(bool ok)
{
    if (!ok && !m_hasError) {
        if (!m_file->errorString().isEmpty()) {
            m_errorString = QString("Cannot write file %1: %2")
                                .arg(m_filePath, m_file->errorString());
        } else {
            m_errorString = QString("Cannot write file %1. Disk full?")
                                .arg(m_filePath);
        }
        m_hasError = true;
    }
    return ok;
}

bool SdkFileSaverBase::setResult(QTextStream *stream)
{
    stream->flush();
    return setResult(stream->status() == QTextStream::Ok);
}

bool SdkFileSaverBase::setResult(QDataStream *stream)
{
    return setResult(stream->status() == QDataStream::Ok);
}

bool SdkFileSaverBase::setResult(QXmlStreamWriter *stream)
{
    return setResult(!stream->hasError());
}

// SdkFileSaver

class SdkFileSaver : public SdkFileSaverBase
{
public:
    // QIODevice::WriteOnly is implicit
    explicit SdkFileSaver(const QString &filePath, QIODevice::OpenMode mode = QIODevice::NotOpen);

    bool finalize() override;

private:
    bool m_isSafe = false;
};

SdkFileSaver::SdkFileSaver(const QString &filePath, QIODevice::OpenMode mode)
{
    m_filePath = filePath;
    // Workaround an assert in Qt -- and provide a useful error message, too:
#ifdef Q_OS_WIN
        // Taken from: https://msdn.microsoft.com/en-us/library/windows/desktop/aa365247(v=vs.85).aspx
        static const QStringList reservedNames
                = {"CON", "PRN", "AUX", "NUL",
                   "COM1", "COM2", "COM3", "COM4", "COM5", "COM6", "COM7", "COM8", "COM9",
                   "LPT1", "LPT2", "LPT3", "LPT4", "LPT5", "LPT6", "LPT7", "LPT8", "LPT9"};
        const QString fn = QFileInfo(filePath).baseName().toUpper();
        if (reservedNames.contains(fn)) {
            m_errorString = QString("%1: Is a reserved filename on Windows. Cannot save.").arg(filePath);
            m_hasError = true;
            return;
        }
#endif

    if (mode & (QIODevice::ReadOnly | QIODevice::Append)) {
        m_file.reset(new QFile{filePath});
        m_isSafe = false;
    } else {
        m_file.reset(new SdkSaveFile(filePath));
        m_isSafe = true;
    }
    if (!m_file->open(QIODevice::WriteOnly | mode)) {
        QString err = QFileInfo::exists(filePath) ?
                QString("Cannot overwrite file %1: %2") : QString("Cannot create file %1: %2");
        m_errorString = err.arg(filePath, m_file->errorString());
        m_hasError = true;
    }
}

bool SdkFileSaver::finalize()
{
    if (!m_isSafe)
        return SdkFileSaverBase::finalize();

    auto sf = static_cast<SdkSaveFile *>(m_file.get());
    if (m_hasError) {
        if (sf->isOpen())
            sf->rollback();
    } else {
        setResult(sf->commit());
    }
    m_file.reset();
    return !m_hasError;
}


// Read and write rectangle in X11 resource syntax "12x12+4+3"
static QString rectangleToString(const QRect &r)
{
    QString result;
    QTextStream str(&result);
    str << r.width() << 'x' << r.height();
    if (r.x() >= 0)
        str << '+';
    str << r.x();
    if (r.y() >= 0)
        str << '+';
    str << r.y();
    return result;
}

static QRect stringToRectangle(const QString &v)
{
    static QRegularExpression pattern("^(\\d+)x(\\d+)([-+]\\d+)([-+]\\d+)$");
    Q_ASSERT(pattern.isValid());
    const QRegularExpressionMatch match = pattern.match(v);
    return match.hasMatch() ?
        QRect(QPoint(match.captured(3).toInt(), match.captured(4).toInt()),
              QSize(match.captured(1).toInt(), match.captured(2).toInt())) :
        QRect();
}

/*!
    \class SdkPersistentSettingsReader

    \note This is aQString based fork of Utils::PersistentSettigsReader

    \brief The SdkPersistentSettingsReader class reads a QVariantMap of arbitrary,
    nested data structures from an XML file.

    Handles all string-serializable simple types and QVariantList and QVariantMap. Example:
    \code
<qtcreator>
    <data>
        <variable>ProjectExplorer.Project.ActiveTarget</variable>
        <value type="int">0</value>
    </data>
    <data>
        <variable>ProjectExplorer.Project.EditorSettings</variable>
        <valuemap type="QVariantMap">
            <value type="bool" key="EditorConfiguration.AutoIndent">true</value>
        </valuemap>
    </data>
    \endcode

    When parsing the structure, a parse stack of ParseValueStackEntry is used for each
    <data> element. ParseValueStackEntry is a variant/union of:
    \list
    \li simple value
    \li map
    \li list
    \endlist

    You can register string-serialize functions for custom types by registering them in the Qt Meta
    type system. Example:
    \code
      QMetaType::registerConverter(&MyCustomType::toString);
      QMetaType::registerConverter<QString, MyCustomType>(&myCustomTypeFromString);
    \endcode

    When entering a value element ( \c <value> / \c <valuelist> , \c <valuemap> ), entry is pushed
    accordingly. When leaving the element, the QVariant-value of the entry is taken off the stack
    and added to the stack entry below (added to list or inserted into map). The first element
    of the stack is the value of the <data> element.

    \sa SdkPersistentSettingsWriter
*/

struct Context // Basic context containing element name string constants.
{
    Context() {}
    const QString qtCreatorElement = QString("qtcreator");
    const QString dataElement = QString("data");
    const QString variableElement = QString("variable");
    const QString typeAttribute = QString("type");
    const QString valueElement = QString("value");
    const QString valueListElement = QString("valuelist");
    const QString valueMapElement = QString("valuemap");
    const QString keyAttribute = QString("key");
};

struct ParseValueStackEntry
{
    explicit ParseValueStackEntry(QVariant::Type t = QVariant::Invalid, const QString &k = QString()) : type(t), key(k) {}
    explicit ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k);

    QVariant value() const;
    void addChild(const QString &key, const QVariant &v);

    QVariant::Type type;
    QString key;
    QVariant simpleValue;
    QVariantList listValue;
    QVariantMap mapValue;
};

ParseValueStackEntry::ParseValueStackEntry(const QVariant &aSimpleValue, const QString &k) :
    type(aSimpleValue.type()), key(k), simpleValue(aSimpleValue)
{
    QTC_ASSERT(simpleValue.isValid(), return);
}

QVariant ParseValueStackEntry::value() const
{
    switch (type) {
    case QVariant::Invalid:
        return QVariant();
    case QVariant::Map:
        return QVariant(mapValue);
    case QVariant::List:
        return QVariant(listValue);
    default:
        break;
    }
    return simpleValue;
}

void ParseValueStackEntry::addChild(const QString &key, const QVariant &v)
{
    switch (type) {
    case QVariant::Map:
        mapValue.insert(key, v);
        break;
    case QVariant::List:
        listValue.push_back(v);
        break;
    default:
        qWarning() << "ParseValueStackEntry::Internal error adding " << key << v << " to "
                 << QVariant::typeToName(type) << value();
        break;
    }
}

class ParseContext : public Context
{
public:
    QVariantMap parse(const QString &file);

private:
    enum Element { QtCreatorElement, DataElement, VariableElement,
                   SimpleValueElement, ListValueElement, MapValueElement, UnknownElement };

    Element element(const QStringView &r) const;
    static inline bool isValueElement(Element e)
        { return e == SimpleValueElement || e == ListValueElement || e == MapValueElement; }
    QVariant readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const;

    bool handleStartElement(QXmlStreamReader &r);
    bool handleEndElement(const QStringView &name);

    static QString formatWarning(const QXmlStreamReader &r, const QString &message);

    QStack<ParseValueStackEntry> m_valueStack;
    QVariantMap m_result;
    QString m_currentVariableName;
};

static QByteArray fileContents(const QString &path)
{
    QFile f(path);
    if (!f.exists())
        return {};

    if (!f.open(QFile::ReadOnly))
        return {};

    return f.readAll();
}

QVariantMap ParseContext::parse(const QString &file)
{
    QXmlStreamReader r(fileContents(file));

    m_result.clear();
    m_currentVariableName.clear();

    while (!r.atEnd()) {
        switch (r.readNext()) {
        case QXmlStreamReader::StartElement:
            if (handleStartElement(r))
                return m_result;
            break;
        case QXmlStreamReader::EndElement:
            if (handleEndElement(r.name()))
                return m_result;
            break;
        case QXmlStreamReader::Invalid:
            qWarning("Error reading %s:%d: %s", qPrintable(file),
                     int(r.lineNumber()), qPrintable(r.errorString()));
            return QVariantMap();
        default:
            break;
        } // switch token
    } // while (!r.atEnd())
    return m_result;
}

bool ParseContext::handleStartElement(QXmlStreamReader &r)
{
    const QStringView name = r.name();
    const Element e = element(name);
    if (e == VariableElement) {
        m_currentVariableName = r.readElementText();
        return false;
    }
    if (!ParseContext::isValueElement(e))
        return false;

    const QXmlStreamAttributes attributes = r.attributes();
    const QString key = attributes.hasAttribute(keyAttribute) ?
                attributes.value(keyAttribute).toString() : QString();
    switch (e) {
    case SimpleValueElement: {
        // This reads away the end element, so, handle end element right here.
        const QVariant v = readSimpleValue(r, attributes);
        if (!v.isValid()) {
            qWarning() << ParseContext::formatWarning(r, QString::fromLatin1("Failed to read element \"%1\".").arg(name.toString()));
            return false;
        }
        m_valueStack.push_back(ParseValueStackEntry(v, key));
        return handleEndElement(name);
    }
    case ListValueElement:
        m_valueStack.push_back(ParseValueStackEntry(QVariant::List, key));
        break;
    case MapValueElement:
        m_valueStack.push_back(ParseValueStackEntry(QVariant::Map, key));
        break;
    default:
        break;
    }
    return false;
}

bool ParseContext::handleEndElement(const QStringView &name)
{
    const Element e = element(name);
    if (ParseContext::isValueElement(e)) {
        QTC_ASSERT(!m_valueStack.isEmpty(), return true);
        const ParseValueStackEntry top = m_valueStack.pop();
        if (m_valueStack.isEmpty()) { // Last element? -> Done with that variable.
            QTC_ASSERT(!m_currentVariableName.isEmpty(), return true);
            m_result.insert(m_currentVariableName, top.value());
            m_currentVariableName.clear();
            return false;
        }
        m_valueStack.top().addChild(top.key, top.value());
    }
    return e == QtCreatorElement;
}

QString ParseContext::formatWarning(const QXmlStreamReader &r, const QString &message)
{
    QString result = QLatin1String("Warning reading ");
    if (const QIODevice *device = r.device())
        if (const auto file = qobject_cast<const QFile *>(device))
            result += QDir::toNativeSeparators(file->fileName()) + QLatin1Char(':');
    result += QString::number(r.lineNumber());
    result += QLatin1String(": ");
    result += message;
    return result;
}

ParseContext::Element ParseContext::element(const QStringView &r) const
{
    if (r == valueElement)
        return SimpleValueElement;
    if (r == valueListElement)
        return ListValueElement;
    if (r == valueMapElement)
        return MapValueElement;
    if (r == qtCreatorElement)
        return QtCreatorElement;
    if (r == dataElement)
        return DataElement;
    if (r == variableElement)
        return VariableElement;
    return UnknownElement;
}

QVariant ParseContext::readSimpleValue(QXmlStreamReader &r, const QXmlStreamAttributes &attributes) const
{
    // Simple value
    const QStringView type = attributes.value(typeAttribute);
    const QString text = r.readElementText();
    if (type == QLatin1String("QChar")) { // Workaround: QTBUG-12345
        QTC_ASSERT(text.size() == 1, return QVariant());
        return QVariant(QChar(text.at(0)));
    }
    if (type == QLatin1String("QRect")) {
        const QRect rectangle = stringToRectangle(text);
        return rectangle.isValid() ? QVariant(rectangle) : QVariant();
    }
    QVariant value;
    value.setValue(text);
    value.convert(QMetaType::type(type.toLatin1().constData()));
    return value;
}

// =================================== SdkPersistentSettingsReader

SdkPersistentSettingsReader::SdkPersistentSettingsReader() = default;

QVariant SdkPersistentSettingsReader::restoreValue(const QString &variable, const QVariant &defaultValue) const
{
    if (m_valueMap.contains(variable))
        return m_valueMap.value(variable);
    return defaultValue;
}

QVariantMap SdkPersistentSettingsReader::restoreValues() const
{
    return m_valueMap;
}

bool SdkPersistentSettingsReader::load(const QString &fileName)
{
    m_valueMap.clear();

    if (QFileInfo(fileName).size() == 0) // skip empty files
        return false;

    ParseContext ctx;
    m_valueMap = ctx.parse(fileName);
    return true;
}

/*!
    \class SdkPersistentSettingsWriter

    \note This is a fork of Utils::PersistentSettingsWriter

    \brief The SdkPersistentSettingsWriter class serializes a QVariantMap of
    arbitrary, nested data structures to an XML file.
    \sa SdkPersistentSettingsReader
*/

static void writeVariantValue(QXmlStreamWriter &w, const Context &ctx,
                              const QVariant &variant, const QString &key = QString())
{
    switch (static_cast<int>(variant.type())) {
    case static_cast<int>(QVariant::StringList):
    case static_cast<int>(QVariant::List): {
        w.writeStartElement(ctx.valueListElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(QVariant::typeToName(QVariant::List)));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        const QList<QVariant> list = variant.toList();
        for (const QVariant &var : list)
            writeVariantValue(w, ctx, var);
        w.writeEndElement();
        break;
    }
    case static_cast<int>(QVariant::Map): {
        w.writeStartElement(ctx.valueMapElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(QVariant::typeToName(QVariant::Map)));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        const QVariantMap varMap = variant.toMap();
        const QVariantMap::const_iterator cend = varMap.constEnd();
        for (QVariantMap::const_iterator i = varMap.constBegin(); i != cend; ++i)
            writeVariantValue(w, ctx, i.value(), i.key());
        w.writeEndElement();
    }
    break;
    case static_cast<int>(QMetaType::QObjectStar): // ignore QObjects!
    case static_cast<int>(QMetaType::VoidStar): // ignore void pointers!
        break;
    default:
        w.writeStartElement(ctx.valueElement);
        w.writeAttribute(ctx.typeAttribute, QLatin1String(variant.typeName()));
        if (!key.isEmpty())
            w.writeAttribute(ctx.keyAttribute, key);
        switch (variant.type()) {
        case QVariant::Rect:
            w.writeCharacters(rectangleToString(variant.toRect()));
            break;
        default:
            w.writeCharacters(variant.toString());
            break;
        }
        w.writeEndElement();
        break;
    }
}

SdkPersistentSettingsWriter::SdkPersistentSettingsWriter(const QString &fileName, const QString &docType) :
    m_fileName(fileName), m_docType(docType)
{ }

bool SdkPersistentSettingsWriter::save(const QVariantMap &data, QString *errorString) const
{
    if (data == m_savedData)
        return true;
    return write(data, errorString);
}

QString SdkPersistentSettingsWriter::fileName() const
{ return m_fileName; }

//** * @brief Set contents of file (e.g. from data read from it). */
void SdkPersistentSettingsWriter::setContents(const QVariantMap &data)
{
    m_savedData = data;
}

bool SdkPersistentSettingsWriter::write(const QVariantMap &data, QString *errorString) const
{
    const QString parentDir = cleanPath(m_fileName + "/..");

    const QFileInfo fi(parentDir);
    if (!(fi.exists() && fi.isDir() && fi.isWritable())) {
        bool res = QDir().mkpath(parentDir);
        if (!res)
            return false;
    }

    SdkFileSaver saver(m_fileName, QIODevice::Text);
    if (!saver.hasError()) {
        const Context ctx;
        QXmlStreamWriter w(saver.file());
        w.setAutoFormatting(true);
        w.setAutoFormattingIndent(1); // Historical, used to be QDom.
        w.writeStartDocument();
        w.writeDTD(QLatin1String("<!DOCTYPE ") + m_docType + QLatin1Char('>'));
        w.writeComment(QString::fromLatin1(" Written by %1 %2, %3. ").
                       arg(QCoreApplication::applicationName(),
                           QCoreApplication::applicationVersion(),
                           QDateTime::currentDateTime().toString(Qt::ISODate)));
        w.writeStartElement(ctx.qtCreatorElement);
        const QVariantMap::const_iterator cend = data.constEnd();
        for (QVariantMap::const_iterator it =  data.constBegin(); it != cend; ++it) {
            w.writeStartElement(ctx.dataElement);
            w.writeTextElement(ctx.variableElement, it.key());
            writeVariantValue(w, ctx, it.value());
            w.writeEndElement();
        }
        w.writeEndDocument();

        saver.setResult(&w);
    }
    bool ok = saver.finalize();
    if (ok) {
        m_savedData = data;
    } else if (errorString) {
        m_savedData.clear();
        *errorString = saver.errorString();
    }

    return ok;
}
