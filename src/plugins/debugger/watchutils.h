/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef WATCHUTILS_H
#define WATCHUTILS_H

#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QMap>

QT_BEGIN_NAMESPACE
class QDebug;
QT_END_NAMESPACE

namespace TextEditor {
    class ITextEditor;
}

namespace Core {
    class IEditor;
}

namespace CPlusPlus {
    class Snapshot;
}

namespace Debugger {
namespace Internal {

class WatchData;
class GdbMi;

QByteArray dotEscape(QByteArray str);
QString currentTime();
bool isSkippableFunction(const QString &funcName, const QString &fileName);
bool isLeavableFunction(const QString &funcName, const QString &fileName);

inline bool isNameChar(char c)
{
    // could be 'stopped' or 'shlibs-added'
    return (c >= 'a' && c <= 'z') || c == '-';
}

bool hasLetterOrNumber(const QString &exp);
bool hasSideEffects(const QString &exp);
bool isKeyWord(const QString &exp);
bool isPointerType(const QByteArray &type);
bool isCharPointerType(const QByteArray &type);
bool startsWithDigit(const QString &str);
QByteArray stripPointerType(QByteArray type);
QByteArray gdbQuoteTypes(const QByteArray &type);
bool extractTemplate(const QString &type, QString *tmplate, QString *inner);
QString extractTypeFromPTypeOutput(const QString &str);
bool isFloatType(const QByteArray &type);
bool isIntOrFloatType(const QByteArray &type);
bool isIntType(const QByteArray &type);
bool isSymbianIntType(const QByteArray &type);

enum GuessChildrenResult { HasChildren, HasNoChildren, HasPossiblyChildren };
GuessChildrenResult guessChildren(const QByteArray &type);

QString quoteUnprintableLatin1(const QByteArray &ba);

// Editor tooltip support
bool isCppEditor(Core::IEditor *editor);
QString cppExpressionAt(TextEditor::ITextEditor *editor, int pos,
                        int *line, int *column, QString *function = 0);
// Editor helpers
TextEditor::ITextEditor *currentTextEditor();
bool currentTextEditorPosition(QString *fileNameIn = 0,
                               int *lineNumberIn = 0);

// Decode string data as returned by the dumper helpers.
QString decodeData(const QByteArray &baIn, int encoding);

// Get variables that are not initialized at a certain line
// of a function from the code model. Shadowed variables will
// be reported using the debugger naming conventions '<shadowed n>'
bool getUninitializedVariables(const CPlusPlus::Snapshot &snapshot,
                       const QString &function,
                       const QString &file,
                       int line,
                       QStringList *uninitializedVariables);

/* Attempt to put common code of the dumper handling into a helper
 * class.
 * "Custom dumper" is a library compiled against the current
 * Qt containing functions to evaluate values of Qt classes
 * (such as QString, taking pointers to their addresses).
 * The library must be loaded into the debuggee.
 * It provides a function that takes input from an input buffer
 * and some parameters and writes output into an output buffer.
 * Parameter 1 is the protocol:
 * 1) Query. Fills output buffer with known types, Qt version and namespace.
 *    This information is parsed and stored by this class (special type
 *    enumeration).
 * 2) Evaluate symbol, taking address and some additional parameters
 *    depending on type. */

class QtDumperHelper
{
public:
    enum Type {
        UnknownType,
        SupportedType, // A type that requires no special handling by the dumper
        // Below types require special handling
        QAbstractItemType,
        QObjectType, QWidgetType, QObjectSlotType, QObjectSignalType,
        QVectorType, QMapType, QMultiMapType, QMapNodeType, QStackType,
        StdVectorType, StdDequeType, StdSetType, StdMapType, StdStackType,
        StdStringType
    };

    // Type/Parameter struct required for building a value query
    struct TypeData {
        TypeData();
        void clear();

        Type type;
        bool isTemplate;
        QByteArray tmplate;
        QByteArray inner;
    };

    QtDumperHelper();
    void clear();

    double dumperVersion() const { return m_dumperVersion; }

    int typeCount() const;
    // Look up a simple, non-template  type
    Type simpleType(const QByteArray &simpleType) const;
    // Look up a (potentially) template type and fill parameter struct
    TypeData typeData(const QByteArray &typeName) const;
    Type type(const QByteArray &typeName) const;

    int qtVersion() const;
    QByteArray qtVersionString() const;
    QByteArray qtNamespace() const;
    void setQtNamespace(const QByteArray &ba)
        { if (!ba.isEmpty()) m_qtNamespace = ba; }

    // Complete parse of "query" (protocol 1) response from debuggee buffer.
    // 'data' excludes the leading indicator character.
    bool parseQuery(const GdbMi &data);
    // Sizes can be added as the debugger determines them
    void addSize(const QByteArray &type, int size);

    // Determine the parameters required for an "evaluate" (protocol 2) call
    void evaluationParameters(const WatchData &data,
                              const TypeData &td,
                              QByteArray *inBuffer,
                              QList<QByteArray> *extraParameters) const;

    QString toString(bool debug = false) const;

    static QString msgDumperOutdated(double requiredVersion, double currentVersion);

private:
    typedef QMap<QString, Type> NameTypeMap;
    typedef QMap<QByteArray, int> SizeCache;

    // Look up a simple (namespace) type
    QByteArray evaluationSizeofTypeExpression(const QByteArray &typeName) const;

    NameTypeMap m_nameTypeMap;
    SizeCache m_sizeCache;

    // The initial dumper query function returns sizes of some special
    // types to aid CDB since it cannot determine the size of classes.
    // They are not complete (std::allocator<X>).
    enum SpecialSizeType { IntSize, PointerSize, StdAllocatorSize,
                           QSharedPointerSize, QSharedDataPointerSize,
                           QWeakPointerSize, QPointerSize,
                           QListSize, QLinkedListSize, QVectorSize, QQueueSize,
                           SpecialSizeCount };

    // Resolve name to enumeration or SpecialSizeCount (invalid)
    SpecialSizeType specialSizeType(const QByteArray &type) const;

    int m_specialSizes[SpecialSizeCount];

    typedef QMap<QByteArray, QByteArray> ExpressionCache;
    ExpressionCache m_expressionCache;
    int m_qtVersion;
    double m_dumperVersion;
    QByteArray m_qtNamespace;

    void setQClassPrefixes(const QByteArray &qNamespace);

    QByteArray m_qPointerPrefix;
    QByteArray m_qSharedPointerPrefix;
    QByteArray m_qSharedDataPointerPrefix;
    QByteArray m_qWeakPointerPrefix;
    QByteArray m_qListPrefix;
    QByteArray m_qLinkedListPrefix;
    QByteArray m_qVectorPrefix;
    QByteArray m_qQueuePrefix;
};

QDebug operator<<(QDebug in, const QtDumperHelper::TypeData &d);

// remove the default template argument in std:: containers
QString removeDefaultTemplateArguments(QString type);


//
// GdbMi interaction
//

void setWatchDataValue(WatchData &data, const GdbMi &item);
void setWatchDataValueToolTip(WatchData &data, const GdbMi &mi,
    int encoding);
void setWatchDataChildCount(WatchData &data, const GdbMi &mi);
void setWatchDataValueEnabled(WatchData &data, const GdbMi &mi);
void setWatchDataValueEditable(WatchData &data, const GdbMi &mi);
void setWatchDataExpression(WatchData &data, const GdbMi &mi);
void setWatchDataAddress(WatchData &data, const GdbMi &mi);
void setWatchDataAddressHelper(WatchData &data, const QByteArray &addr);
void setWatchDataType(WatchData &data, const GdbMi &mi);
void setWatchDataDisplayedType(WatchData &data, const GdbMi &mi);

void parseWatchData(const QSet<QByteArray> &expandedINames,
    const WatchData &parent, const GdbMi &child,
    QList<WatchData> *insertions);

} // namespace Internal
} // namespace Debugger

#endif // WATCHUTILS_H
