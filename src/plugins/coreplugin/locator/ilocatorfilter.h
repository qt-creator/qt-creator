/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef ILOCATORFILTER_H
#define ILOCATORFILTER_H

#include <coreplugin/id.h>

#include <QVariant>
#include <QFutureInterface>
#include <QIcon>

namespace Core {

class ILocatorFilter;

struct LocatorFilterEntry
{
    LocatorFilterEntry()
        : filter(0)
        , fileIconResolved(false)
    {}

    LocatorFilterEntry(ILocatorFilter *fromFilter, const QString &name, const QVariant &data,
                const QIcon &icon = QIcon())
        : filter(fromFilter)
        , displayName(name)
        , internalData(data)
        , displayIcon(icon)
        , fileIconResolved(false)
    {}

    bool operator==(const LocatorFilterEntry &other) const {
        if (internalData.canConvert(QVariant::String))
            return (internalData.toString() == other.internalData.toString());
        return internalData.constData() == other.internalData.constData();
    }

    /* backpointer to creating filter */
    ILocatorFilter *filter;
    /* displayed string */
    QString displayName;
    /* extra information displayed in light-gray in a second column (optional) */
    QString extraInfo;
    /* can be used by the filter to save more information about the entry */
    QVariant internalData;
    /* icon to display along with the entry */
    QIcon displayIcon;
    /* file name, if the entry is related to a file, is used e.g. for resolving a file icon */
    QString fileName;
    /* internal */
    bool fileIconResolved;
};

class CORE_EXPORT ILocatorFilter : public QObject
{
    Q_OBJECT

public:
    enum Priority {Highest = 0, High = 1, Medium = 2, Low = 3};

    ILocatorFilter(QObject *parent = 0);
    virtual ~ILocatorFilter() {}

    /* Internal Id. */
    Id id() const;

    /* Visible name. */
    QString displayName() const;

    /* Selection list order in case of multiple active filters (high goes on top). */
    Priority priority() const;

    /* String to type to use this filter exclusively. */
    QString shortcutString() const;
    void setShortcutString(const QString &shortcut);

    /* Called on the main thread before matchesFor is called in a separate thread.
       Can be used to perform actions that need to be done in the main thread before actually
       running the search. */
    virtual void prepareSearch(const QString &entry);

    /* List of matches for the given user entry. This runs in a separate thread, but only
       a single thread at a time. */
    virtual QList<LocatorFilterEntry> matchesFor(QFutureInterface<LocatorFilterEntry> &future, const QString &entry) = 0;

    /* User has selected the given entry that belongs to this filter. */
    virtual void accept(LocatorFilterEntry selection) const = 0;

    /* Implement to update caches on user request, if that's a long operation. */
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
    bool isConfigurable() const;

    /* Is this filter used also when the shortcutString is not used? */
    bool isIncludedByDefault() const;
    void setIncludedByDefault(bool includedByDefault);

    /* Returns whether the filter should be hidden from configuration and menus. */
    bool isHidden() const;

    /* Returns whether the filter should be enabled and used in menus. */
    bool isEnabled() const;

    static QString trimWildcards(const QString &str);
    static Qt::CaseSensitivity caseSensitivity(const QString &str);

    static QString msgConfigureDialogTitle();
    static QString msgPrefixLabel();
    static QString msgPrefixToolTip();
    static QString msgIncludeByDefault();
    static QString msgIncludeByDefaultToolTip();

public slots:
    /* Enable or disable the filter. */
    void setEnabled(bool enabled);

protected:
    void setHidden(bool hidden);
    void setId(Id id);
    void setPriority(Priority priority);
    void setDisplayName(const QString &displayString);
    void setConfigurable(bool configurable);

private:
    Id m_id;
    QString m_shortcut;
    Priority m_priority;
    QString m_displayName;
    bool m_includedByDefault;
    bool m_hidden;
    bool m_enabled;
    bool m_isConfigurable;
};

} // namespace Core

#endif // ILOCATORFILTER_H
