/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "findinfiles.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QtCore/QtDebug>
#include <QtCore/QSettings>
#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtGui/QPushButton>
#include <QtGui/QFileDialog>
#include <QtGui/QVBoxLayout>

using namespace Find;
using namespace TextEditor::Internal;

FindInFiles::FindInFiles(SearchResultWindow *resultWindow)
  : BaseFileFind(resultWindow),
    m_configWidget(0),
    m_directory(0)
{
}

QString FindInFiles::id() const
{
    return "Files on Disk";
}

QString FindInFiles::displayName() const
{
    return tr("Files on File System");
}

void FindInFiles::findAll(const QString &txt, Find::FindFlags findFlags)
{
    updateComboEntries(m_directory, true);
    BaseFileFind::findAll(txt, findFlags);
}

Utils::FileIterator *FindInFiles::files() const
{
    return new Utils::SubDirFileIterator(QStringList() << m_directory->currentText(),
                                         fileNameFilters(),
                                         Core::EditorManager::instance()->defaultTextCodec());
}

QWidget *FindInFiles::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        QGridLayout * const gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setMargin(0);
        m_configWidget->setLayout(gridLayout);

        QLabel *dirLabel = new QLabel(tr("&Directory:"));
        gridLayout->addWidget(dirLabel, 0, 0, Qt::AlignRight);
        m_directory = new QComboBox;
        m_directory->setEditable(true);
        m_directory->setMaxCount(30);
        m_directory->setMinimumContentsLength(10);
        m_directory->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
        m_directory->setInsertPolicy(QComboBox::InsertAtTop);
        m_directory->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        m_directory->setModel(&m_directoryStrings);
        syncComboWithSettings(m_directory, m_directorySetting);
        dirLabel->setBuddy(m_directory);
        gridLayout->addWidget(m_directory, 0, 1);
        QPushButton *browseButton = new QPushButton(tr("&Browse"));
        gridLayout->addWidget(browseButton, 0, 2);
        connect(browseButton, SIGNAL(clicked()), this, SLOT(openFileBrowser()));

        QLabel * const filePatternLabel = new QLabel(tr("File &pattern:"));
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
    QString oldDir = m_directory->currentText();
    if (!QDir(oldDir).exists())
        oldDir.clear();
    QString dir = QFileDialog::getExistingDirectory(m_configWidget,
        tr("Directory to search"), oldDir);
    if (!dir.isEmpty())
        m_directory->setEditText(dir);
}

void FindInFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup("FindInFiles");
    writeCommonSettings(settings);
    settings->setValue("directories", m_directoryStrings.stringList());
    if (m_directory)
        settings->setValue("currentDirectory", m_directory->currentText());
    settings->endGroup();
}

void FindInFiles::readSettings(QSettings *settings)
{
    settings->beginGroup("FindInFiles");
    readCommonSettings(settings, "*.cpp,*.h");
    m_directoryStrings.setStringList(settings->value("directories").toStringList());
    m_directorySetting = settings->value("currentDirectory").toString();
    settings->endGroup();
    syncComboWithSettings(m_directory, m_directorySetting);
}
