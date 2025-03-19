// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "fileiconprovider.h"

#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/qtcassert.h>

#include <QApplication>
#include <QStyle>
#include <QPainter>
#include <QFileInfo>
#include <QHash>
#include <QDebug>

#include <QFileIconProvider>
#include <QIcon>
#include <QLoggingCategory>

#include <optional>
#include <variant>

using namespace Utils;

Q_LOGGING_CATEGORY(fileIconProvider, "qtc.core.fileiconprovider", QtWarningMsg)

/*!
  \class Utils::FileIconProvider
  \internal
  \inmodule QtCreator
  \brief Provides functions for registering custom overlay icons for system
  icons.

  Provides icons based on file suffixes with the ability to overwrite system
  icons for specific subtypes. The underlying QFileIconProvider
  can be used for QFileSystemModel.

  \note Registering overlay icons currently completely replaces the system
        icon and is therefore not recommended on platforms that have their
        own overlay icon handling (\macOS and Windows).

  Plugins can register custom overlay icons via registerIconOverlayForSuffix(), and
  retrieve icons via the icon() function.
  */

using Item = std::variant<QIcon, QString>; // icon or filename for the icon

namespace Utils {
namespace FileIconProvider {

static std::optional<QIcon> getIcon(QHash<QString, Item> &cache, const QString &key)
{
    auto it = cache.constFind(key);
    if (it == cache.constEnd())
        return {};
    if (const QIcon *icon = std::get_if<QIcon>(&*it))
        return *icon;
    // need to create icon from file name first
    const QString *fileName = std::get_if<QString>(&*it);
    QTC_ASSERT(fileName, return {});
    const QIcon icon = QIcon(
        FileIconProvider::overlayIcon(QStyle::SP_FileIcon, QIcon(*fileName), QSize(16, 16)));
    cache.insert(key, icon);
    return icon;
}

class FileIconProviderImplementation : public QFileIconProvider
{
public:
    FileIconProviderImplementation()
    {}

    QIcon icon(const FilePath &filePath) const;
    using QFileIconProvider::icon;

    void registerIconOverlayForFilename(const QString &iconFilePath, const QString &filename)
    {
        m_filenameCache.insert(filename, iconFilePath);
    }

    void registerIconOverlayForSuffix(const QString &iconFilePath, const QString &suffix)
    {
        m_suffixCache.insert(suffix, iconFilePath);
    }

    void registerIconOverlayForMimeType(const QIcon &icon, const QString &mimeName)
    {
        // avoid accessing the MIME database right away
        m_mimeUpdater.append([this, icon, mimeName] {
            addIconOverlayForMimeType(icon, Utils::mimeTypeForName(mimeName));
        });
    }

    void registerIconForMimeType(const QIcon &icon, const QString &mimeName)
    {
        // avoid accessing the MIME database right away
        m_mimeUpdater.append(
            [this, icon, mimeName] { addIconForMimeType(icon, Utils::mimeTypeForName(mimeName)); });
    }

    void registerIconOverlayForMimeType(const QString &iconFilePath, const QString &mimeName)
    {
        // avoid accessing the MIME database right away
        m_mimeUpdater.append([this, iconFilePath, mimeName] {
            addIconOverlayForMimeType(iconFilePath, Utils::mimeTypeForName(mimeName));
        });
    }

private:
    void addIconOverlayForMimeType(const QIcon &icon, const Utils::MimeType &mimeType)
    {
        const QStringList suffixes = mimeType.suffixes();
        for (const QString &suffix : suffixes) {
            QTC_ASSERT(!icon.isNull() && !suffix.isEmpty(), return );

            const QPixmap fileIconPixmap = FileIconProvider::overlayIcon(QStyle::SP_FileIcon,
                                                                         icon,
                                                                         QSize(16, 16));
            // replace old icon, if it exists
            m_suffixCache.insert(suffix, fileIconPixmap);
        }
    }

    void addIconOverlayForMimeType(const QString &iconFilePath, const Utils::MimeType &mimeType)
    {
        const QStringList suffixes =  mimeType.suffixes();
        for (const QString &suffix : suffixes)
            registerIconOverlayForSuffix(iconFilePath, suffix);
    }

    void addIconForMimeType(const QIcon &icon, const Utils::MimeType &mimeType)
    {
        const QStringList suffixes = mimeType.suffixes();
        for (const QString &suffix : suffixes)
            m_suffixCache.insert(suffix, icon);
    }

    void ensureMimeOverlays() const
    {
        for (const std::function<void()> &f : std::as_const(m_mimeUpdater))
            f();
        m_mimeUpdater.clear();
    }

    // Mapping of file suffix to icon.
    mutable QHash<QString, Item> m_suffixCache;
    mutable QHash<QString, Item> m_filenameCache;
    mutable QList<std::function<void()>> m_mimeUpdater;

    // QAbstractFileIconProvider interface
public:
    QIcon icon(IconType) const override;
    QIcon icon(const QFileInfo &) const override;
    QString type(const QFileInfo &) const override;
};

QIcon FileIconProviderImplementation::icon(IconType type) const
{
    return QFileIconProvider::icon(type);
}

QString FileIconProviderImplementation::type(const QFileInfo &fi) const
{
    const FilePath fPath = FilePath::fromString(fi.filePath());
    if (!fPath.isLocal()) {
        if (fi.isDir()) {
#ifdef Q_OS_WIN
        return QGuiApplication::translate("QAbstractFileIconProvider", "File Folder", "Match Windows Explorer");
#else
        return QGuiApplication::translate("QAbstractFileIconProvider", "Folder", "All other platforms");
#endif
        }
        if (fi.isExecutable()) {
            return "Program";
        }

        return "File";
    }
    return QFileIconProvider::type(fi);
}

FileIconProviderImplementation *instance()
{
    static FileIconProviderImplementation theInstance;
    return &theInstance;
}

QFileIconProvider *iconProvider()
{
    return instance();
}

static const QIcon &unknownFileIcon()
{
    static const QIcon icon(QApplication::style()->standardIcon(QStyle::SP_FileIcon));
    return icon;
}

static const QIcon &dirIcon()
{
    static const QIcon icon(QApplication::style()->standardIcon(QStyle::SP_DirIcon));
    return icon;
}

QIcon FileIconProviderImplementation::icon(const FilePath &filePath) const
{
    qCDebug(fileIconProvider) << "FileIconProvider::icon" << filePath.absoluteFilePath();

    if (filePath.isEmpty())
        return unknownFileIcon();

    // Check if its one of the virtual devices directories
    if (filePath.path().startsWith(FilePath::specialRootPath())) {
        // If the filepath does not need a device, it is a virtual device directory
        if (filePath.isLocal())
            return dirIcon();
    }

    const bool isDir = filePath.isDir();

    // Check for cached overlay icons by file suffix.
    const QString filename = !isDir ? filePath.fileName() : QString();
    if (!filename.isEmpty()) {
        const std::optional<QIcon> icon = getIcon(m_filenameCache, filename);
        if (icon)
            return *icon;
    }

    ensureMimeOverlays();

    const QString suffix = !isDir ? filePath.suffix() : QString();
    if (!suffix.isEmpty()) {
        const std::optional<QIcon> icon = getIcon(m_suffixCache, suffix);
        if (icon)
            return *icon;
    }

    if (!filePath.isLocal())
        return isDir ? dirIcon() : unknownFileIcon();

    // Get icon from OS (and cache it based on suffix!)
    QIcon icon;
    if (HostOsInfo::isWindowsHost() || HostOsInfo::isMacHost())
        icon = QFileIconProvider::icon(filePath.toFileInfo());
    else // File icons are unknown on linux systems.
        icon = isDir ? QFileIconProvider::icon(filePath.toFileInfo()) : unknownFileIcon();

    if (!isDir && !suffix.isEmpty())
        m_suffixCache.insert(suffix, icon);
    return icon;
}

QIcon FileIconProviderImplementation::icon(const QFileInfo &fi) const
{
    qCDebug(fileIconProvider) << "FileIconProvider::icon" << fi.absoluteFilePath();

    return icon(FilePath::fromString(fi.filePath()));
}

/*!
  \internal
  Returns the icon associated with the file suffix in \a filePath. If there is none,
  the default icon of the operating system is returned.
  */

QIcon icon(const FilePath &filePath)
{
    return instance()->icon(filePath);
}

/*!
    \internal
    \overload
*/
QIcon icon(QFileIconProvider::IconType type)
{
    return instance()->icon(type);
}

/*!
  \internal
  Creates a pixmap with \a baseIcon and lays \a overlayIcon over it.
  */
QPixmap overlayIcon(const QPixmap &baseIcon, const QIcon &overlayIcon)
{
    QPixmap result = baseIcon;
    QPainter painter(&result);
    overlayIcon.paint(&painter, QRect(QPoint(), result.size() / result.devicePixelRatio()));
    return result;
}

/*!
  \internal
  Creates a pixmap with \a baseIcon at \a size and \a overlay.
  */
QPixmap overlayIcon(QStyle::StandardPixmap baseIcon, const QIcon &overlay, const QSize &size)
{
    return overlayIcon(QApplication::style()->standardIcon(baseIcon).pixmap(size), overlay);
}

/*!
  \internal
  Registers an icon at \a path for a given \a suffix, overlaying the system
  file icon.
 */
void registerIconOverlayForSuffix(const QString &path, const QString &suffix)
{
    instance()->registerIconOverlayForSuffix(path, suffix);
}

/*!
  \internal
  Registers \a icon for all the suffixes of a the mime type \a mimeType,
  overlaying the system file icon.
  */
void registerIconOverlayForMimeType(const QIcon &icon, const QString &mimeType)
{
    instance()->registerIconOverlayForMimeType(icon, mimeType);
}

void registerIconForMimeType(const QIcon &icon, const QString &mimeType)
{
    instance()->registerIconForMimeType(icon, mimeType);
}

/*!
    \internal
    \overload
 */
void registerIconOverlayForMimeType(const QString &path, const QString &mimeType)
{
    instance()->registerIconOverlayForMimeType(path, mimeType);
}

void registerIconOverlayForFilename(const QString &path, const QString &filename)
{
    instance()->registerIconOverlayForFilename(path, filename);
}

// Return a standard directory icon with the specified overlay:
QIcon directoryIcon(const QString &overlay)
{
    // Overlay the SP_DirIcon with the custom icons
    const QSize desiredSize = QSize(16, 16);

    const QPixmap dirPixmap = dirIcon().pixmap(desiredSize);
    const QIcon overlayIcon(overlay);
    QIcon result;
    result.addPixmap(FileIconProvider::overlayIcon(dirPixmap, overlayIcon));
    return result;
}

} // namespace FileIconProvider
} // namespace Core
