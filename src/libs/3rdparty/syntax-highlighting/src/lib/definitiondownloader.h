/*
    SPDX-FileCopyrightText: 2016 Volker Krause <vkrause@kde.org>

    SPDX-License-Identifier: MIT
*/

#ifndef KSYNTAXHIGHLIGHTING_DEFINITIONDOWNLOADER_H
#define KSYNTAXHIGHLIGHTING_DEFINITIONDOWNLOADER_H

#include "ksyntaxhighlighting_export.h"

#include <QObject>
#include <memory>

namespace KSyntaxHighlighting
{
class DefinitionDownloaderPrivate;
class Repository;

/**
 * Helper class to download definition file updates.
 *
 * With the DefinitionDownloader you can download new and update existing
 * syntax highlighting definition files (xml files).
 *
 * An example that updates the highlighting Definition%s and prints the current
 * update progress to the console may look as follows:
 *
 * @code
 * auto downloader = new DefinitionDownloader(repo); // repo is a pointer to a Repository
 *
 * // print update progress to console
 * QObject::connect(downloader, &DefinitionDownloader::informationMessage, [](const QString &msg) {
 *     std::cout << qPrintable(msg) << std::endl;
 * });
 *
 * // connect to signal done to delete the downloader later
 * QObject::connect(downloader, &DefinitionDownloader::done,
 *                  downloader, &DefinitionDownloader::deleteLater);
 * downloader->start();
 * @endcode
 *
 * @see Repository, Definition
 * @since 5.28
 */
class KSYNTAXHIGHLIGHTING_EXPORT DefinitionDownloader : public QObject
{
    Q_OBJECT
public:
    /**
     * Constructor.
     * The Repository @p repo is used as reference to compare the versions of
     * the existing Definition%s with the ones that are available online.
     *
     * Optionally, @p parent is a pointer to the owner of this instance.
     */
    explicit DefinitionDownloader(Repository *repo, QObject *parent = nullptr);

    /**
     * Destructor.
     */
    ~DefinitionDownloader();

    /**
     * Starts the update procedure.
     * Once no more updates are available (i.e. either the local definition files
     * are up-to-date, or all updates have been downloaded), the signal done()
     * is emitted.
     *
     * During the update process, the signal informationMessage() can be used
     * to display the current update progress to the user.
     *
     * @see done(), informationMessage()
     */
    void start();

Q_SIGNALS:
    /**
     * Prints the information about the current state of the definition files.
     * If all files are up-to-date, this signal is emitted informing you that
     * all highlighting files are up-to-date. If there are updates, this signal
     * is emitted for each update being downloaded.
     */
    void informationMessage(const QString &msg);

    /**
     * This signal is emitted when there are no pending downloads anymore.
     */
    void done();

private:
    std::unique_ptr<DefinitionDownloaderPrivate> d;
};
}

#endif // KSYNTAXHIGHLIGHTING_DEFINITIONDOWNLOADER_H
