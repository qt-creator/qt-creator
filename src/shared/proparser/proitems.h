// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qmake_global.h"

#include <qstring.h>
#include <qvector.h>
#include <qhash.h>

QT_BEGIN_NAMESPACE

class QTextStream;

#ifdef PROPARSER_THREAD_SAFE
typedef QAtomicInt ProItemRefCount;
#else
class ProItemRefCount {
public:
    ProItemRefCount(int cnt = 0) : m_cnt(cnt) {}
    bool ref() { return ++m_cnt != 0; }
    bool deref() { return --m_cnt != 0; }
    ProItemRefCount &operator=(int value) { m_cnt = value; return *this; }
private:
    int m_cnt;
};
#endif

#ifndef QT_BUILD_QMAKE
#  define PROITEM_EXPLICIT explicit
#else
#  define PROITEM_EXPLICIT
#endif

class ProKey;
class ProStringList;
class ProFile;

class ProString {
public:
    ProString();
    ProString(const ProString &other);
    ProString &operator=(const ProString &) = default;
    template<typename A, typename B>
    ProString &operator=(const QStringBuilder<A, B> &str)
    { return *this = QString(str); }
    ProString(const QString &str);
    PROITEM_EXPLICIT ProString(QStringView str);
    PROITEM_EXPLICIT ProString(const char *str);
    template<typename A, typename B>
    ProString(const QStringBuilder<A, B> &str)
        : ProString(QString(str))
    {}
    ProString(const QString &str, int offset, int length);
    void setValue(const QString &str);
    void clear() { m_string.clear(); m_length = 0; }
    ProString &setSource(const ProString &other) { m_file = other.m_file; return *this; }
    ProString &setSource(int id) { m_file = id; return *this; }
    int sourceFile() const { return m_file; }

    ProString &prepend(const ProString &other);
    ProString &append(const ProString &other, bool *pending = nullptr);
    ProString &append(const QString &other) { return append(ProString(other)); }
    template<typename A, typename B>
    ProString &append(const QStringBuilder<A, B> &other) { return append(QString(other)); }
    ProString &append(const QLatin1String other);
    ProString &append(const char *other) { return append(QLatin1String(other)); }
    ProString &append(QChar other);
    ProString &append(const ProStringList &other, bool *pending = nullptr, bool skipEmpty1st = false);
    ProString &operator+=(const ProString &other) { return append(other); }
    ProString &operator+=(const QString &other) { return append(other); }
    template<typename A, typename B>
    ProString &operator+=(const QStringBuilder<A, B> &other) { return append(QString(other)); }
    ProString &operator+=(const QLatin1String other) { return append(other); }
    ProString &operator+=(const char *other) { return append(other); }
    ProString &operator+=(QChar other) { return append(other); }

    void chop(int n) { Q_ASSERT(n <= m_length); m_length -= n; }
    void chopFront(int n) { Q_ASSERT(n <= m_length); m_offset += n; m_length -= n; }

    bool operator==(const ProString &other) const { return toStringView() == other.toStringView(); }
    bool operator==(const QString &other) const { return toStringView() == other; }
    bool operator==(QStringView other) const { return toStringView() == other; }
    bool operator==(QLatin1String other) const  { return toStringView() == other; }
    bool operator==(const char *other) const { return toStringView() == QLatin1String(other); }
    bool operator!=(const ProString &other) const { return !(*this == other); }
    bool operator!=(const QString &other) const { return !(*this == other); }
    bool operator!=(QLatin1String other) const { return !(*this == other); }
    bool operator!=(const char *other) const { return !(*this == other); }
    bool operator<(const ProString &other) const { return toStringView() < other.toStringView(); }
    bool isNull() const { return m_string.isNull(); }
    bool isEmpty() const { return !m_length; }
    int length() const { return m_length; }
    int size() const { return m_length; }
    QChar at(int i) const { Q_ASSERT((uint)i < (uint)m_length); return constData()[i]; }
    const QChar *constData() const { return m_string.constData() + m_offset; }
    ProString mid(int off, int len = -1) const;
    ProString left(int len) const { return mid(0, len); }
    ProString right(int len) const { return mid(qMax(0, size() - len)); }
    ProString trimmed() const;
    int compare(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().compare(sub.toStringView(), cs); }
    int compare(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().compare(sub, cs); }
    int compare(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().compare(QLatin1String(sub), cs); }
    bool startsWith(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().startsWith(sub.toStringView(), cs); }
    bool startsWith(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().startsWith(sub, cs); }
    bool startsWith(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().startsWith(QLatin1String(sub), cs); }
    bool startsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().startsWith(c, cs); }
    template<typename A, typename B>
    bool startsWith(const QStringBuilder<A, B> &str) { return startsWith(QString(str)); }
    bool endsWith(const ProString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().endsWith(sub.toStringView(), cs); }
    bool endsWith(const QString &sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().endsWith(sub, cs); }
    bool endsWith(const char *sub, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().endsWith(QLatin1String(sub), cs); }
    template<typename A, typename B>
    bool endsWith(const QStringBuilder<A, B> &str) { return endsWith(QString(str)); }
    bool endsWith(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().endsWith(c, cs); }
    int indexOf(const QString &s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().indexOf(s, from, cs); }
    int indexOf(const char *s, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().indexOf(QLatin1String(s), from, cs); }
    int indexOf(QChar c, int from = 0, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().indexOf(c, from, cs); }
    int lastIndexOf(const QString &s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().lastIndexOf(s, from, cs); }
    int lastIndexOf(const char *s, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().lastIndexOf(QLatin1String(s), from, cs); }
    int lastIndexOf(QChar c, int from = -1, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return toStringView().lastIndexOf(c, from, cs); }
    bool contains(const QString &s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(s, 0, cs) >= 0; }
    bool contains(const char *s, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(QLatin1String(s), 0, cs) >= 0; }
    bool contains(QChar c, Qt::CaseSensitivity cs = Qt::CaseSensitive) const { return indexOf(c, 0, cs) >= 0; }
    qlonglong toLongLong(bool *ok = nullptr, int base = 10) const { return toStringView().toLongLong(ok, base); }
    int toInt(bool *ok = nullptr, int base = 10) const { return toStringView().toInt(ok, base); }
    short toShort(bool *ok = nullptr, int base = 10) const { return toStringView().toShort(ok, base); }

    uint hash() const { return m_hash; }
    static uint hash(const QChar *p, int n);

    ALWAYS_INLINE QStringView toStringView() const { return QStringView(m_string).mid(m_offset, m_length); }

    ALWAYS_INLINE ProKey &toKey() { return *(ProKey *)this; }
    ALWAYS_INLINE const ProKey &toKey() const { return *(const ProKey *)this; }

    QString toQString() const;
    QString &toQString(QString &tmp) const;

    QByteArray toLatin1() const { return toStringView().toLatin1(); }

    friend QDebug operator<<(QDebug debug, const ProString &str);

private:
    ProString(const ProKey &other);
    ProString &operator=(const ProKey &other);

    enum OmitPreHashing { NoHash };
    ProString(const ProString &other, OmitPreHashing);

    enum DoPreHashing { DoHash };
    ALWAYS_INLINE ProString(const QString &str, DoPreHashing);
    ALWAYS_INLINE ProString(const char *str, DoPreHashing);
    ALWAYS_INLINE ProString(const QString &str, int offset, int length, DoPreHashing);
    ALWAYS_INLINE ProString(const QString &str, int offset, int length, uint hash);

    QString m_string;
    int m_offset, m_length;
    int m_file;
    mutable uint m_hash;
    uint updatedHash() const;
    friend size_t qHash(const ProString &str);
    friend QString operator+(const ProString &one, const ProString &two);
    friend class ProKey;
};
Q_DECLARE_TYPEINFO(ProString, Q_RELOCATABLE_TYPE);


class ProKey : public ProString {
public:
    ALWAYS_INLINE ProKey() : ProString() {}
    explicit ProKey(const QString &str);
    template<typename A, typename B>
    ProKey(const QStringBuilder<A, B> &str)
        : ProString(str)
    {}
    PROITEM_EXPLICIT ProKey(const char *str);
    ProKey(const QString &str, int off, int len);
    ProKey(const QString &str, int off, int len, uint hash);
    void setValue(const QString &str);

#ifdef Q_CC_MSVC
    // Workaround strange MSVC behaviour when exporting classes with ProKey members.
    ALWAYS_INLINE ProKey(const ProKey &other) : ProString(other.toString()) {}
    ALWAYS_INLINE ProKey &operator=(const ProKey &other)
    {
        toString() = other.toString();
        return *this;
    }
#endif

    ALWAYS_INLINE ProString &toString() { return *(ProString *)this; }
    ALWAYS_INLINE const ProString &toString() const { return *(const ProString *)this; }

private:
    ProKey(const ProString &other);
};
Q_DECLARE_TYPEINFO(ProKey, Q_RELOCATABLE_TYPE);

template <> struct QConcatenable<ProString> : private QAbstractConcatenable
{
    typedef ProString type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const ProString &a) { return a.length(); }
    static inline void appendTo(const ProString &a, QChar *&out)
    {
        const auto n = a.size();
        if (!n)
            return;
        memcpy(out, a.toStringView().data(), sizeof(QChar) * n);
        out += n;
    }
};

template <> struct QConcatenable<ProKey> : private QAbstractConcatenable
{
    typedef ProKey type;
    typedef QString ConvertTo;
    enum { ExactSize = true };
    static int size(const ProKey &a) { return a.length(); }
    static inline void appendTo(const ProKey &a, QChar *&out)
    {
        const auto n = a.size();
        if (!n)
            return;
        memcpy(out, a.toStringView().data(), sizeof(QChar) * n);
        out += n;
    }
};


size_t qHash(const ProString &str);

inline QString &operator+=(QString &that, const ProString &other)
    { return that += other.toStringView(); }

QTextStream &operator<<(QTextStream &t, const ProString &str);
template<typename A, typename B>
QTextStream &operator<<(QTextStream &t, const QStringBuilder<A, B> &str) { return t << QString(str); }

// This class manages read-only access to a ProString via a raw data QString
// temporary, ensuring that the latter is accessed exclusively.
class ProStringRoUser
{
public:
    ProStringRoUser(QString &rs)
    {
        m_rs = &rs;
    }
    ProStringRoUser(const ProString &ps, QString &rs)
        : ProStringRoUser(rs)
    {
        ps.toQString(rs);
    }
    // No destructor, as a RAII pattern cannot be used: references to the
    // temporary string can legitimately outlive instances of this class
    // (if they are held by Qt, e.g. in QRegExp).
    QString &set(const ProString &ps) { return ps.toQString(*m_rs); }
    QString &str() { return *m_rs; }

protected:
    QString *m_rs;
};

// This class manages read-write access to a ProString via a raw data QString
// temporary, ensuring that the latter is accessed exclusively, and that raw
// data does not leak outside its source's refcounting.
class ProStringRwUser : public ProStringRoUser
{
public:
    ProStringRwUser(QString &rs)
        : ProStringRoUser(rs), m_ps(nullptr) {}
    ProStringRwUser(const ProString &ps, QString &rs)
        : ProStringRoUser(ps, rs), m_ps(&ps) {}
    QString &set(const ProString &ps) { m_ps = &ps; return ProStringRoUser::set(ps); }
    ProString extract(const QString &s) const
        { return s.isSharedWith(*m_rs) ? *m_ps : ProString(s).setSource(*m_ps); }
    ProString extract(const QString &s, const ProStringRwUser &other) const
    {
        if (other.m_ps && s.isSharedWith(*other.m_rs))
            return *other.m_ps;
        return extract(s);
    }

private:
    const ProString *m_ps;
};

class ProStringList : public QVector<ProString> {
public:
    ProStringList() {}
    ProStringList(const ProString &str) { *this << str; }
    explicit ProStringList(const QStringList &list);
    QStringList toQStringList() const;

    ProStringList &operator<<(const ProString &str)
        { QVector<ProString>::operator<<(str); return *this; }

    int length() const { return size(); }

    QString join(const ProString &sep) const;
    QString join(const QString &sep) const;
    QString join(QChar sep) const;
    template<typename A, typename B>
    QString join(const QStringBuilder<A, B> &str) { return join(QString(str)); }

    void insertUnique(const ProStringList &value);

    void removeAll(const ProString &str);
    void removeAll(const char *str);
    void removeEach(const ProStringList &value);
    void removeAt(int idx) { remove(idx); }
    void removeEmpty();
    void removeDuplicates();

    bool contains(const ProString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    bool contains(QStringView str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
    bool contains(const QString &str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const
        { return contains(ProString(str), cs); }
    bool contains(const char *str, Qt::CaseSensitivity cs = Qt::CaseSensitive) const;
};
Q_DECLARE_TYPEINFO(ProStringList, Q_RELOCATABLE_TYPE);

inline ProStringList operator+(const ProStringList &one, const ProStringList &two)
    { ProStringList ret = one; ret += two; return ret; }

typedef QMap<ProKey, ProStringList> ProValueMap;

// For std::list (sic!)
#ifdef Q_CC_MSVC
inline bool operator<(const ProValueMap &, const ProValueMap &)
{
    Q_ASSERT(false);
    return false;
}
#endif

// These token definitions affect both ProFileEvaluator and ProWriter
enum ProToken {
    TokTerminator = 0,  // end of stream (possibly not included in length; must be zero)
    TokLine,            // line marker:
                        // - line (1)
    TokAssign,          // variable =
    TokAppend,          // variable +=
    TokAppendUnique,    // variable *=
    TokRemove,          // variable -=
    TokReplace,         // variable ~=
                        // previous literal/expansion is a variable manipulation
                        // - lower bound for expected output length (1)
                        // - value expression + TokValueTerminator
    TokValueTerminator, // assignment value terminator
    TokLiteral,         // literal string (fully dequoted)
                        // - length (1)
                        // - string data (length; unterminated)
    TokHashLiteral,     // literal string with hash (fully dequoted)
                        // - hash (2)
                        // - length (1)
                        // - string data (length; unterminated)
    TokVariable,        // qmake variable expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
    TokProperty,        // qmake property expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
    TokEnvVar,          // environment variable expansion
                        // - name length (1)
                        // - name (name length; unterminated)
    TokFuncName,        // replace function expansion
                        // - hash (2)
                        // - name length (1)
                        // - name (name length; unterminated)
                        // - ((nested expansion + TokArgSeparator)* + nested expansion)?
                        // - TokFuncTerminator
    TokArgSeparator,    // function argument separator
    TokFuncTerminator,  // function argument list terminator
    TokCondition,       // previous literal/expansion is a conditional
    TokTestCall,        // previous literal/expansion is a test function call
                        // - ((nested expansion + TokArgSeparator)* + nested expansion)?
                        // - TokFuncTerminator
    TokReturn,          // previous literal/expansion is a return value
    TokBreak,           // break loop
    TokNext,            // shortcut to next loop iteration
    TokNot,             // '!' operator
    TokAnd,             // ':' operator
    TokOr,              // '|' operator
    TokBranch,          // branch point:
                        // - then block length (2)
                        // - then block + TokTerminator (then block length)
                        // - else block length (2)
                        // - else block + TokTerminator (else block length)
    TokForLoop,         // for loop:
                        // - variable name: hash (2), length (1), chars (length)
                        // - expression: length (2), bytes + TokValueTerminator (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
    TokTestDef,         // test function definition:
    TokReplaceDef,      // replace function definition:
                        // - function name: hash (2), length (1), chars (length)
                        // - body length (2)
                        // - body + TokTerminator (body length)
    TokBypassNesting,   // escape from function local variable scopes:
                        // - block length (2)
                        // - block + TokTerminator (block length)
    TokMask = 0xff,
    TokQuoted = 0x100,  // The expression is quoted => join expanded stringlist
    TokNewStr = 0x200   // Next stringlist element
};

class QMAKE_EXPORT ProFile
{
public:
    ProFile(const QString &device, int id, const QString &fileName);
    ~ProFile();

    int id() const { return m_id; }
    const QString &fileName() const { return m_fileName; }
    const QString &device() const { return m_device; }
    const QString &directoryName() const { return m_directoryName; }
    const QString &items() const { return m_proitems; }
    QString *itemsRef() { return &m_proitems; }
    const ushort *tokPtr() const { return (const ushort *)m_proitems.constData(); }
    const ushort *tokPtrEnd() const { return (const ushort *)m_proitems.constData() + m_proitems.size(); }

    void ref() { m_refCount.ref(); }
    void deref() { if (!m_refCount.deref()) delete this; }

    bool isOk() const { return m_ok; }
    void setOk(bool ok) { m_ok = ok; }

    bool isHostBuild() const { return m_hostBuild; }
    void setHostBuild(bool host_build) { m_hostBuild = host_build; }

    ProString getStr(const ushort *&tPtr);
    ProKey getHashStr(const ushort *&tPtr);

private:
    ProItemRefCount m_refCount;
    QString m_proitems;
    const QString m_fileName;
    const QString m_device;
    QString m_directoryName;
    int m_id;
    bool m_ok;
    bool m_hostBuild;
};

class ProFunctionDef {
public:
    ProFunctionDef(ProFile *pro, int offset) : m_pro(pro), m_offset(offset) { m_pro->ref(); }
    ProFunctionDef(const ProFunctionDef &o) : m_pro(o.m_pro), m_offset(o.m_offset) { m_pro->ref(); }
    ProFunctionDef(ProFunctionDef &&other) noexcept
        : m_pro(other.m_pro), m_offset(other.m_offset) { other.m_pro = nullptr; }
    ~ProFunctionDef() { if (m_pro) m_pro->deref(); }
    ProFunctionDef &operator=(const ProFunctionDef &o)
    {
        if (this != &o) {
            if (m_pro)
                m_pro->deref();
            m_pro = o.m_pro;
            m_pro->ref();
            m_offset = o.m_offset;
        }
        return *this;
    }
    ProFunctionDef &operator=(ProFunctionDef &&other) noexcept
    {
        ProFunctionDef moved(std::move(other));
        swap(moved);
        return *this;
    }
    void swap(ProFunctionDef &other) noexcept
    {
        qSwap(m_pro, other.m_pro);
        qSwap(m_offset, other.m_offset);
    }

    ProFile *pro() const { return m_pro; }
    const ushort *tokPtr() const { return m_pro->tokPtr() + m_offset; }
private:
    ProFile *m_pro;
    int m_offset;
};

Q_DECLARE_TYPEINFO(ProFunctionDef, Q_RELOCATABLE_TYPE);

struct ProFunctionDefs {
    QHash<ProKey, ProFunctionDef> testFunctions;
    QHash<ProKey, ProFunctionDef> replaceFunctions;
};

QT_END_NAMESPACE
