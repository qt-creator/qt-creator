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

#include "findinfiles.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <utils/filesearch.h>
#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QSettings>
#include <QDir>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>
#include <QComboBox>
#include <QHBoxLayout>

using namespace Core;
using namespace TextEditor;

static FindInFiles *m_instance = 0;

FindInFiles::FindInFiles()
  : m_configWidget(0),
    m_directory(0)
{
    m_instance = this;
    connect(EditorManager::instance(), SIGNAL(findOnFileSystemRequest(QString)),
            this, SLOT(findOnFileSystem(QString)));
}

FindInFiles::~FindInFiles()
{
}

QString FindInFiles::id() const
{
    return QLatin1String("Files on Disk");
}

QString FindInFiles::displayName() const
{
    return tr("Files on File System");
}

void FindInFiles::findAll(const QString &txt, FindFlags findFlags)
{
    updateComboEntries(m_directory, true);
    BaseFileFind::findAll(txt, findFlags);
}

Utils::FileIterator *FindInFiles::files(const QStringList &nameFilters,
                                        const QVariant &additionalParameters) const
{
    return new Utils::SubDirFileIterator(QStringList() << additionalParameters.toString(),
                                         nameFilters,
                                         EditorManager::defaultTextCodec());
}

QVariant FindInFiles::additionalParameters() const
{
    return qVariantFromValue(path().toString());
}

QString FindInFiles::label() const
{
    const QChar slash = QLatin1Char('/');
    const QStringList &nonEmptyComponents = path().toFileInfo().absoluteFilePath()
            .split(slash, QString::SkipEmptyParts);
    return tr("Directory \"%1\":").arg(nonEmptyComponents.isEmpty() ? QString(slash) : nonEmptyComponents.last());
}

QString FindInFiles::toolTip() const
{
    //: %3 is filled by BaseFileFind::runNewSearch
    return tr("Path: %1\nFilter: %2\n%3")
            .arg(path().toUserOutput())
            .arg(fileNameFilters().join(QLatin1Char(',')));
}

QWidget *FindInFiles::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);

        QLabel *dirLabel = new QLabel(tr("Director&y:"));
        gridLayout->addWidget(dirLabel, 0, 0, Qt::AlignRight);
        m_directory = new QComboBox;
        m_directory->setEditable(true);
        m_directory->setMaxCount(30);
        m_directory->setMinimumContentsLength(10);
        m_directory->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_directory->setInsertPolicy(QComboBox::InsertAtTop);
        m_directory->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_directory->setModel(&m_directoryStrings);
        syncComboWithSettings(m_directory, m_directorySetting.toUserOutput());
        dirLabel->setBuddy(m_directory);
        gridLayout->addWidget(m_directory, 0, 1);
        QPushButton *browseButton = new QPushButton(Utils::PathChooser::browseButtonLabel());
        gridLayout->addWidget(browseButton, 0, 2);
        connect(browseButton, SIGNAL(clicked()), this, SLOT(openFileBrowser()));

        QLabel * const filePatternLabel = new QLabel(tr("Fi&le pattern:"));
        filePatternLabel->setMinimumWidth(80);
        filePatternLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        filePatternLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
        QWidget *patternWidget = createPatternWidget();
        filePatternLabel->setBuddy(patternWidget);
        gridLayout->addWidget(filePatternLabel, 1, 0);
        gridLayout->addWidget(patternWidget, 1, 1, 1, 2);
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    }
    return m_configWidget;
}

void FindInFiles::openFileBrowser()
{
    if (!m_directory)
        return;
    QString oldDir = path().toString();
    if (!QDir(oldDir).exists())
        oldDir.clear();
    QString dir = QFileDialog::getExistingDirectory(m_configWidget,
        tr("Directory to search"), oldDir);
    if (!dir.isEmpty())
        m_directory->setEditText(QDir::toNativeSeparators(dir));
}

Utils::FileName FindInFiles::path() const
{
    return Utils::FileName::fromUserInput(Utils::FileUtils::normalizePathName(
                                              m_directory->currentText()));
}

void FindInFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInFiles"));
    writeCommonSettings(settings);
    settings->setValue(QLatin1String("directories"), m_directoryStrings.stringList());
    if (m_directory)
        settings->setValue(QLatin1String("currentDirectory"), path().toString());
    settings->endGroup();
}

void FindInFiles::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInFiles"));
    readCommonSettings(settings, QLatin1String("*.cpp,*.h"));
    m_directoryStrings.setStringList(settings->value(QLatin1String("directories")).toStringList());
    m_directorySetting = Utils::FileName::fromString(
                settings->value(QLatin1String("currentDirectory")).toString());
    settings->endGroup();
    syncComboWithSettings(m_directory, m_directorySetting.toUserOutput());
}

void FindInFiles::setDirectory(const Utils::FileName &directory)
{
    syncComboWithSettings(m_directory, directory.toUserOutput());
}

void FindInFiles::findOnFileSystem(const QString &path)
{
    QTC_ASSERT(m_instance, return);
    const QFileInfo fi(path);
    const QString folder = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    m_instance->setDirectory(Utils::FileName::fromString(folder));
    FindPlugin::instance()->openFindDialog(m_instance);
}
