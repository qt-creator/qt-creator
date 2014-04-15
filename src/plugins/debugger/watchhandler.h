/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#ifndef DEBUGGER_WATCHHANDLER_H
#define DEBUGGER_WATCHHANDLER_H

#include "watchdata.h"

#include <QAbstractItemModel>
#include <QPointer>
#include <QVector>

QT_FORWARD_DECLARE_CLASS(QTabWidget)

namespace Debugger {
namespace Internal {

// Special formats. Keep in sync with dumper.py.
enum DisplayFormat
{
    AutomaticFormat = -1, // Based on type for individuals, dumper default for types.
    RawFormat = 0,

    // Values between 1 and 99 refer to dumper provided custom formats.

    // Values between 100 and 199 refer to well-known formats handled in dumpers.
    KnownDumperFormatBase = 100,
    Latin1StringFormat,
    Utf8StringFormat,
    Local8BitStringFormat,
    Utf16StringFormat,
    Ucs4StringFormat,

    Array10Format,
    Array100Format,
    Array1000Format,
    Array10000Format,


    // Values above 200 refer to format solely handled in the WatchHandler code
    ArtificialFormatBase = 200,

    BoolTextFormat,
    BoolIntegerFormat,

    DecimalIntegerFormat,
    HexadecimalIntegerFormat,
    BinaryIntegerFormat,
    OctalIntegerFormat,

    CompactFloatFormat,
    ScientificFloatFormat,
};


class TypeFormatItem
{
public:
    TypeFormatItem() : format(-1) {}
    TypeFormatItem(const QString &display, int format);

    QString display;
    int format;
};

class TypeFormatList : public QVector<TypeFormatItem>
{
public:
    using QVector::append;
    void append(int format);
    TypeFormatItem find(int format) const;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::TypeFormatList)

namespace Debugger {

class DebuggerEngine;

namespace Internal {

class WatchModel;

class UpdateParameters
{
public:
    UpdateParameters() { tryPartial = tooltipOnly = false; }

    bool tryPartial;
    bool tooltipOnly;
    QByteArray varList;
};

typedef QHash<QString, QStringList> DumperTypeFormats; // Type name -> Dumper Formats

class WatchHandler : public QObject
{
    Q_OBJECT

public:
    explicit WatchHandler(DebuggerEngine *engine);
    ~WatchHandler();

    QAbstractItemModel *model() const;

    void cleanup();
    void watchExpression(const QString &exp, const QString &name = QString());
    void watchVariable(const QString &exp);
    Q_SLOT void clearWatches();

    void showEditValue(const WatchData &data);

    const WatchData *watchData(const QModelIndex &) const;
    const QModelIndex watchDataIndex(const QByteArray &iname) const;
    const WatchData *findData(const QByteArray &iname) const;
    const WatchData *findCppLocalVariable(const QString &name) const;
    bool hasItem(const QByteArray &iname) const;

    void loadSessionData();
    void saveSessionData();

    bool isExpandedIName(const QByteArray &iname) const;
    QSet<QByteArray> expandedINames() const;

    static QStringList watchedExpressions();
    static QHash<QByteArray, int> watcherNames();

    QByteArray expansionRequests() const;
    QByteArray typeFormatRequests() const;
    QByteArray individualFormatRequests() const;

    int format(const QByteArray &iname) const;

    void addTypeFormats(const QByteArray &type, const QStringList &formats);
    void setTypeFormats(const DumperTypeFormats &typeFormats);
    DumperTypeFormats typeFormats() const;

    void setUnprintableBase(int base);
    static int unprintableBase();

    QByteArray watcherName(const QByteArray &exp);
    QString editorContents();
    void editTypeFormats(bool includeLocals, const QByteArray &iname);

    void scheduleResetLocation();
    void resetLocation();
    bool isValidToolTip(const QByteArray &iname) const;

    void setCurrentItem(const QByteArray &iname);
    void updateWatchersWindow();

    void insertData(const WatchData &data); // Convenience.
    void insertData(const QList<WatchData> &list);
    void insertIncompleteData(const WatchData &data);
    void removeData(const QByteArray &iname);
    void removeChildren(const QByteArray &iname);
    void removeAllData(bool includeInspectData = false);
    void resetValueCache();

private:
    void removeSeparateWidget(QObject *o);
    void showSeparateWidget(QWidget *w);

    friend class WatchModel;

    void saveWatchers();
    static void loadFormats();
    static void saveFormats();

    void setFormat(const QByteArray &type, int format);

    WatchModel *m_model; // Owned.
    DebuggerEngine *m_engine; // Not owned.
    QPointer<QTabWidget> m_separateWindow; // Owned.

    int m_watcherCounter;

    bool m_contentsValid;
    bool m_resetLocationScheduled;
};

} // namespace Internal
} // namespace Debugger

Q_DECLARE_METATYPE(Debugger::Internal::UpdateParameters)

#endif // DEBUGGER_WATCHHANDLER_H
