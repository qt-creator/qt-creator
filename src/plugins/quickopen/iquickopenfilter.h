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
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef IQUICKOPENFILTER_H
#define IQUICKOPENFILTER_H

#include "quickopen_global.h"

#include <QtCore/QDir>
#include <QtCore/QVariant>
#include <QtCore/QFutureInterface>
#include <QtGui/QIcon>

namespace QuickOpen {

class IQuickOpenFilter;

struct FilterEntry
{
    FilterEntry() {}
    FilterEntry(IQuickOpenFilter *fromFilter, const QString &name, const QVariant &data,
                const QIcon &icon = QIcon())
    : filter(fromFilter)
    , displayName(name)
    , internalData(data)
    , displayIcon(icon)
    , resolveFileIcon(false)
    {}

    bool operator==(const FilterEntry &other) const {
        if (internalData.canConvert(QVariant::String))
            return (internalData.toString() == other.internalData.toString());
        return internalData.constData() == other.internalData.constData();
    }

    /* backpointer to creating filter */
    IQuickOpenFilter *filter;
    /* displayed string */
    QString displayName;
    /* extra information displayed in light-gray in a second column (optional) */
    QString extraInfo;
    /* can be used by the filter to save more information about the entry */
    QVariant internalData;
    /* icon to display along with the entry */
    QIcon displayIcon;
    /* internal data is interpreted as file name and icon is retrieved from the file system if true */
    bool resolveFileIcon;
};

class QUICKOPEN_EXPORT IQuickOpenFilter : public QObject
{
    Q_OBJECT

public:
    enum Priority {High = 0, Medium = 1, Low = 2};

    IQuickOpenFilter(QObject *parent = 0);
    virtual ~IQuickOpenFilter() {}

    /* visible name */
    virtual QString trName() const = 0;

    /* internal name */
    virtual QString name() const = 0;

    /* selection list order in case of multiple active filters (high goes on top) */
    virtual Priority priority() const = 0;

    /* string to type to use this filter exclusively */
    virtual QString shortcutString() const;
    void setShortcutString(const QString &shortcut);

    /* list of matches for the given user entry */
    virtual QList<FilterEntry> matchesFor(const QString &entry) = 0;

    /* user has selected the given entry that belongs to this filter */
    virtual void accept(FilterEntry selection) const = 0;

    /* implement to update caches on user request, if that's a long operation */
    virtual void refresh(QFutureInterface<void> &future) = 0;

    /* Saved state is used to restore the filter at start up. */
    virtual QByteArray saveState() const;

    /* Used to restore the filter at start up. */
    virtual bool restoreState(const QByteArray &state);

    /* User wants to configure this filter (if supported). Use it to pop up a dialog.
     * needsRefresh is used as a hint to indicate that refresh should be called.
     * The default implementation allows changing the shortcut and whether the filter
     * is enabled by default.
     */
    virtual bool openConfigDialog(QWidget *parent, bool &needsRefresh);

    /* If there is a configure dialog available for this filter. The default
     * implementation returns true. */
    virtual bool isConfigurable() const;

    /* is this filter used also when the shortcutString is not used? */
    virtual bool defaultActiveState() const;
    void setIncludedByDefault(bool includedByDefault);

    static QString trimWildcards(const QString &str) {
        if (str.isEmpty())
            return str;
        int first = 0, last = str.size()-1;
        while (first < str.size() && (str.at(first) == '*' || str.at(first) == '?'))
            ++first;
        while (last >= 0 && (str.at(last) == '*' || str.at(last) == '?'))
            --last;
        if (first > last)
            return QString();
        return str.mid(first, last-first+1);
    }

private:
    QString m_shortcut;
    bool m_default;
};

} // namespace QuickOpen

#endif // IQUICKOPENFILTER_H
