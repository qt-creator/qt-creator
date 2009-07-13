
#ifdef Q_WS_WIN
# include "QtCore/qt_windows.h"
#endif
#include "QtCore/qlibrary.h"
#include "QtCore/qpointer.h"
#include "QtCore/qstringlist.h"
#include "QtCore/qplugin.h"


#include "qplatformdefs.h"

#include "qplugin.h"
#include "qpluginloader.h"
#include <qfileinfo.h>
#include "qdebug.h"

#include "qplatformdefs.h"
#include "qlibrary.h"

#include <qstringlist.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qmutex.h>
#include <qmap.h>
#include <qsettings.h>
#include <qdatetime.h>
#include <qcoreapplication.h>
#include <qvector.h>
#include <qdir.h>


#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif
#ifndef NO_ERRNO_H
#include <errno.h>
#endif // NO_ERROR_H

#include "qplatformdefs.h"

#ifdef Q_OS_MAC
#  include <private/qcore_mac_p.h>
#endif

#if defined(QT_AOUT_UNDERSCORE)
#include <string.h>
#endif


#if !defined(QT_HPUX_LD)
#include <dlfcn.h>
#endif

QT_BEGIN_NAMESPACE

bool qt_debug_component();

class PatchedLibraryPrivate
{
public:

#ifdef Q_WS_WIN
    HINSTANCE
#else
    void *
#endif
    pHnd;

    QString fileName, qualifiedFileName;
    QString fullVersion;

    bool load();
    bool loadPlugin(); // loads and resolves instance
    bool unload();
    void release();
    void *resolve(const char *);

    static PatchedLibraryPrivate *findOrCreate(const QString &fileName, const QString &version = QString());

    QtPluginInstanceFunction instance;
    uint qt_version;
    QString lastModified;

    QString errorString;
    QLibrary::LoadHints loadHints;

    bool isPlugin(QSettings *settings = 0);


private:
    explicit PatchedLibraryPrivate(const QString &canonicalFileName, const QString &version);
    ~PatchedLibraryPrivate();

    bool load_sys();
    bool unload_sys();
    void *resolve_sys(const char *);

    QAtomicInt libraryRefCount;
    QAtomicInt libraryUnloadCount;

    enum {IsAPlugin, IsNotAPlugin, MightBeAPlugin } pluginState;
    friend class PatchedLibraryPrivateHasFriends;
};

class PatchedPluginLoader
{
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName)
    Q_PROPERTY(QLibrary::LoadHints loadHints READ loadHints WRITE setLoadHints)
public:
    explicit PatchedPluginLoader(QObject *parent = 0);
    explicit PatchedPluginLoader(const QString &fileName, QObject *parent = 0);
    ~PatchedPluginLoader();

    QObject *instance();

    bool load();
    bool unload();
    bool isLoaded() const;

    void setFileName(const QString &fileName);
    QString fileName() const;

    QString errorString() const;

    void setLoadHints(QLibrary::LoadHints loadHints);
    QLibrary::LoadHints loadHints() const;

private:
    PatchedLibraryPrivate *d;
    bool did_load;
    Q_DISABLE_COPY(PatchedPluginLoader)
};

//#define QT_DEBUG_COMPONENT

#ifdef QT_NO_DEBUG
#  define QLIBRARY_AS_DEBUG false
#else
#  define QLIBRARY_AS_DEBUG true
#endif

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
// We don't use separate debug and release libs on UNIX, so we want
// to allow loading plugins, regardless of how they were built.
#  define QT_NO_DEBUG_PLUGIN_CHECK
#endif

Q_GLOBAL_STATIC(QMutex, patched_qt_library_mutex)


#ifndef QT_NO_PLUGIN_CHECK
struct qt_token_info
{
    qt_token_info(const char *f, const ulong fc)
        : fields(f), field_count(fc), results(fc), lengths(fc)
    {
        results.fill(0);
        lengths.fill(0);
    }

    const char *fields;
    const ulong field_count;

    QVector<const char *> results;
    QVector<ulong> lengths;
};

/*
  return values:
       1 parse ok
       0 eos
      -1 parse error
*/
static int qt_tokenize(const char *s, ulong s_len, ulong *advance,
                        qt_token_info &token_info)
{
    ulong pos = 0, field = 0, fieldlen = 0;
    char current;
    int ret = -1;
    *advance = 0;
    for (;;) {
        current = s[pos];

        // next char
        ++pos;
        ++fieldlen;
        ++*advance;

        if (! current || pos == s_len + 1) {
            // save result
            token_info.results[(int)field] = s;
            token_info.lengths[(int)field] = fieldlen - 1;

            // end of string
            ret = 0;
            break;
        }

        if (current == token_info.fields[field]) {
            // save result
            token_info.results[(int)field] = s;
            token_info.lengths[(int)field] = fieldlen - 1;

            // end of field
            fieldlen = 0;
            ++field;
            if (field == token_info.field_count - 1) {
                // parse ok
                ret = 1;
            }
            if (field == token_info.field_count) {
                // done parsing
                break;
            }

            // reset string and its length
            s = s + pos;
            s_len -= pos;
            pos = 0;
        }
    }

    return ret;
}

/*
  returns true if the string s was correctly parsed, false otherwise.
*/
static bool qt_parse_pattern(const char *s, uint *version, bool *debug, QByteArray *key)
{
    bool ret = true;

    qt_token_info pinfo("=\n", 2);
    int parse;
    ulong at = 0, advance, parselen = qstrlen(s);
    do {
        parse = qt_tokenize(s + at, parselen, &advance, pinfo);
        if (parse == -1) {
            ret = false;
            break;
        }

        at += advance;
        parselen -= advance;

        if (qstrncmp("version", pinfo.results[0], pinfo.lengths[0]) == 0) {
            // parse version string
            qt_token_info pinfo2("..-", 3);
            if (qt_tokenize(pinfo.results[1], pinfo.lengths[1],
                              &advance, pinfo2) != -1) {
                QByteArray m(pinfo2.results[0], pinfo2.lengths[0]);
                QByteArray n(pinfo2.results[1], pinfo2.lengths[1]);
                QByteArray p(pinfo2.results[2], pinfo2.lengths[2]);
                *version  = (m.toUInt() << 16) | (n.toUInt() << 8) | p.toUInt();
            } else {
                ret = false;
                break;
            }
        } else if (qstrncmp("debug", pinfo.results[0], pinfo.lengths[0]) == 0) {
            *debug = qstrncmp("true", pinfo.results[1], pinfo.lengths[1]) == 0;
        } else if (qstrncmp("buildkey", pinfo.results[0],
                              pinfo.lengths[0]) == 0){
            // save buildkey
            *key = QByteArray(pinfo.results[1], pinfo.lengths[1]);
        }
    } while (parse == 1 && parselen > 0);

    return ret;
}
#endif // QT_NO_PLUGIN_CHECK

#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC) && !defined(QT_NO_PLUGIN_CHECK)

#if defined(Q_OS_FREEBSD) || defined(Q_OS_LINUX)
#  define USE_MMAP
QT_BEGIN_INCLUDE_NAMESPACE
#  include <sys/types.h>
#  include <sys/mman.h>
QT_END_INCLUDE_NAMESPACE
#endif // Q_OS_FREEBSD || Q_OS_LINUX

static long qt_find_pattern(const char *s, ulong s_len,
                             const char *pattern, ulong p_len)
{
    /*
      we search from the end of the file because on the supported
      systems, the read-only data/text segments are placed at the end
      of the file.  HOWEVER, when building with debugging enabled, all
      the debug symbols are placed AFTER the data/text segments.

      what does this mean?  when building in release mode, the search
      is fast because the data we are looking for is at the end of the
      file... when building in debug mode, the search is slower
      because we have to skip over all the debugging symbols first
    */

    if (! s || ! pattern || p_len > s_len) return -1;
    ulong i, hs = 0, hp = 0, delta = s_len - p_len;

    for (i = 0; i < p_len; ++i) {
        hs += s[delta + i];
        hp += pattern[i];
    }
    i = delta;
    for (;;) {
        if (hs == hp && qstrncmp(s + i, pattern, p_len) == 0)
            return i;
        if (i == 0)
            break;
        --i;
        hs -= s[i + p_len];
        hs += s[i];
    }

    return -1;
}

/*
  This opens the specified library, mmaps it into memory, and searches
  for the QT_PLUGIN_VERIFICATION_DATA.  The advantage of this approach is that
  we can get the verification data without have to actually load the library.
  This lets us detect mismatches more safely.

  Returns false if version/key information is not present, or if the
                information could not be read.
  Returns  true if version/key information is present and successfully read.
*/
static bool qt_unix_query(const QString &library, uint *version, bool *debug, QByteArray *key, PatchedLibraryPrivate *lib = 0)
{
    QFile file(library);
    if (!file.open(QIODevice::ReadOnly)) {
        if (lib)
            lib->errorString = file.errorString();
        if (qt_debug_component()) {
            qWarning("%s: %s", (const char*) QFile::encodeName(library),
                qPrintable(qt_error_string(errno)));
        }
        return false;
    }

    QByteArray data;
    char *filedata = 0;
    ulong fdlen = 0;

#ifdef USE_MMAP
    char *mapaddr = 0;
    size_t maplen = file.size();
    mapaddr = (char *) mmap(mapaddr, maplen, PROT_READ, MAP_PRIVATE, file.handle(), 0);
    if (mapaddr != MAP_FAILED) {
        // mmap succeeded
        filedata = mapaddr;
        fdlen = maplen;
    } else {
        // mmap failed
        if (qt_debug_component()) {
            qWarning("mmap: %s", qPrintable(qt_error_string(errno)));
        }
        if (lib)
            lib->errorString = QLibrary::tr("Could not mmap '%1': %2")
                .arg(library)
                .arg(qt_error_string());
#endif // USE_MMAP
        // try reading the data into memory instead
        data = file.readAll();
        filedata = data.data();
        fdlen = data.size();
#ifdef USE_MMAP
    }
#endif // USE_MMAP

    // verify that the pattern is present in the plugin
    const char pattern[] = "pattern=QT_PLUGIN_VERIFICATION_DATA";
    const ulong plen = qstrlen(pattern);
    long pos = qt_find_pattern(filedata, fdlen, pattern, plen);

    bool ret = false;
    if (pos >= 0)
        ret = qt_parse_pattern(filedata + pos, version, debug, key);

    if (!ret && lib)
        lib->errorString = QLibrary::tr("Plugin verification data mismatch in '%1'").arg(library);
#ifdef USE_MMAP
    if (mapaddr != MAP_FAILED && munmap(mapaddr, maplen) != 0) {
        if (qt_debug_component())
            qWarning("munmap: %s", qPrintable(qt_error_string(errno)));
        if (lib)
            lib->errorString = QLibrary::tr("Could not unmap '%1': %2")
                .arg(library)
                .arg( qt_error_string() );
    }
#endif // USE_MMAP

    file.close();
    return ret;
}

#endif // Q_OS_UNIX && !Q_OS_MAC && !defined(QT_NO_PLUGIN_CHECK)

typedef QMap<QString, PatchedLibraryPrivate*> LibraryMap;
Q_GLOBAL_STATIC(LibraryMap, libraryMap)

PatchedLibraryPrivate::PatchedLibraryPrivate(const QString &canonicalFileName, const QString &version)
    :pHnd(0), fileName(canonicalFileName), fullVersion(version), instance(0), qt_version(0),
     libraryRefCount(1), libraryUnloadCount(0), pluginState(MightBeAPlugin)
{ libraryMap()->insert(canonicalFileName, this); }

PatchedLibraryPrivate *PatchedLibraryPrivate::findOrCreate(const QString &fileName, const QString &version)
{
    QMutexLocker locker(patched_qt_library_mutex());
    if (PatchedLibraryPrivate *lib = libraryMap()->value(fileName)) {
        lib->libraryRefCount.ref();
        return lib;
    }

    return new PatchedLibraryPrivate(fileName, version);
}

PatchedLibraryPrivate::~PatchedLibraryPrivate()
{
    LibraryMap * const map = libraryMap();
    if (map) {
        PatchedLibraryPrivate *that = map->take(fileName);
        Q_ASSERT(this == that);
	Q_UNUSED(that)
    }
}

void *PatchedLibraryPrivate::resolve(const char *symbol)
{
    if (!pHnd)
        return 0;
    return resolve_sys(symbol);
}


bool PatchedLibraryPrivate::load()
{
    libraryUnloadCount.ref();
    if (pHnd)
        return true;
    if (fileName.isEmpty())
        return false;
    return load_sys();
}

bool PatchedLibraryPrivate::unload()
{
    if (!pHnd)
        return false;
    if (!libraryUnloadCount.deref()) { // only unload if ALL QLibrary instance wanted to
        if (instance)
            delete instance();
        if  (unload_sys()) {
            instance = 0;
            pHnd = 0;
        }
    }

    return (pHnd == 0);
}

void PatchedLibraryPrivate::release()
{
    QMutexLocker locker(patched_qt_library_mutex());
    if (!libraryRefCount.deref())
        delete this;
}

bool PatchedLibraryPrivate::loadPlugin()
{
    if (instance) {
        libraryUnloadCount.ref();
        return true;
    }
    if (load()) {
        instance = (QtPluginInstanceFunction)resolve("qt_plugin_instance");
        return instance;
    }
    return false;
}

/*!
    Returns true if \a fileName has a valid suffix for a loadable
    library; otherwise returns false.

    \table
    \header \i Platform \i Valid suffixes
    \row \i Windows     \i \c .dll
    \row \i Unix/Linux  \i \c .so
    \row \i AIX  \i \c .a
    \row \i HP-UX       \i \c .sl, \c .so (HP-UXi)
    \row \i Mac OS X    \i \c .dylib, \c .bundle, \c .so
    \endtable

    Trailing versioning numbers on Unix are ignored.
 */
bool QLibrary::isLibrary(const QString &fileName)
{
#if defined(Q_OS_WIN32) || defined(Q_OS_WINCE)
    return fileName.endsWith(QLatin1String(".dll"));
#else
    QString completeSuffix = QFileInfo(fileName).completeSuffix();
    if (completeSuffix.isEmpty())
        return false;
    QStringList suffixes = completeSuffix.split(QLatin1Char('.'));
# if defined(Q_OS_DARWIN)

    // On Mac, libs look like libmylib.1.0.0.dylib
    const QString lastSuffix = suffixes.at(suffixes.count() - 1);
    const QString firstSuffix = suffixes.at(0);

    bool valid = (lastSuffix == QLatin1String("dylib")
            || firstSuffix == QLatin1String("so")
            || firstSuffix == QLatin1String("bundle"));

    return valid;
# else  // Generic Unix
    QStringList validSuffixList;

#  if defined(Q_OS_HPUX)
/*
    See "HP-UX Linker and Libraries User's Guide", section "Link-time Differences between PA-RISC and IPF":
    "In PA-RISC (PA-32 and PA-64) shared libraries are suffixed with .sl. In IPF (32-bit and 64-bit),
    the shared libraries are suffixed with .so. For compatibility, the IPF linker also supports the .sl suffix."
 */
    validSuffixList << QLatin1String("sl");
#   if defined __ia64
    validSuffixList << QLatin1String("so");
#   endif
#  elif defined(Q_OS_AIX)
    validSuffixList << QLatin1String("a") << QLatin1String("so");
#  elif defined(Q_OS_UNIX)
    validSuffixList << QLatin1String("so");
#  endif

    // Examples of valid library names:
    //  libfoo.so
    //  libfoo.so.0
    //  libfoo.so.0.3
    //  libfoo-0.3.so
    //  libfoo-0.3.so.0.3.0

    int suffix;
    int suffixPos = -1;
    for (suffix = 0; suffix < validSuffixList.count() && suffixPos == -1; ++suffix)
        suffixPos = suffixes.indexOf(validSuffixList.at(suffix));

    bool valid = suffixPos != -1;
    for (int i = suffixPos + 1; i < suffixes.count() && valid; ++i)
        if (i != suffixPos)
            suffixes.at(i).toInt(&valid);
    return valid;
# endif
#endif

}

bool PatchedLibraryPrivate::isPlugin(QSettings *settings)
{
    errorString.clear();
    if (pluginState != MightBeAPlugin)
        return pluginState == IsAPlugin;

#ifndef QT_NO_PLUGIN_CHECK
    bool debug = !QLIBRARY_AS_DEBUG;
    QByteArray key;
    bool success = false;

    QFileInfo fileinfo(fileName);

#ifndef QT_NO_DATESTRING
    lastModified  = fileinfo.lastModified().toString(Qt::ISODate);
#endif
    QString regkey = QString::fromLatin1("Qt Plugin Cache %1.%2.%3/%4")
                     .arg((QT_VERSION & 0xff0000) >> 16)
                     .arg((QT_VERSION & 0xff00) >> 8)
                     .arg(QLIBRARY_AS_DEBUG ? QLatin1String("debug") : QLatin1String("false"))
                     .arg(fileName);
    QStringList reg;
#ifndef QT_NO_SETTINGS
    if (!settings) {
        static QSettings *staticSettings =
            new QSettings(QSettings::UserScope, QLatin1String("Trolltech"));
        settings = staticSettings;
    }
    reg = settings->value(regkey).toStringList();
#endif
    if (reg.count() == 4 && lastModified == reg.at(3)) {
        qt_version = reg.at(0).toUInt(0, 16);
        debug = bool(reg.at(1).toInt());
        key = reg.at(2).toLatin1();
        success = qt_version != 0;
    } else {
#if defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
        if (!pHnd) {
            // use unix shortcut to avoid loading the library
            success = qt_unix_query(fileName, &qt_version, &debug, &key, this);
        } else
#endif
        {
            bool temporary_load = false;
#ifdef Q_OS_WIN
            HMODULE hTempModule = 0;
#endif
            if (!pHnd) {
#ifdef Q_OS_WIN
                QT_WA({
                    hTempModule = ::LoadLibraryExW((wchar_t*)QDir::toNativeSeparators(fileName).utf16(), 0, DONT_RESOLVE_DLL_REFERENCES);
                } , {
                    temporary_load = load_sys();
                });
#else
                temporary_load =  load_sys();
#endif
            }
#  ifdef Q_CC_BOR
            typedef const char * __stdcall (*QtPluginQueryVerificationDataFunction)();
#  else
            typedef const char * (*QtPluginQueryVerificationDataFunction)();
#  endif
#ifdef Q_OS_WIN
            QtPluginQueryVerificationDataFunction qtPluginQueryVerificationDataFunction = hTempModule
                ? (QtPluginQueryVerificationDataFunction)
#ifdef Q_OS_WINCE
                    ::GetProcAddressW(hTempModule, L"qt_plugin_query_verification_data")
#else
                    ::GetProcAddress(hTempModule, "qt_plugin_query_verification_data")
#endif
                : (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_verification_data");
#else
            QtPluginQueryVerificationDataFunction qtPluginQueryVerificationDataFunction =
                (QtPluginQueryVerificationDataFunction) resolve("qt_plugin_query_verification_data");
#endif

            if (!qtPluginQueryVerificationDataFunction
                || !qt_parse_pattern(qtPluginQueryVerificationDataFunction(), &qt_version, &debug, &key)) {
                qt_version = 0;
                key = "unknown";
                if (temporary_load)
                    unload_sys();
            } else {
                success = true;
            }
#ifdef Q_OS_WIN
            if (hTempModule) {
                BOOL ok = ::FreeLibrary(hTempModule);
                if (ok) {
                    hTempModule = 0;
                }

            }
#endif
        }

        QStringList queried;
        queried << QString::number(qt_version,16)
                << QString::number((int)debug)
                << QLatin1String(key)
                << lastModified;
#ifndef QT_NO_SETTINGS
        settings->setValue(regkey, queried);
#endif
    }

    if (!success) {
        if (errorString.isEmpty()){
            if (fileName.isEmpty())
                errorString = QLibrary::tr("The shared library was not found.");
            else
                errorString = QLibrary::tr("The file '%1' is not a valid Qt plugin.").arg(fileName);
        }
        return false;
    }

    pluginState = IsNotAPlugin; // be pessimistic

    if ((qt_version > QT_VERSION) || ((QT_VERSION & 0xff0000) > (qt_version & 0xff0000))) {
        if (qt_debug_component()) {
            qWarning("In %s:\n"
                 "  Plugin uses incompatible Qt library (%d.%d.%d) [%s]",
                 (const char*) QFile::encodeName(fileName),
                 (qt_version&0xff0000) >> 16, (qt_version&0xff00) >> 8, qt_version&0xff,
                 debug ? "debug" : "release");
        }
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library. (%2.%3.%4) [%5]")
            .arg(fileName)
            .arg((qt_version&0xff0000) >> 16)
            .arg((qt_version&0xff00) >> 8)
            .arg(qt_version&0xff)
            .arg(debug ? QLatin1String("debug") : QLatin1String("release"));
    } else if (key != QT_BUILD_KEY
#ifdef QT_BUILD_KEY_COMPAT
               // be sure to load plugins using an older but compatible build key
               && key != QT_BUILD_KEY_COMPAT
#endif
               ) {
        if (qt_debug_component()) {
            qWarning("In %s:\n"
                 "  Plugin uses incompatible Qt library\n"
                 "  expected build key \"%s\", got \"%s\"",
                 (const char*) QFile::encodeName(fileName),
                 QT_BUILD_KEY,
                 key.isEmpty() ? "<null>" : (const char *) key);
        }
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " Expected build key \"%2\", got \"%3\"")
                 .arg(fileName)
                 .arg(QLatin1String(QT_BUILD_KEY))
                 .arg(key.isEmpty() ? QLatin1String("<null>") : QLatin1String((const char *) key));
#ifndef QT_NO_DEBUG_PLUGIN_CHECK
    } else if(debug != QLIBRARY_AS_DEBUG) {
        //don't issue a qWarning since we will hopefully find a non-debug? --Sam
        errorString = QLibrary::tr("The plugin '%1' uses incompatible Qt library."
                 " (Cannot mix debug and release libraries.)").arg(fileName);
#endif
    } else {
        pluginState = IsAPlugin;
    }

    return pluginState == IsAPlugin;
#else
    Q_UNUSED(settings)
    return pluginState == MightBeAPlugin;
#endif
}

/* Internal, for debugging */
bool qt_debug_component()
{
#if defined(QT_DEBUG_COMPONENT)
    return true;    //compatibility?
#else
    static int debug_env = -1;
    if (debug_env == -1)
       debug_env = QT_PREPEND_NAMESPACE(qgetenv)("QT_DEBUG_PLUGINS").toInt();

    return debug_env != 0;
#endif
}

PatchedPluginLoader::PatchedPluginLoader(QObject *)
    : d(0), did_load(false)
{
}

PatchedPluginLoader::PatchedPluginLoader(const QString &fileName, QObject *)
    : d(0), did_load(false)
{
    setFileName(fileName);
}

PatchedPluginLoader::~PatchedPluginLoader()
{
    if (d)
        d->release();
}

QObject *PatchedPluginLoader::instance()
{
    if (!load())
        return 0;
    if (d->instance)
        return d->instance();
    return 0;
}

bool PatchedPluginLoader::load()
{
    if (!d || d->fileName.isEmpty())
        return false;
    if (did_load)
        return d->pHnd && d->instance;
    if (!d->isPlugin())
        return false;
    did_load = true;
    return d->loadPlugin();
}

bool PatchedPluginLoader::unload()
{
    if (did_load) {
        did_load = false;
        return d->unload();
    }
    if (d)  // Ouch
        d->errorString = QLibrary::tr("The plugin was not loaded.");
    return false;
}

bool PatchedPluginLoader::isLoaded() const
{
    return d && d->pHnd && d->instance;
}

void PatchedPluginLoader::setFileName(const QString &fileName)
{
#if defined(QT_SHARED)
    QLibrary::LoadHints lh;
    if (d) {
        lh = d->loadHints;
        d->release();
        d = 0;
        did_load = false;
    }
    QString fn = QFileInfo(fileName).canonicalFilePath();
    d = PatchedLibraryPrivate::findOrCreate(fn);
    d->loadHints = lh;
    if (fn.isEmpty())
        d->errorString = QLibrary::tr("The shared library was not found.");
#else
    if (qt_debug_component()) {
        qWarning("Cannot load %s into a statically linked Qt library.",
            (const char*)QFile::encodeName(fileName));
    }
    Q_UNUSED(fileName)
#endif
}

QString PatchedPluginLoader::fileName() const
{
    if (d)
        return d->fileName;
    return QString();
}

QString PatchedPluginLoader::errorString() const
{
    return (!d || d->errorString.isEmpty()) ? QLibrary::tr("Unknown error") : d->errorString;
}

void PatchedPluginLoader::setLoadHints(QLibrary::LoadHints loadHints)
{
    if (!d) {
        d = PatchedLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        d->errorString.clear();
    }
    d->loadHints = loadHints;
}

QLibrary::LoadHints PatchedPluginLoader::loadHints() const
{
    if (!d) {
        PatchedPluginLoader *that = const_cast<PatchedPluginLoader *>(this);
        that->d = PatchedLibraryPrivate::findOrCreate(QString());   // ugly, but we need a d-ptr
        that->d->errorString.clear();
    }
    return d->loadHints;
}




#ifdef Q_OS_UNIX



static QString qdlerror()
{
#if !defined(QT_HPUX_LD)
    const char *err = dlerror();
#else
    const char *err = strerror(errno);
#endif
    return err ? QLatin1String("(")+QString::fromLocal8Bit(err) + QLatin1String(")"): QString();
}

bool PatchedLibraryPrivate::load_sys()
{
    QFileInfo fi(fileName);
    QString path = fi.path();
    QString name = fi.fileName();
    if (path == QLatin1String(".") && !fileName.startsWith(path))
        path.clear();
    else
        path += QLatin1Char('/');

    // The first filename we want to attempt to load is the filename as the callee specified.
    // Thus, the first attempt we do must be with an empty prefix and empty suffix.
    QStringList suffixes(QLatin1String("")), prefixes(QLatin1String(""));
    if (pluginState != IsAPlugin) {
        prefixes << QLatin1String("lib");
#if defined(Q_OS_HPUX)
        // according to
        // http://docs.hp.com/en/B2355-90968/linkerdifferencesiapa.htm

        // In PA-RISC (PA-32 and PA-64) shared libraries are suffixed
        // with .sl. In IPF (32-bit and 64-bit), the shared libraries
        // are suffixed with .so. For compatibility, the IPF linker
        // also supports the .sl suffix.

        // But since we don't know if we are built on HPUX or HPUXi,
        // we support both .sl (and .<version>) and .so suffixes but
        // .so is preferred.
# if defined(__ia64)
        if (!fullVersion.isEmpty()) {
            suffixes << QString::fromLatin1(".so.%1").arg(fullVersion);
        } else {
            suffixes << QLatin1String(".so");
        }
# endif
        if (!fullVersion.isEmpty()) {
            suffixes << QString::fromLatin1(".sl.%1").arg(fullVersion);
            suffixes << QString::fromLatin1(".%1").arg(fullVersion);
        } else {
            suffixes << QLatin1String(".sl");
        }
#elif defined(Q_OS_AIX)
        suffixes << ".a";
#else
        if (!fullVersion.isEmpty()) {
            suffixes << QString::fromLatin1(".so.%1").arg(fullVersion);
        } else {
            suffixes << QLatin1String(".so");
        }
#endif
# ifdef Q_OS_MAC
        if (!fullVersion.isEmpty()) {
            suffixes << QString::fromLatin1(".%1.bundle").arg(fullVersion);
            suffixes << QString::fromLatin1(".%1.dylib").arg(fullVersion);
        } else {
            suffixes << QLatin1String(".bundle") << QLatin1String(".dylib");
        }
#endif
    }
    int dlFlags = 0;
#if defined(QT_HPUX_LD)
    dlFlags = DYNAMIC_PATH | BIND_NONFATAL;
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= BIND_IMMEDIATE;
    } else {
        dlFlags |= BIND_DEFERRED;
    }
#else
    if (loadHints & QLibrary::ResolveAllSymbolsHint) {
        dlFlags |= RTLD_NOW;
    } else {
        dlFlags |= RTLD_LAZY;
    }
    if (loadHints & QLibrary::ExportExternalSymbolsHint) {
        dlFlags |= RTLD_GLOBAL;
    }
#if !defined(Q_OS_CYGWIN)
    else {
#if defined(Q_OS_MAC)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_4)
#endif        
        dlFlags |= RTLD_LOCAL;
    }
#endif
#if defined(Q_OS_AIX)	// Not sure if any other platform actually support this thing.
    if (loadHints & QLibrary::LoadArchiveMemberHint) {
        dlFlags |= RTLD_MEMBER;
    }
#endif
#endif // QT_HPUX_LD
    QString attempt;
    bool retry = true;
    for(int prefix = 0; retry && !pHnd && prefix < prefixes.size(); prefix++) {
        for(int suffix = 0; retry && !pHnd && suffix < suffixes.size(); suffix++) {
            if (!prefixes.at(prefix).isEmpty() && name.startsWith(prefixes.at(prefix)))
                continue;
            if (!suffixes.at(suffix).isEmpty() && name.endsWith(suffixes.at(suffix)))
                continue;
            if (loadHints & QLibrary::LoadArchiveMemberHint) {
                attempt = name;
                int lparen = attempt.indexOf(QLatin1Char('('));
                if (lparen == -1)
                    lparen = attempt.count();
                attempt = path + prefixes.at(prefix) + attempt.insert(lparen, suffixes.at(suffix));
            } else {
                attempt = path + prefixes.at(prefix) + name + suffixes.at(suffix);
            }
#if defined(QT_HPUX_LD)
            pHnd = (void*)shl_load(QFile::encodeName(attempt), dlFlags, 0);
#else
            pHnd = dlopen(QFile::encodeName(attempt), dlFlags);
#endif
            if (!pHnd && fileName.startsWith(QLatin1Char('/')) && QFile::exists(attempt)) {
                // We only want to continue if dlopen failed due to that the shared library did not exist.
                // However, we are only able to apply this check for absolute filenames (since they are
                // not influenced by the content of LD_LIBRARY_PATH, /etc/ld.so.cache, DT_RPATH etc...)
                // This is all because dlerror is flawed and cannot tell us the reason why it failed.
                retry = false;
            }
        }
    }

#ifdef Q_OS_MAC
    if (!pHnd) {
        if (CFBundleRef bundle = CFBundleGetBundleWithIdentifier(QCFString(fileName))) {
            QCFType<CFURLRef> url = CFBundleCopyExecutableURL(bundle);
            QCFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            pHnd = dlopen(QFile::encodeName(str), dlFlags);
            attempt = str;
        }
    }
# endif
    if (!pHnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(fileName).arg(qdlerror());
    }
    if (pHnd) {
        qualifiedFileName = attempt;
        errorString.clear();
    }
    return (pHnd != 0);
}

bool PatchedLibraryPrivate::unload_sys()
{
#if defined(QT_HPUX_LD)
    if (shl_unload((shl_t)pHnd)) {
#else
    if (dlclose(pHnd)) {
#endif
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName).arg(qdlerror());
        return false;
    }
    errorString.clear();
    return true;
}

void* PatchedLibraryPrivate::resolve_sys(const char* symbol)
{
#if defined(QT_AOUT_UNDERSCORE)
    // older a.out systems add an underscore in front of symbols
    char* undrscr_symbol = new char[strlen(symbol)+2];
    undrscr_symbol[0] = '_';
    strcpy(undrscr_symbol+1, symbol);
    void* address = dlsym(pHnd, undrscr_symbol);
    delete [] undrscr_symbol;
#elif defined(QT_HPUX_LD)
    void* address = 0;
    if (shl_findsym((shl_t*)&pHnd, symbol, TYPE_UNDEFINED, &address) < 0)
        address = 0;
#else
    void* address = dlsym(pHnd, symbol);
#endif
    if (!address) {
        errorString = QLibrary::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
            QString::fromAscii(symbol)).arg(fileName).arg(qdlerror());
    } else {
        errorString.clear();
    }
    return address;
}

#endif // Q_OS_UNIX





#ifdef Q_OS_WIN

extern QString qt_error_string(int code);

bool PatchedLibraryPrivate::load_sys()
{
#ifdef Q_OS_WINCE
    QString attempt = QFileInfo(fileName).absoluteFilePath();
#else
    QString attempt = fileName;
#endif

    //avoid 'Bad Image' message box
    UINT oldmode = SetErrorMode(SEM_FAILCRITICALERRORS|SEM_NOOPENFILEERRORBOX);
    QT_WA({
        pHnd = LoadLibraryW((TCHAR*)QDir::toNativeSeparators(attempt).utf16());
    } , {
        pHnd = LoadLibraryA(QFile::encodeName(QDir::toNativeSeparators(attempt)).data());
    });
    
    if (pluginState != IsAPlugin) {
#if defined(Q_OS_WINCE)
        if (!pHnd && ::GetLastError() == ERROR_MOD_NOT_FOUND) {
            QString secondAttempt = fileName;
            QT_WA({
                pHnd = LoadLibraryW((TCHAR*)QDir::toNativeSeparators(secondAttempt).utf16());
            } , {
                pHnd = LoadLibraryA(QFile::encodeName(QDir::toNativeSeparators(secondAttempt)).data());
            });
        }
#endif
        if (!pHnd && ::GetLastError() == ERROR_MOD_NOT_FOUND) {
            attempt += QLatin1String(".dll");
            QT_WA({
                pHnd = LoadLibraryW((TCHAR*)QDir::toNativeSeparators(attempt).utf16());
            } , {
                pHnd = LoadLibraryA(QFile::encodeName(QDir::toNativeSeparators(attempt)).data());
            });
        }
    }

    SetErrorMode(oldmode);
    if (!pHnd) {
        errorString = QLibrary::tr("Cannot load library %1: %2").arg(fileName).arg(qt_error_string());
    }
    if (pHnd) {
        errorString.clear();
        QT_WA({
            TCHAR buffer[MAX_PATH + 1];
            ::GetModuleFileNameW(pHnd, buffer, MAX_PATH);
            attempt = QString::fromUtf16(reinterpret_cast<const ushort *>(&buffer));
        }, {
            char buffer[MAX_PATH + 1];
            ::GetModuleFileNameA(pHnd, buffer, MAX_PATH);
            attempt = QString::fromLocal8Bit(buffer);
        });
        const QDir dir =  QFileInfo(fileName).dir();
        const QString realfilename = attempt.mid(attempt.lastIndexOf(QLatin1Char('\\')) + 1);
        if (dir.path() == QLatin1String("."))
            qualifiedFileName = realfilename;
        else
            qualifiedFileName = dir.filePath(realfilename);
    }
    return (pHnd != 0);
}

bool PatchedLibraryPrivate::unload_sys()
{
    if (!FreeLibrary(pHnd)) {
        errorString = QLibrary::tr("Cannot unload library %1: %2").arg(fileName).arg(qt_error_string());
        return false;
    }
    errorString.clear();
    return true;
}

void* PatchedLibraryPrivate::resolve_sys(const char* symbol)
{
#ifdef Q_OS_WINCE
    void* address = (void*)GetProcAddress(pHnd, (const wchar_t*)QString::fromLatin1(symbol).utf16());
#else
    void* address = (void*)GetProcAddress(pHnd, symbol);
#endif
    if (!address) {
        errorString = QLibrary::tr("Cannot resolve symbol \"%1\" in %2: %3").arg(
            QString::fromAscii(symbol)).arg(fileName).arg(qt_error_string());
    } else {
        errorString.clear();
    }
    return address;
}

#endif // Q_OS_WIN

QT_END_NAMESPACE
