// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filepath.h"

#include "algorithm.h"
#include "devicefileaccess.h"
#include "environment.h"
#include "filestreamermanager.h"
#include "fileutils.h"
#include "hostosinfo.h"
#include "qtcassert.h"
#include "utilstr.h"

#include <QByteArray>
#include <QDateTime>
#include <QDebug>
#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <QStringView>
#include <QUrl>
#include <QtGlobal>
#include <QTemporaryFile>

#ifdef Q_OS_WIN
#ifdef QTCREATOR_PCH_H
#define CALLBACK WINAPI
#endif
#include <qt_windows.h>
#include <shlobj.h>
#endif

namespace Utils {

static DeviceFileHooks s_deviceHooks;
inline bool isWindowsDriveLetter(QChar ch);


/*!
    \class Utils::FilePath
    \inmodule QtCreator

    \brief The FilePath class is an abstraction for handles to objects
    in a (possibly remote) file system, similar to a URL or, in the local
    case, a path to a file or directory.

    Ideally, all of \QC code should use FilePath for this purpose,
    but for historic reasons there are still large parts using QString.

    FilePaths are internally stored as triple of strings, with one
    part ("scheme") identifying an access method, a second part ("host")
    a file system (e.g. a host) and a third part ("path") identifying
    a (potential) object on the systems.

    FilePath follows the Unix paradigm of "everything is a file":
    There is no conceptional difference between FilePaths referring
    to plain files or directories.

    A FilePath is implicitly associated with an operating system via its
    host part. The path part of a FilePath is internally stored
    with forward slashes, independent of the associated OS.

    The path parts of FilePaths associated with Windows (and macOS,
    unless selected otherwise in the settings) are compared case-insensitively
    to each other.
    Note that comparisons for equivalence generally need out-of-band
    knowledge, as there may be multiple FilePath representations for
    the same file (e.g. different access methods may lead to the same
    file).

    There are several conversions between FilePath and other string-like
    representations:

    \list

    \li FilePath::fromUserInput()

        Convert string-like data from sources originating outside of
        \QC, e.g. from human input in GUI controls, from environment
        variables and from command-line parameters to \QC.

        The input can contain both slashes and backslashes and will
        be parsed and normalized.

    \li FilePath::nativePath()

        Converts the FilePath to the slash convention of the associated
        OS and drops the scheme and host parts.

        This is useful to interact with the facilities of the associated
        OS, e.g. when passing this FilePath as an argument to a command
        executed on the associated OS.

        \note The FilePath passed as executable to a CommandLine is typically
        not touched by user code. The Process will use it to determine
        the remote system and apply the necessary conversions internally.

    \li FilePath::toFSPathString()

        Converts the FilePath to a [drive:]/__qtc_devices__/scheme/host/path
        string.

        The result works in most cases also with remote setups and QDir/QFileInfo
        but is slower than direct FilePath use and should only be used when
        proper porting to FilePath is too difficult, or not possible, e.g.
        when external Qt based libraries are used that do not use FilePath.

    \li FilePath::toUserOutput()

        Converts the FilePath to the slash convention of the associated
        OS and retains the scheme and host parts.

        This is rarely useful for remote paths as there is practically
        no consumer of this style.

    \li FilePath::displayName()

        Converts the FilePath to the slash convention of the associated
        OS and adds the scheme and host as a " on <device>" suffix.

        This is useful for static user-facing output in the GUI.

    \li FilePath::fromVariant(), FilePath::toVariant()

        These are used to interface QVariant-based API, e.g.
        settings or item model (internal) data.

    \li FilePath::fromString(), FilePath::toString()

        These are used for internal interfaces to code areas that
        still use QString based file paths for simple storage and retrieval.

        In most cases, use of one of the more specialized above is
        more appropriate.

    \endlist

    Conversion of string-like data should always happen at the outer boundary
    of \QC code, using \c fromUserInput() for in-bound communication,
    and depending on the medium \c nativePath() or \c displayName() for out-bound
    communication.

    Communication with QVariant based Qt API should use \c fromVariant() and
    \c toVariant().

    Uses of \c fromString() and \c toString() should be phased out by transforming
    code from QString based file path to FilePath. An exception here are
    fragments of paths of a FilePath that are later used with \c pathAppended()
    or similar which should be kept as QString.

    UNC paths will retain their "//" begin, and are recognizable by this.
*/

FilePath::FilePath()
{
}

/*!
   Constructs a FilePath from \a info.
*/
FilePath FilePath::fromFileInfo(const QFileInfo &info)
{
    return FilePath::fromString(info.absoluteFilePath());
}

/*!
   Returns a QFileInfo.
*/
QFileInfo FilePath::toFileInfo() const
{
    return QFileInfo(toFSPathString());
}

/*!
    Constructs a FilePath from \a variant.

    \sa toVariant()
*/
FilePath FilePath::fromVariant(const QVariant &variant)
{
    return fromSettings(variant); // FIXME: Use variant.value<FilePath>()
}

FilePath FilePath::fromParts(const QStringView scheme, const QStringView host, const QStringView path)
{
    FilePath result;
    result.setParts(scheme, host, path);
    return result;
}

FilePath FilePath::fromPathPart(const QStringView path)
{
    FilePath result;
    result.m_data = path.toString();
    result.m_pathLen = path.size();
    return result;
}

FilePath FilePath::currentWorkingPath()
{
    return FilePath::fromString(QDir::currentPath());
}

bool FilePath::isRootPath() const
{
    if (needsDevice()) {
        QStringView path = pathView();
        if (osType() != OsTypeWindows)
            return path == QLatin1String("/");

        // Remote windows paths look like this: "/c:/", so we remove the leading '/'
        if (path.startsWith('/'))
            path = path.mid(1);

        if (path.length() > 3)
            return false;

        if (!startsWithDriveLetter())
            return false;

        if (path.length() == 3 && path[2] != QLatin1Char('/'))
            return false;

        return true;
    }

    return *this == HostOsInfo::root();
}

QString FilePath::encodedHost() const
{
    QString result = host().toString();
    result.replace('%', "%25");
    result.replace('/', "%2f");
    return result;
}

QString decodeHost(QString host)
{
    return host.replace("%25", "%").replace("%2f", "/");
}

/*!
    Returns a QString for passing through QString based APIs.

    \note This is obsolete API and should be replaced by extended use
    of proper \c FilePath, or, in case this is not possible by \c toFSPathString().

    This uses a scheme://host/path setup and is, together with
    fromString, used to pass FilePath through \c QString using
    code paths.

    The result is not useful for use with \cQDir and \c QFileInfo
    and gets destroyed by some operations like \c QFileInfo::canonicalFile.

    \sa toFSPathString()
*/
QString FilePath::toString() const
{
    if (!needsDevice())
        return path();

    if (pathView().isEmpty())
        return scheme() + "://" + encodedHost();

    if (isRelativePath())
        return scheme() + "://" + encodedHost() + "/./" + pathView();
    return scheme() + "://" + encodedHost() + pathView();
}

/*!
    Returns a QString for passing on to QString based APIs.

    This uses a /__qtc_devices__/host/path setup.

    This works in most cases also with remote setups and QDir/QFileInfo etc.
    but is slower than direct FilePath use and should only be used when
    proper porting to FilePath is too difficult, or not possible, e.g.
    when external Qt based libraries are used that do not use FilePath.

    \sa fromUserInput()
*/
QString FilePath::toFSPathString() const
{
    if (scheme().isEmpty())
        return path();

    if (pathView().isEmpty())
        return specialRootPath() + '/' + scheme() + '/' + encodedHost();

    if (isRelativePath())
        return specialRootPath() + '/' + scheme() + '/' + encodedHost() + "/./" + pathView();
    return specialRootPath() + '/' + scheme() + '/' + encodedHost() + pathView();
}

QUrl FilePath::toUrl() const
{
    if (!needsDevice())
        return QUrl::fromLocalFile(toFSPathString());
    QUrl url;
    url.setScheme(scheme().toString());
    url.setHost(host().toString());
    url.setPath(path());
    return url;
}

/*!
    Returns a QString to display to the user, including the device prefix.

    Converts the separators to the native format of the system
    this path belongs to.
*/
QString FilePath::toUserOutput() const
{
    QString tmp = toString();
    if (needsDevice())
        return tmp;

    if (osType() == OsTypeWindows)
        tmp.replace('/', '\\');
    return tmp;
}

/*!
    Returns a QString to pass to target system native commands, without the device prefix.

    Converts the separators to the native format of the system
    this path belongs to.
*/

QString FilePath::nativePath() const
{
    QString data = path();
    if (osType() == OsTypeWindows)
        data.replace('/', '\\');
    return data;
}

QStringView FilePath::fileNameView() const
{
    const QStringView fp = pathView();
    return fp.mid(fp.lastIndexOf('/') + 1);
}

QString FilePath::fileName() const
{
    return fileNameView().toString();
}

QString FilePath::fileNameWithPathComponents(int pathComponents) const
{
    QString fullPath = path();

    if (pathComponents < 0)
        return fullPath;
    const QChar slash = QLatin1Char('/');
    int i = fullPath.lastIndexOf(slash);
    if (pathComponents == 0 || i == -1)
        return fullPath.mid(i + 1);
    int component = i + 1;
    // skip adjacent slashes
    while (i > 0 && fullPath.at(--i) == slash)
        ;
    while (i >= 0 && --pathComponents >= 0) {
        i = fullPath.lastIndexOf(slash, i);
        component = i + 1;
        while (i > 0 && fullPath.at(--i) == slash)
            ;
    }

    if (i > 0 && fullPath.lastIndexOf(slash, i) != -1)
        return fullPath.mid(component);

    // If there are no more slashes before the found one, return the entire string
    return toString();
}

/*!
    Returns the base name of the file without the path.

    The base name consists of all characters in the file up to
    (but not including) the first '.' character.
*/
QString FilePath::baseName() const
{
    const QString &name = fileName();
    return name.left(name.indexOf('.'));
}

/*!
   Returns the complete base name of the file without the path.

    The complete base name consists of all characters in the file up to
    (but not including) the last '.' character. In case of ".ui.qml"
    it will be treated as one suffix.
*/
QString FilePath::completeBaseName() const
{
    const QString &name = fileName();
    if (name.endsWith(".ui.qml"))
        return name.left(name.length() - QString(".ui.qml").length());
    return name.left(name.lastIndexOf('.'));
}

/*!
   Returns the suffix (extension) of the file.

    The suffix consists of all characters in the file after
    (but not including) the last '.'. In case of ".ui.qml" it will
    be treated as one suffix.
*/
QStringView FilePath::suffixView() const
{
    const QStringView name = fileNameView();
    if (name.endsWith(u".ui.qml"))
        return u"ui.qml";
    const int index = name.lastIndexOf('.');
    if (index >= 0)
        return name.mid(index + 1);
    return {};
}

QString FilePath::suffix() const
{
    return suffixView().toString();
}

/*!
   Returns the complete suffix (extension) of the file.

    The complete suffix consists of all characters in the file after
    (but not including) the first '.'.
*/
QString FilePath::completeSuffix() const
{
    const QString &name = fileName();
    const int index = name.indexOf('.');
    if (index >= 0)
        return name.mid(index + 1);
    return {};
}

QStringView FilePath::scheme() const
{
    return QStringView{m_data}.mid(m_pathLen, m_schemeLen);
}

QStringView FilePath::host() const
{
    return QStringView{m_data}.mid(m_pathLen + m_schemeLen, m_hostLen);
}

QStringView FilePath::pathView() const
{
    return QStringView(m_data).left(m_pathLen);
}

QString FilePath::path() const
{
    return pathView().toString();
}

void FilePath::setParts(const QStringView scheme, const QStringView host, QStringView path)
{
    QTC_CHECK(!scheme.contains('/'));

    if (path.length() >= 3 && path[0] == '/' && path[1] == '.' && path[2] == '/')
        path = path.mid(3);

    m_hash = 0;
    m_data = path.toString() + scheme.toString() + host.toString();
    m_schemeLen = scheme.size();
    m_hostLen = host.size();
    m_pathLen = path.size();
}

/*!
    \brief Re-uses or creates a directory in this location.

    Returns true if the directory is writable afterwards.

    \sa createDir()
*/
expected_str<void> FilePath::ensureWritableDir() const
{
    return fileAccess()->ensureWritableDirectory(*this);
}

bool FilePath::ensureExistingFile() const
{
    return fileAccess()->ensureExistingFile(*this);
}

/*!
    Returns a bool indicating on whether a process with this FilePath's
    native path is likely to start.

    This is equivalent to \l isExecutableFile() in general.
    On Windows, it might append various suffixes depending on
    \a matchScope.
*/
std::optional<FilePath> FilePath::refersToExecutableFile(MatchScope matchScope) const
{
    return fileAccess()->refersToExecutableFile(*this,  matchScope);
}

expected_str<FilePath> FilePath::tmpDir() const
{
    if (needsDevice()) {
        const expected_str<Environment> env = deviceEnvironmentWithError();
        if (!env)
            return make_unexpected(env.error());

        if (env->hasKey("TMPDIR"))
            return withNewPath(env->value("TMPDIR")).cleanPath();
        if (env->hasKey("TEMP"))
            return withNewPath(env->value("TEMP")).cleanPath();
        if (env->hasKey("TMP"))
            return withNewPath(env->value("TMP")).cleanPath();

        if (osType() != OsTypeWindows)
            return withNewPath("/tmp");
        return make_unexpected(QString("Could not find temporary directory on device %1")
                               .arg(displayName()));
    }

    return FilePath::fromUserInput(QDir::tempPath());
}

expected_str<FilePath> FilePath::createTempFile() const
{
    if (!needsDevice()) {
        QTemporaryFile file(toFSPathString());
        file.setAutoRemove(false);
        if (file.open())
            return FilePath::fromString(file.fileName());

        return make_unexpected(QString("Could not create temporary file: %1").arg(file.errorString()));
    }

    return fileAccess()->createTempFile(*this);
}

bool FilePath::hasHardLinks() const
{
    return fileAccess()->hasHardLinks(*this);
}

/*!
    \brief Creates a directory in this location.

    Returns true if the directory could be created, false if not,
    even if it existed before.

    \sa ensureWritableDir()
*/
bool FilePath::createDir() const
{
    return fileAccess()->createDirectory(*this);
}

FilePaths FilePath::dirEntries(const FileFilter &filter, QDir::SortFlags sort) const
{
    FilePaths result;

    iterateDirectory(
        [&result](const FilePath &path) { result.append(path); return IterationPolicy::Continue; },
        filter
     );

    // FIXME: Not all flags supported here.
    const QDir::SortFlags sortBy = (sort & QDir::SortByMask);
    if (sortBy == QDir::Name) {
        Utils::sort(result);
    } else if (sortBy == QDir::Time) {
        Utils::sort(result, [](const FilePath &path1, const FilePath &path2) {
            return path1.lastModified() < path2.lastModified();
        });
    }

    if (sort != QDir::NoSort && (sort & QDir::Reversed))
        std::reverse(result.begin(), result.end());

    return result;
}

FilePaths FilePath::dirEntries(QDir::Filters filters) const
{
    return dirEntries(FileFilter({}, filters));
}

/*!
    Runs \a callBack on each directory entry matching the \a filter.
*/

void FilePath::iterateDirectory(const IterateDirCallback &callBack, const FileFilter &filter) const
{
    fileAccess()->iterateDirectory(*this, callBack, filter);
}

void FilePath::iterateDirectories(const FilePaths &dirs,
                                  const IterateDirCallback &callBack,
                                  const FileFilter &filter)
{
    for (const FilePath &dir : dirs)
        dir.iterateDirectory(callBack, filter);
}

expected_str<QByteArray> FilePath::fileContents(qint64 maxSize, qint64 offset) const
{
    return fileAccess()->fileContents(*this, maxSize, offset);
}

bool FilePath::ensureReachable(const FilePath &other) const
{
    if (needsDevice()) {
        QTC_ASSERT(s_deviceHooks.ensureReachable, return false);
        return s_deviceHooks.ensureReachable(*this, other);
    } else if (!other.needsDevice()) {
        return true;
    }
    return false;
}

expected_str<qint64> FilePath::writeFileContents(const QByteArray &data, qint64 offset) const
{
    return fileAccess()->writeFileContents(*this, data, offset);
}

FileStreamHandle FilePath::asyncCopy(const FilePath &target, QObject *context,
                                     const CopyContinuation &cont) const
{
    return FileStreamerManager::copy(*this, target, context, cont);
}

FileStreamHandle FilePath::asyncRead(QObject *context, const ReadContinuation &cont) const
{
    return FileStreamerManager::read(*this, context, cont);
}

FileStreamHandle FilePath::asyncWrite(const QByteArray &data, QObject *context,
                                      const WriteContinuation &cont) const
{
    return FileStreamerManager::write(*this, data, context, cont);
}

bool FilePath::needsDevice() const
{
    return m_schemeLen != 0;
}

bool FilePath::isSameDevice(const FilePath &other) const
{
    if (needsDevice() != other.needsDevice())
        return false;
    if (!needsDevice() && !other.needsDevice())
        return true;

    QTC_ASSERT(s_deviceHooks.isSameDevice, return true);
    return s_deviceHooks.isSameDevice(*this, other);
}

bool FilePath::isSameFile(const FilePath &other) const
{
    if (*this == other)
        return true;

    if (!isSameDevice(other))
        return false;

    const QByteArray fileId = fileAccess()->fileId(*this);
    const QByteArray otherFileId = fileAccess()->fileId(other);
    if (fileId.isEmpty() || otherFileId.isEmpty())
        return false;

    if (fileId == otherFileId)
        return true;

    return false;
}

static FilePaths appendExeExtensions(const FilePath &executable,
                                     FilePath::MatchScope matchScope)
{
    FilePaths result = {executable};
    const QStringView suffix = executable.suffixView();
    if (executable.osType() == OsTypeWindows && suffix.isEmpty()) {
        switch (matchScope) {
            case FilePath::ExactMatchOnly:
                break;
            case FilePath::WithExeSuffix:
                result.append(executable.stringAppended(".exe"));
                break;
            case FilePath::WithBatSuffix:
                result.append(executable.stringAppended(".bat"));
                break;
            case FilePath::WithExeOrBatSuffix:
                result.append(executable.stringAppended(".exe"));
                result.append(executable.stringAppended(".bat"));
                break;
            case FilePath::WithAnySuffix: {
                // Check all the executable extensions on windows:
                // PATHEXT is only used if the executable has no extension
                static const QStringList extensions = Environment::systemEnvironment()
                    .expandedValueForKey("PATHEXT").split(';');
                for (const QString &ext : extensions)
                    result.append(executable.stringAppended(ext.toLower()));
                break;
            }
            default:
                break;
        }
    }
    return result;
}

bool FilePath::isSameExecutable(const FilePath &other) const
{
    if (*this == other)
        return true;

    if (!isSameDevice(other))
        return false;

    const FilePaths exe1List = appendExeExtensions(*this, WithAnySuffix);
    const FilePaths exe2List = appendExeExtensions(other, WithAnySuffix);
    for (const FilePath &f1 : exe1List) {
        for (const FilePath &f2 : exe2List) {
            if (f1.isSameFile(f2))
                return true;
        }
    }
    return false;
}

/*!
    Returns an empty FilePath if this is not a symbolic link.
*/
FilePath FilePath::symLinkTarget() const
{
    return fileAccess()->symLinkTarget(*this);
}

FilePath FilePath::withExecutableSuffix() const
{
    return withNewPath(OsSpecificAspects::withExecutableSuffix(osType(), path()));
}

FilePath FilePath::withSuffix(const QString &suffix) const
{
    return withNewPath(path() + suffix);
}

static bool startsWithWindowsDriveLetterAndSlash(QStringView path)
{
    return path.size() > 2 && path[1] == ':' && path[2] == '/' && isWindowsDriveLetter(path[0]);
}

int FilePath::rootLength(const QStringView path)
{
    if (path.size() == 0)
        return 0;

    if (path.size() == 1)
        return path[0] == '/' ? 1 : 0;

    if (path[0] == '/' && path[1] == '/') { // UNC, FIXME: Incomplete
        if (path.size() == 2)
            return 2; // case deviceless UNC root - assuming there's such a thing.
        const int pos = path.indexOf('/', 2);
        if (pos == -1)
            return path.size(); // case   //localhost
        return pos + 1;     // case   //localhost/*
    }

    if (startsWithWindowsDriveLetterAndSlash(path))
        return 3; // FIXME-ish: same assumption as elsewhere: we assume "x:/" only ever appears as root

    if (path[0] == '/')
        return 1;

    return 0;
}

int FilePath::schemeAndHostLength(const QStringView path)
{
    static const QLatin1String colonSlashSlash("://");

    const int sep = path.indexOf(colonSlashSlash);
    if (sep == -1)
        return 0;

    const int pos = path.indexOf('/', sep + 3);
    if (pos == -1) // Just   scheme://host
        return path.size();

    return pos + 1;  // scheme://host/ plus something
}

static QString normalizePathSegmentHelper(const QString &name)
{
    const int len = name.length();

    if (len == 0 || name.contains("%{"))
        return name;

    int i = len - 1;
    QVarLengthArray<char16_t> outVector(len);
    int used = len;
    char16_t *out = outVector.data();
    const ushort *p = reinterpret_cast<const ushort *>(name.data());
    const ushort *prefix = p;
    int up = 0;

    const int prefixLength = name.at(0) == u'/' ? 1 : 0;

    p += prefixLength;
    i -= prefixLength;

    // replicate trailing slash (i > 0 checks for emptiness of input string p)
    // except for remote paths because there can be /../ or /./ ending
    if (i > 0 && p[i] == '/') {
        out[--used] = '/';
        --i;
    }

    while (i >= 0) {
        if (p[i] == '/') {
            --i;
            continue;
        }

        // remove current directory
        if (p[i] == '.' && (i == 0 || p[i - 1] == '/')) {
            --i;
            continue;
        }

        // detect up dir
        if (i >= 1 && p[i] == '.' && p[i - 1] == '.' && (i < 2 || p[i - 2] == '/')) {
            ++up;
            i -= i >= 2 ? 3 : 2;
            continue;
        }

        // prepend a slash before copying when not empty
        if (!up && used != len && out[used] != '/')
            out[--used] = '/';

        // skip or copy
        while (i >= 0) {
            if (p[i] == '/') {
                --i;
                break;
            }

            // actual copy
            if (!up)
                out[--used] = p[i];
            --i;
        }

        // decrement up after copying/skipping
        if (up)
            --up;
    }

    // Indicate failure when ".." are left over for an absolute path.
    //    if (ok)
    //        *ok = prefixLength == 0 || up == 0;

    // add remaining '..'
    while (up) {
        if (used != len && out[used] != '/') // is not empty and there isn't already a '/'
            out[--used] = '/';
        out[--used] = '.';
        out[--used] = '.';
        --up;
    }

    bool isEmpty = used == len;

    if (prefixLength) {
        if (!isEmpty && out[used] == '/') {
            // Even though there is a prefix the out string is a slash. This happens, if the input
            // string only consists of a prefix followed by one or more slashes. Just skip the slash.
            ++used;
        }
        for (int i = prefixLength - 1; i >= 0; --i)
            out[--used] = prefix[i];
    } else {
        if (isEmpty) {
            // After resolving the input path, the resulting string is empty (e.g. "foo/.."). Return
            // a dot in that case.
            out[--used] = '.';
        } else if (out[used] == '/') {
            // After parsing the input string, out only contains a slash. That happens whenever all
            // parts are resolved and there is a trailing slash ("./" or "foo/../" for example).
            // Prepend a dot to have the correct return value.
            out[--used] = '.';
        }
    }

    // If path was not modified return the original value
    if (used == 0)
        return name;
    return QString::fromUtf16(out + used, len - used);
}

QString doCleanPath(const QString &input_)
{
    QString input = input_;
    if (input.contains('\\'))
        input.replace('\\', '/');

    if (input.startsWith("//?/")) {
        input = input.mid(4);
        if (input.startsWith("UNC/"))
            input = '/' + input.mid(3); // trick it into reporting two slashs at start
    }

    int prefixLen = 0;
    const int shLen = FilePath::schemeAndHostLength(input);
    if (shLen > 0) {
        prefixLen = shLen + FilePath::rootLength(input.mid(shLen));
    } else {
        prefixLen = FilePath::rootLength(input);
        if (prefixLen > 0 && input.at(prefixLen - 1) == '/')
            --prefixLen;
    }

    QString path = normalizePathSegmentHelper(input.mid(prefixLen));

    // Strip away last slash except for root directories
    if (path.size() > 1 && path.endsWith(u'/'))
        path.chop(1);

    return input.left(prefixLen) + path;
}

/*!
    Finds the parent directory of the file path.

    Returns an empty file path if the file path is already
    a root level directory.

    Returns a file path with the last segment removed.
*/
FilePath FilePath::parentDir() const
{
    const QString basePath = path();
    if (basePath.isEmpty())
        return {};

    const QString path = basePath + QLatin1String("/..");
    const QString parent = doCleanPath(path);
    if (parent == path)
        return {};

    return withNewPath(parent);
}

FilePath FilePath::absolutePath() const
{
    if (!needsDevice() && isEmpty())
        return *this;
    const FilePath parentPath = isAbsolutePath()
                                    ? parentDir()
                                    : FilePath::currentWorkingPath().resolvePath(*this).parentDir();
    return parentPath.isEmpty() ? *this : parentPath;
}

FilePath FilePath::absoluteFilePath() const
{
    if (isAbsolutePath())
        return cleanPath();
    if (!needsDevice() && isEmpty())
        return cleanPath();

    return FilePath::currentWorkingPath().resolvePath(*this);
}

const QString &FilePath::specialRootName()
{
    static const QString rootName = "__qtc_devices__";
    return rootName;
}

const QString &FilePath::specialRootPath()
{
    static const QString rootPath = HostOsInfo::root().path() + u"__qtc_devices__";
    return rootPath;
}

const QString &FilePath::specialDeviceRootName()
{
    static const QString deviceRootName = "device";
    return deviceRootName;
}

const QString &FilePath::specialDeviceRootPath()
{
    static const QString deviceRootPath = HostOsInfo::root().path() + u"__qtc_devices__/device";
    return deviceRootPath;
}

FilePath FilePath::normalizedPathName() const
{
    FilePath result = *this;
    if (!needsDevice()) // FIXME: Assumes no remote Windows and Mac for now.
        result.setParts(scheme(), host(), FileUtils::normalizedPathName(path()));
    return result;
}

/*!
    Converts the file path to the slash convention of the associated
    OS and adds the scheme and host as a " on <device>" suffix.

    This is useful for static user-facing output in the GUI.

    If \a args is not empty, it is added to the output after the file path:
    "<path> <args> on <device>".
*/
QString FilePath::displayName(const QString &args) const
{
    QString deviceName;
    if (needsDevice()) {
        QTC_ASSERT(s_deviceHooks.deviceDisplayName, return nativePath());
        deviceName = s_deviceHooks.deviceDisplayName(*this);
    }

    const QString fullPath = nativePath();

    if (args.isEmpty()) {
        if (deviceName.isEmpty())
            return fullPath;

        return Tr::tr("%1 on %2", "File on device").arg(fullPath, deviceName);
    }

    if (deviceName.isEmpty())
        return fullPath + ' ' + args;

    return Tr::tr("%1 %2 on %3", "File and args on device")
            .arg(fullPath, args, deviceName);
}

/*!
    \fn FilePath FilePath::fromString(const QString &filepath)

    Constructs a FilePath from \a filepath

    \a filepath is not checked for validity. It can be given in the following forms:

    \list
    \li  /some/absolute/local/path
    \li  some/relative/path
    \li  scheme://host/absolute/path
    \li  scheme://host/./relative/path    \note the ./ is verbatim part of the path
    \endlist

    Some decoding happens when parsing the \a filepath
    A sequence %25 present in the host part is replaced by % in the host name,
    a sequence %2f present in the host part is replaced by / in the host name.

    The path part might consist of several parts separated by /, independent
    of the platform or file system.

    To create FilePath objects from strings possibly containing backslashes as
    path separator, use \c fromUserInput.

    \sa toString, fromUserInput
*/
FilePath FilePath::fromString(const QString &filepath)
{
    FilePath fn;
    fn.setFromString(filepath);
    return fn;
}

bool isWindowsDriveLetter(QChar ch)
{
    return (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z');
}

void FilePath::setPath(QStringView path)
{
    setParts(scheme(), host(), path);
}

void FilePath::setFromString(QStringView fileNameView)
{
    static const QStringView qtcDevSlash(u"__qtc_devices__/");
    static const QStringView colonSlashSlash(u"://");

#if 1
    // FIXME: Remove below once the calling code is adjusted
    QString dummy;
    if (fileNameView.contains(u'\\')) {
        QTC_CHECK(false);
        dummy = fileNameView.toString();
        dummy.replace('\\', '/');
        fileNameView = dummy;
    }
#endif

    const QChar slash('/');

    bool startsWithQtcSlashDev = false;
    QStringView withoutQtcDeviceRoot = fileNameView;
    if (fileNameView.startsWith(slash) && fileNameView.mid(1).startsWith(qtcDevSlash)) {
        startsWithQtcSlashDev = true;
        withoutQtcDeviceRoot = withoutQtcDeviceRoot.mid(1 + qtcDevSlash.size());
    } else if (fileNameView.size() > 3 && isWindowsDriveLetter(fileNameView.at(0))
               && fileNameView.at(1) == ':' && fileNameView.mid(3).startsWith(qtcDevSlash)) {
        startsWithQtcSlashDev = true;
        withoutQtcDeviceRoot = withoutQtcDeviceRoot.mid(3 + qtcDevSlash.size());
    }

    if (startsWithQtcSlashDev) {
        const int firstSlash = withoutQtcDeviceRoot.indexOf(slash);

        if (firstSlash != -1) {
            const QString scheme = withoutQtcDeviceRoot.left(firstSlash).toString();
            const int secondSlash = withoutQtcDeviceRoot.indexOf(slash, firstSlash + 1);
            const QString host = decodeHost(
                withoutQtcDeviceRoot.mid(firstSlash + 1, secondSlash - firstSlash - 1).toString());
            if (secondSlash != -1) {
                QStringView path = withoutQtcDeviceRoot.mid(secondSlash);
                setParts(scheme, host, path);
                return;
            }

            setParts(scheme, host, u"/");
            return;
        }

        setParts({}, {}, fileNameView);
        return;
    }

    const int firstSlash = fileNameView.indexOf(slash);
    const int schemeEnd = fileNameView.indexOf(colonSlashSlash);
    if (schemeEnd != -1 && schemeEnd < firstSlash) {
        // This is a pseudo Url, we can't use QUrl here sadly.
        const QStringView scheme = fileNameView.left(schemeEnd);
        const int hostEnd = fileNameView.indexOf(slash, schemeEnd + 3);
        const QString host = decodeHost(
                    fileNameView.mid(schemeEnd + 3, hostEnd - schemeEnd - 3).toString());
        setParts(scheme, host, hostEnd != -1 ? fileNameView.mid(hostEnd) : QStringView());
        return;
    }

    setParts({}, {}, fileNameView);
}

static expected_str<DeviceFileAccess *> getFileAccess(const FilePath &filePath)
{
    if (!filePath.needsDevice())
        return DesktopDeviceFileAccess::instance();

    if (!s_deviceHooks.fileAccess) {
        // Happens during startup and in tst_fsengine
        QTC_CHECK(false);
        return DesktopDeviceFileAccess::instance();
    }

    return s_deviceHooks.fileAccess(filePath);
}

DeviceFileAccess *FilePath::fileAccess() const
{
    static DeviceFileAccess dummy;
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    QTC_ASSERT_EXPECTED(access, return &dummy);
    return *access;
}

bool FilePath::hasFileAccess() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    return access.has_value();
}

FilePathInfo FilePath::filePathInfo() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return {};
    return (*access)->filePathInfo(*this);
}

/*!
   Returns a bool indicating whether a file or directory with this FilePath exists.
*/
bool FilePath::exists() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->exists(*this);
}

/*!
    Returns a bool indicating whether this is an executable file.
*/
bool FilePath::isExecutableFile() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isExecutableFile(*this);
}

/*!
    Returns a bool indicating whether this is a writable directory.
*/
bool FilePath::isWritableDir() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isWritableDirectory(*this);
}

/*!
    Returns a bool indicating whether this is a writable file.
*/
bool FilePath::isWritableFile() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isWritableFile(*this);
}


bool FilePath::isReadableFile() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isReadableFile(*this);
}

bool FilePath::isReadableDir() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isReadableDirectory(*this);
}

bool FilePath::isFile() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isFile(*this);
}

bool FilePath::isDir() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isDirectory(*this);
}

bool FilePath::isSymLink() const
{
    const expected_str<DeviceFileAccess *> access = getFileAccess(*this);
    if (!access)
        return false;

    return (*access)->isSymLink(*this);
}

/*!
    Constructs a FilePath from \a filepath. The \a defaultExtension is appended
    to \a filepath if that does not have an extension already.

    \a filepath is not checked for validity.
*/
FilePath FilePath::fromStringWithExtension(const QString &filepath, const QString &defaultExtension)
{
    if (filepath.isEmpty() || defaultExtension.isEmpty())
        return FilePath::fromString(filepath);

    FilePath rc = FilePath::fromString(filepath);
    // Add extension unless user specified something else
    const QChar dot = QLatin1Char('.');
    if (!rc.fileName().contains(dot)) {
        if (!defaultExtension.startsWith(dot))
            rc = rc.stringAppended(dot);
        rc = rc.stringAppended(defaultExtension);
    }
    return rc;
}

/*!
    Constructs a FilePath from \a filePath

    The path \a filePath is cleaned, and ~ is replaced by the home path.
*/
FilePath FilePath::fromUserInput(const QString &filePath)
{
    const QString expandedPath = filePath.startsWith("~/")
                                     ? (QDir::homePath() + "/" + filePath.mid(2))
                                     : filePath;
    return FilePath::fromString(doCleanPath(expandedPath));
}

/*!
    Constructs a FilePath from \a filename with \a filenameSize, which is
    encoded as UTF-8.

   \a filename is not checked for validity.
*/
FilePath FilePath::fromUtf8(const char *filename, int filenameSize)
{
    return FilePath::fromString(QString::fromUtf8(filename, filenameSize));
}

FilePath FilePath::fromSettings(const QVariant &variant)
{
    if (variant.type() == QVariant::Url) {
        const QUrl url = variant.toUrl();
        return FilePath::fromParts(url.scheme(), url.host(), url.path());
    }

    // The installer sometimes fails and adds "docker:/..." instead of "docker://...
    // So we fix these paths here in those cases.
    QString data = variant.toString();
    if (data.length() > 8 && data.startsWith("docker:/") && data[8] != '/') {
        qWarning() << "Broken path in settings:" << data << ", applying workaround.";
        data.insert(8, '/');
    }

    return FilePath::fromUserInput(data);
}

QVariant FilePath::toSettings() const
{
    return toString();
}

/*!
    Returns the FilePath as a variant.

    To be used for type-agnostic internal interfaces like storage in
    QAbstractItemModels.
*/
QVariant FilePath::toVariant() const
{
    // FIXME: Use qVariantFromValue
    return toSettings();
}

/*!
    Returns whether FilePath is a child of \a s.
*/
bool FilePath::isChildOf(const FilePath &s) const
{
    if (!s.isSameDevice(*this))
        return false;
    if (s.isEmpty())
        return false;
    const QStringView p = pathView();
    const QStringView sp = s.pathView();
    if (!p.startsWith(sp, caseSensitivity()))
        return false;
    if (p.size() <= sp.size())
        return false;
    // s is root, '/' was already tested in startsWith
    if (sp.endsWith(QLatin1Char('/')))
        return true;
    // s is a directory, next character should be '/' (/tmpdir is NOT a child of /tmp)
    return sp.isEmpty() || p.at(sp.size()) == QLatin1Char('/');
}

/*!
    Returns whether \c path() starts with \a s.
*/
bool FilePath::startsWith(const QString &s) const
{
    return pathView().startsWith(s, caseSensitivity());
}

/*!
   Returns whether \c path() ends with \a s.
*/
bool FilePath::endsWith(const QString &s) const
{
    return pathView().endsWith(s, caseSensitivity());
}

/*!
   Returns whether \c path() contains \a s.
*/
bool FilePath::contains(const QString &s) const
{
    return pathView().contains(s, caseSensitivity());
}

/*!
    \brief Checks whether the FilePath starts with a drive letter.
    Returns whether FilePath starts with a drive letter
*/
bool FilePath::startsWithDriveLetter() const
{
    QStringView p = pathView();
    if (needsDevice() && !p.isEmpty())
        p = p.mid(1);

    return p.size() >= 2 && isWindowsDriveLetter(p[0]) && p.at(1) == ':';
}

/*!
    \brief Relative path from \a parent to this.

    Returns a empty \c FilePath if this is not a child of \a parent.
    \a parent is the Parent to calculate the relative path to.
    That is, this never returns a path starting with "../"

    Returns the relative path of this to \a parent if this is a child of \a parent.
*/
FilePath FilePath::relativeChildPath(const FilePath &parent) const
{
    FilePath res;
    if (isChildOf(parent)) {
        QStringView p = pathView().mid(parent.pathView().size());
        if (p.startsWith('/'))
            p = p.mid(1);
        res.setParts(scheme(), host(), p);
    }
    return res;
}

/*!
    Returns the relative path of FilePath from a given \a anchor.
    Both, FilePath and anchor may be files or directories.
    Example usage:

    \code
        FilePath filePath("/foo/b/ar/file.txt");
        FilePath relativePath = filePath.relativePathFrom("/foo/c");
        qDebug() << relativePath
    \endcode

    The debug output will be "../b/ar/file.txt".
*/
FilePath FilePath::relativePathFrom(const FilePath &anchor) const
{
    QTC_ASSERT(isSameDevice(anchor), return *this);

    FilePath absPath;
    QString filename;
    if (isFile()) {
        absPath = absolutePath();
        filename = fileName();
    } else if (isDir()) {
        absPath = absoluteFilePath();
    } else {
        return {};
    }
    FilePath absoluteAnchorPath;
    if (anchor.isFile())
        absoluteAnchorPath = anchor.absolutePath();
    else if (anchor.isDir())
        absoluteAnchorPath = anchor.absoluteFilePath();
    else
        return {};
    QString relativeFilePath = calcRelativePath(absPath.path(), absoluteAnchorPath.path());
    if (!filename.isEmpty()) {
        if (relativeFilePath == ".")
            relativeFilePath.clear();
        if (!relativeFilePath.isEmpty())
            relativeFilePath += '/';
        relativeFilePath += filename;
    }
    return FilePath::fromString(relativeFilePath);
}

/*!
    Returns the relative path of \a absolutePath to given \a absoluteAnchorPath.
    Both paths must be an absolute path to a directory.

    Example usage:

    \code
        qDebug() << FilePath::calcRelativePath("/foo/b/ar", "/foo/c");
    \endcode

    The debug output will be "../b/ar".

    \see FilePath::isRelativePath(), FilePath::relativePathFrom(), FilePath::relativeChildPath()
*/
QString FilePath::calcRelativePath(const QString &absolutePath, const QString &absoluteAnchorPath)
{
    if (absolutePath.isEmpty() || absoluteAnchorPath.isEmpty())
        return QString();
    // TODO using split() instead of parsing the strings by char index is slow
    // and needs more memory (but the easiest implementation for now)
    const QStringList splits1 = absolutePath.split('/');
    const QStringList splits2 = absoluteAnchorPath.split('/');
    int i = 0;
    while (i < splits1.count() && i < splits2.count() && splits1.at(i) == splits2.at(i))
        ++i;
    QString relativePath;
    int j = i;
    bool addslash = false;
    while (j < splits2.count()) {
        if (!splits2.at(j).isEmpty()) {
            if (addslash)
                relativePath += '/';
            relativePath += u"..";
            addslash = true;
        }
        ++j;
    }
    while (i < splits1.count()) {
        if (!splits1.at(i).isEmpty()) {
            if (addslash)
                relativePath += '/';
            relativePath += splits1.at(i);
            addslash = true;
        }
        ++i;
    }
    if (relativePath.isEmpty())
        return QString(".");
    return relativePath;
}

/*!
    Returns a path corresponding to \a newPath object on the
    same device as the current object.

    This may involve device-specific translations like converting
    windows style paths to unix style paths with suitable file
    system case or handling of drive letters: C:/dev/src -> /c/dev/src

    Example usage:
    \code
        localDir = FilePath("/tmp/workingdir");
        executable = FilePath::fromUrl("docker://123/bin/ls")
        realDir = executable.withNewMappedPath(localDir)
        assert(realDir == FilePath::fromUrl("docker://123/tmp/workingdir"))
    \endcode
*/
FilePath FilePath::withNewMappedPath(const FilePath &newPath) const
{
    FilePath res;
    res.setParts(scheme(), host(), fileAccess()->mapToDevicePath(newPath.path()));
    return res;
}

/*!
    Returns a FilePath with local path \a newPath on the same device
    as the current object.

    Example usage:
    \code
        devicePath = FilePath("docker://123/tmp");
        newPath = devicePath.withNewPath("/bin/ls");
        assert(realDir == FilePath::fromUrl("docker://123/bin/ls"))
    \endcode
*/
FilePath FilePath::withNewPath(const QString &newPath) const
{
    FilePath res;
    res.setParts(scheme(), host(), newPath);
    return res;
}

/*!
    Search for a binary corresponding to this object on each directory entry
    specified by \a dirs matching the \a filter with the \a matchScope of the
    file path.

    Example usage:
    \code
        binary = FilePath::fromUrl("docker://123/./make);
        fullPath = binary.searchInDirectories(binary.deviceEnvironment().path());
        assert(fullPath == FilePath::fromUrl("docker://123/usr/bin/make"))
    \endcode
*/

FilePath FilePath::searchInDirectories(const FilePaths &dirs,
                                       const FilePathPredicate &filter,
                                       MatchScope matchScope) const
{
    if (isEmpty())
        return {};

    const FilePaths execs = appendExeExtensions(*this, matchScope);

    if (isAbsolutePath()) {
        for (const FilePath &filePath : execs) {
            if (filePath.isExecutableFile() && (!filter || filter(filePath)))
                return filePath;
        }
        return {};
    }

    QSet<FilePath> alreadyCheckedDirectories;

    for (const FilePath &dir : dirs) {
        // Compare the initial size of the set with the size after insertion to check
        // if the directory was already checked.
        const int initialCount = alreadyCheckedDirectories.count();
        alreadyCheckedDirectories.insert(dir);
        const bool wasAlreadyChecked = alreadyCheckedDirectories.count() == initialCount;

        if (dir.isEmpty() || wasAlreadyChecked)
            continue;

        for (const FilePath &exe : execs) {
            const FilePath filePath = dir / exe.path();
            if (filePath.isExecutableFile() && (!filter || filter(filePath)))
                return filePath;
        }
    }

    return {};
}

FilePaths FilePath::searchAllInDirectories(const FilePaths &dirs,
                                           const FilePathPredicate &filter,
                                           MatchScope matchScope) const
{
    if (isEmpty())
        return {};

    const FilePaths execs = appendExeExtensions(*this, matchScope);

    FilePaths result;
    if (isAbsolutePath()) {
        for (const FilePath &filePath : execs) {
            if (filePath.isExecutableFile() && (!filter || filter(filePath)))
                result.append(filePath);
        }
        return result;
    }

    QSet<FilePath> alreadyCheckedDirectories;

    for (const FilePath &dir : dirs) {
        // Compare the initial size of the set with the size after insertion to check
        // if the directory was already checked.
        const int initialCount = alreadyCheckedDirectories.count();
        alreadyCheckedDirectories.insert(dir);
        const bool wasAlreadyChecked = alreadyCheckedDirectories.count() == initialCount;

        if (dir.isEmpty() || wasAlreadyChecked)
            continue;

        for (const FilePath &exe : execs) {
            const FilePath filePath = dir / exe.path();
            if (filePath.isExecutableFile() && (!filter || filter(filePath)))
                result.append(filePath);
        }
    }

    return result;
}

static FilePaths dirsFromPath(const FilePath &anchor,
                              const FilePaths &additionalDirs,
                              FilePath::PathAmending amending)
{
    FilePaths directories = anchor.devicePathEnvironmentVariable();

    if (!additionalDirs.isEmpty()) {
        if (amending == FilePath::AppendToPath)
            directories.append(additionalDirs);
        else
            directories = additionalDirs + directories;
    }

    return directories;
}

FilePath FilePath::searchInPath(const FilePaths &additionalDirs,
                                PathAmending amending,
                                const FilePathPredicate &filter,
                                MatchScope matchScope) const
{
    if (isAbsolutePath())
        return *this;

    const FilePaths directories = dirsFromPath(*this, additionalDirs, amending);
    return searchInDirectories(directories, filter, matchScope);
}

FilePaths FilePath::searchAllInPath(const FilePaths &additionalDirs,
                                    PathAmending amending,
                                    const FilePathPredicate &filter,
                                    MatchScope matchScope) const
{
    const FilePaths directories = dirsFromPath(*this, additionalDirs, amending);
    return searchAllInDirectories(directories, filter, matchScope);
}

Environment FilePath::deviceEnvironment() const
{
    expected_str<Environment> env = deviceEnvironmentWithError();
    QTC_ASSERT_EXPECTED(env, return {});
    return *env;
}

expected_str<Environment> FilePath::deviceEnvironmentWithError() const
{
    if (needsDevice()) {
        QTC_ASSERT(s_deviceHooks.environment, return {});
        return s_deviceHooks.environment(*this);
    }
    return Environment::systemEnvironment();
}

FilePaths FilePath::devicePathEnvironmentVariable() const
{
    FilePaths result = deviceEnvironment().path();
    if (needsDevice()) {
        for (FilePath &dir : result)
            dir.setParts(this->scheme(), this->host(), dir.path());
    }
    return result;
}

QString FilePath::formatFilePaths(const FilePaths &files, const QString &separator)
{
    const QStringList nativeFiles = transform(files, &FilePath::toUserOutput);
    return nativeFiles.join(separator);
}

void FilePath::removeDuplicates(FilePaths &files)
{
    // FIXME: Improve.
    // FIXME: This drops the osType information, which is not correct.
    QStringList list = transform<QStringList>(files, &FilePath::toString);
    list.removeDuplicates();
    files = FileUtils::toFilePathList(list);
}

void FilePath::sort(FilePaths &files)
{
    std::sort(files.begin(), files.end(), [](const FilePath &a, const FilePath &b) {
        const int scheme = a.scheme().compare(b.scheme());
        if (scheme != 0)
            return scheme < 0;

        const int host = a.host().compare(b.host());
        if (host != 0)
            return host < 0;

        return a.pathView() < b.pathView();
    });
}

void join(QString &left, const QString &right)
{
    QStringView r(right);
    if (r.startsWith('/'))
        r = r.mid(1);

    if (!left.endsWith('/'))
        left += '/';
    left += r;
}

FilePath FilePath::pathAppended(const QString &path) const
{
    if (path.isEmpty())
        return *this;

    QString other = path;
    other.replace('\\', '/');

    if (isEmpty())
        return FilePath::fromString(other);

    QString p = this->path();
    join(p, other);

    return withNewPath(p);
}

FilePath FilePath::stringAppended(const QString &str) const
{
    return FilePath::fromString(toString() + str);
}

std::optional<FilePath> FilePath::tailRemoved(const QString &str) const
{
    if (pathView().endsWith(str))
        return withNewPath(pathView().chopped(str.size()).toString());
    return {};
}

QDateTime FilePath::lastModified() const
{
    return fileAccess()->lastModified(*this);
}

QFile::Permissions FilePath::permissions() const
{
    return fileAccess()->permissions(*this);
}

bool FilePath::setPermissions(QFile::Permissions permissions) const
{
    return fileAccess()->setPermissions(*this, permissions);
}

OsType FilePath::osType() const
{
    if (!needsDevice())
        return HostOsInfo::hostOs();

    QTC_ASSERT(s_deviceHooks.osType, return HostOsInfo::hostOs());
    return s_deviceHooks.osType(*this);
}

bool FilePath::removeFile() const
{
    return fileAccess()->removeFile(*this);
}

/*!
    Removes the directory this filePath refers too and its subdirectories recursively.

    \note The \a error parameter is optional.

    Returns a Bool indicating whether the operation succeeded.
*/
bool FilePath::removeRecursively(QString *error) const
{
    return fileAccess()->removeRecursively(*this, error);
}

expected_str<void> FilePath::copyRecursively(const FilePath &target) const
{
    return fileAccess()->copyRecursively(*this, target);
}

expected_str<void> FilePath::copyFile(const FilePath &target) const
{
    if (!isSameDevice(target)) {
        // FIXME: This does not scale.
        const expected_str<QByteArray> contents = fileContents();
        if (!contents) {
            return make_unexpected(
                Tr::tr("Error while trying to copy file: %1").arg(contents.error()));
        }

        const QFile::Permissions perms = permissions();
        const expected_str<qint64> copyResult = target.writeFileContents(*contents);

        if (!copyResult)
            return make_unexpected(Tr::tr("Could not copy file: %1").arg(copyResult.error()));

        if (!target.setPermissions(perms)) {
            target.removeFile();
            return make_unexpected(
                Tr::tr("Could not set permissions on \"%1\"").arg(target.toString()));
        }

        return {};
    }
    return fileAccess()->copyFile(*this, target);
}

bool FilePath::renameFile(const FilePath &target) const
{
    return fileAccess()->renameFile(*this, target);
}

qint64 FilePath::fileSize() const
{
    return fileAccess()->fileSize(*this);
}

qint64 FilePath::bytesAvailable() const
{
    return fileAccess()->bytesAvailable(*this);
}

/*!
    \brief Checks if this is newer than \a timeStamp.

    The time stamp \a timeStamp to compare with.
    Returns \c true if this is newer than \a timeStamp.
    If this is a directory, the function will recursively check all files and return
    \c true if one of them is newer than \a timeStamp. If this is a single file, \c true will
    be returned if the file is newer than \a timeStamp.

     Returns whether at least one file in the file path has a newer date than
     \a timeStamp.
*/
bool FilePath::isNewerThan(const QDateTime &timeStamp) const
{
    if (!exists() || lastModified() >= timeStamp)
        return true;
    if (isDir()) {
        const FilePaths dirContents = dirEntries(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
        for (const FilePath &entry : dirContents) {
            if (entry.isNewerThan(timeStamp))
                return true;
        }
    }
    return false;
}

/*!
    \brief Returns the caseSensitivity of the path.

    This is currently only based on the Host OS.
    For device paths, \c Qt::CaseSensitive is always returned.
*/
Qt::CaseSensitivity FilePath::caseSensitivity() const
{
    if (m_schemeLen == 0)
        return HostOsInfo::fileNameCaseSensitivity();

    // FIXME: This could or possibly should the target device's file name case sensitivity
    // into account by diverting to IDevice. However, as this is expensive and we are
    // in time-critical path here, we go with "good enough" for now:
    // The first approximation is "most things are case-sensitive".
    return Qt::CaseSensitive;
}

/*!
    \brief Returns the separator of path components for this path.

    Returns the path separator of the path.
*/
QChar FilePath::pathComponentSeparator() const
{
    return osType() == OsTypeWindows ? u'\\' : u'/';
}

/*!
    \brief Returns the path list separator for the device this path belongs to.

    Returns the path list separator of the device for this path.
*/
QChar FilePath::pathListSeparator() const
{
    return osType() == OsTypeWindows ? u';' : u':';
}

/*!
    \brief Recursively resolves symlinks if this is a symlink.

    To resolve symlinks anywhere in the path, see canonicalPath.
    Unlike QFileInfo::canonicalFilePath(), this function will still return the expected deepest
    target file even if the symlink is dangling.

    \note Maximum recursion depth == 16.

    Returns the symlink target file path.
*/
FilePath FilePath::resolveSymlinks() const
{
    FilePath current = *this;
    int links = 16;
    while (links--) {
        const FilePath target = current.symLinkTarget();
        if (target.isEmpty())
            return current;
        current = target;
    }
    return current;
}

/*!
    Recursively resolves possibly present symlinks in this file name.

    On Windows, also resolves SUBST and re-mounted NTFS drives.
    Unlike QFileInfo::canonicalFilePath(), this function will not return an empty
    string if path doesn't exist.

    Returns the canonical path.
*/
FilePath FilePath::canonicalPath() const
{
    if (needsDevice()) {
        // FIXME: Not a full solution, but it stays on the right device.
        return *this;
    }

#ifdef Q_OS_WIN
    DWORD flagsAndAttrs = FILE_ATTRIBUTE_NORMAL;
    if (isDir())
        flagsAndAttrs |= FILE_FLAG_BACKUP_SEMANTICS;
    const HANDLE fileHandle = CreateFile(
                toUserOutput().toStdWString().c_str(),
                GENERIC_READ,
                FILE_SHARE_READ,
                nullptr,
                OPEN_EXISTING,
                flagsAndAttrs,
                nullptr);
    if (fileHandle != INVALID_HANDLE_VALUE) {
        TCHAR normalizedPath[MAX_PATH];
        const auto length = GetFinalPathNameByHandleW(
                    fileHandle,
                    normalizedPath,
                    MAX_PATH,
                    FILE_NAME_NORMALIZED);
        CloseHandle(fileHandle);
        if (length > 0)
            return fromUserInput(QString::fromStdWString(std::wstring(normalizedPath, length)));
    }
#endif

    const QString result = toFileInfo().canonicalFilePath();
    if (!result.isEmpty())
        return fromString(result);

    return *this;
}

FilePath FilePath::operator/(const QString &str) const
{
    return pathAppended(str);
}

/*!
    \brief Clears all parts of the FilePath.
*/
void FilePath::clear()
{
    *this = {};
}

/*!
    \brief Checks if the path() is empty.

    Returns true if the path() is empty.
    The Host and Scheme of the part are ignored.
*/
bool FilePath::isEmpty() const
{
    return m_pathLen == 0;
}

/*!
    \brief Converts the path to a possibly shortened path with native separators.

    Like QDir::toNativeSeparators(), but use prefix '~' instead of $HOME on unix systems when an
    absolute path is given.

    Returns the possibly shortened path with native separators.
*/
QString FilePath::shortNativePath() const
{
    if (HostOsInfo::isAnyUnixHost()) {
        const FilePath home = FileUtils::homePath();
        if (isChildOf(home)) {
            return QLatin1Char('~') + QDir::separator()
                + QDir::toNativeSeparators(relativeChildPath(home).toString());
        }
    }
    return toUserOutput();
}

/*!
    \brief Checks whether the path is relative.

    Returns true if the path is relative.
*/
bool FilePath::isRelativePath() const
{
    const QStringView p = pathView();
    if (p.startsWith('/'))
        return false;
    if (startsWithWindowsDriveLetterAndSlash(p))
        return false;
    if (p.startsWith(u":/")) // QRC
        return false;
    return true;
}

/*!
    \brief Appends the \a tail to this, if the tail is a relative path.

    Returns the tail if the tail is absolute, otherwise this + tail.
*/
FilePath FilePath::resolvePath(const FilePath &tail) const
{
    if (tail.isEmpty())
        return cleanPath();
    if (tail.isRelativePath())
        return pathAppended(tail.path()).cleanPath();
    return tail;
}

/*!
    \brief Appends the \a tail to this, if the tail is a relative path.

    Returns the tail if the tail is absolute, otherwise this + tail.
*/
FilePath FilePath::resolvePath(const QString &tail) const
{
   return resolvePath(FilePath::fromUserInput(tail));
}

expected_str<FilePath> FilePath::localSource() const
{
    if (!needsDevice())
        return *this;

    QTC_ASSERT(s_deviceHooks.localSource,
               return make_unexpected(Tr::tr("No \"localSource\" device hook set.")));
    return s_deviceHooks.localSource(*this);
}

/*!
    \brief Cleans path part similar to \c QDir::cleanPath().

    \list
    \li directory separators normalized (that is, platform-native
     separators converted to "/") and redundant ones removed, and "."s and ".."s
      resolved (as far as possible).
    \li Symbolic links are kept. This function does not return the
      canonical path, but rather the simplest version of the input.
      For example, "./local" becomes "local", "local/../bin" becomes
      "bin" and "/local/usr/../bin" becomes "/local/bin".
    \endlist
*/
FilePath FilePath::cleanPath() const
{
    return withNewPath(doCleanPath(path()));
}

/*!
    On Linux/Mac replace user's home path with ~ in the \c toString()
    result for this path after cleaning.

    If path is not sub of home path, or when running on Windows, returns the input
*/
QString FilePath::withTildeHomePath() const
{
    if (osType() == OsTypeWindows)
        return toString();

    if (needsDevice())
        return toString();

    static const QString homePath = QDir::homePath();

    QString outPath = cleanPath().absoluteFilePath().path();
    if (outPath.startsWith(homePath))
       return '~' + outPath.mid(homePath.size());

    return toString();
}

QTextStream &operator<<(QTextStream &s, const FilePath &fn)
{
    return s << fn.toString();
}

// FileFilter
FileFilter::FileFilter(const QStringList &nameFilters,
                       const QDir::Filters fileFilters,
                       const QDirIterator::IteratorFlags flags)
    : nameFilters(nameFilters),
      fileFilters(fileFilters),
      iteratorFlags(flags)
{
    QTC_CHECK(this->fileFilters != QDir::Filters());
}

QStringList FileFilter::asFindArguments(const QString &path) const
{
    QStringList arguments;

    const QDir::Filters filters = fileFilters;

    if (iteratorFlags.testFlag(QDirIterator::FollowSymlinks))
        arguments << "-L";
    else
        arguments << "-H";

    arguments << path;

    if (!iteratorFlags.testFlag(QDirIterator::Subdirectories))
        arguments.append({"-maxdepth", "1"});

    QStringList filterOptions;

    if (!(filters & QDir::Hidden))
        filterOptions << "!" << "-name" << ".*";

    QStringList filterFilesAndDirs;
    if (filters.testFlag(QDir::Dirs))
        filterFilesAndDirs << "-type" << "d";
    if (filters.testFlag(QDir::Files)) {
        if (!filterFilesAndDirs.isEmpty())
            filterFilesAndDirs << "-o";
        filterFilesAndDirs << "-type" << "f";
    }

    if (!filters.testFlag(QDir::NoSymLinks)) {
        if (!filterFilesAndDirs.isEmpty())
            filterFilesAndDirs << "-o";
        filterFilesAndDirs << "-type" << "l";
    }

    if (!filterFilesAndDirs.isEmpty())
        filterOptions << "(" << filterFilesAndDirs << ")";

    QStringList accessOptions;
    if (filters & QDir::Readable)
        accessOptions << "-readable";
    if (filters & QDir::Writable) {
        if (!accessOptions.isEmpty())
            accessOptions << "-o";
        accessOptions << "-writable";
    }
    if (filters & QDir::Executable) {
        if (!accessOptions.isEmpty())
            accessOptions << "-o";
        accessOptions << "-executable";
    }

    if (!accessOptions.isEmpty())
        filterOptions << "(" << accessOptions << ")";

    QTC_CHECK(filters ^ QDir::AllDirs);
    QTC_CHECK(filters ^ QDir::Drives);
    QTC_CHECK(filters ^ QDir::NoDot);
    QTC_CHECK(filters ^ QDir::NoDotDot);
    QTC_CHECK(filters ^ QDir::System);

    const QString nameOption = (filters & QDir::CaseSensitive) ? QString{"-name"}
                                                               : QString{"-iname"};
    if (!nameFilters.isEmpty()) {
        bool isFirst = true;
        filterOptions << "(";
        for (const QString &current : nameFilters) {
            if (!isFirst)
                filterOptions << "-o";
            filterOptions << nameOption << current;
            isFirst = false;
        }
        filterOptions << ")";
    }
    arguments << filterOptions;
    return arguments;
}

DeviceFileHooks &DeviceFileHooks::instance()
{
    return s_deviceHooks;
}

QTCREATOR_UTILS_EXPORT bool operator==(const FilePath &first, const FilePath &second)
{
    if (first.m_hash != 0 && second.m_hash != 0 && first.m_hash != second.m_hash)
        return false;

    return first.pathView().compare(second.pathView(), first.caseSensitivity()) == 0
        && first.host() == second.host()
        && first.scheme() == second.scheme();
}

QTCREATOR_UTILS_EXPORT bool operator!=(const FilePath &first, const FilePath &second)
{
    return !(first == second);
}

QTCREATOR_UTILS_EXPORT bool operator<(const FilePath &first, const FilePath &second)
{
    const int cmp = first.pathView().compare(second.pathView(), first.caseSensitivity());
    if (cmp != 0)
        return cmp < 0;
    if (first.host() != second.host())
        return first.host() < second.host();
    return first.scheme() < second.scheme();
}

QTCREATOR_UTILS_EXPORT bool operator<=(const FilePath &first, const FilePath &second)
{
    return !(second < first);
}

QTCREATOR_UTILS_EXPORT bool operator>(const FilePath &first, const FilePath &second)
{
    return second < first;
}

QTCREATOR_UTILS_EXPORT bool operator>=(const FilePath &first, const FilePath &second)
{
    return !(first < second);
}

QTCREATOR_UTILS_EXPORT size_t qHash(const FilePath &filePath, uint seed)
{
    Q_UNUSED(seed);

    if (filePath.m_hash == 0) {
        if (filePath.caseSensitivity() == Qt::CaseSensitive)
            filePath.m_hash = qHash(QStringView(filePath.m_data), 0);
        else
            filePath.m_hash = qHash(filePath.m_data.toCaseFolded(), 0);
    }

    return filePath.m_hash;
}

QTCREATOR_UTILS_EXPORT size_t qHash(const FilePath &filePath)
{
    return qHash(filePath, 0);
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug dbg, const FilePath &c)
{
    return dbg << c.toString();
}

} // Utils
