/****************************************************************************
**
** Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "filepropertiesdialog.h"
#include "ui_filepropertiesdialog.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditorfactory.h>
#include <utils/fileutils.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLocale>

using namespace Utils;

namespace Core {

FilePropertiesDialog::FilePropertiesDialog(const FilePath &filePath, QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::FilePropertiesDialog),
    m_filePath(filePath)
{
    m_ui->setupUi(this);

    connect(m_ui->readable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::ReadUser | QFile::ReadOwner, checked);
    });
    connect(m_ui->writable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::WriteUser | QFile::WriteOwner, checked);
    });
    connect(m_ui->executable, &QCheckBox::clicked, [this](bool checked) {
        setPermission(QFile::ExeUser | QFile::ExeOwner, checked);
    });

    refresh();
}

FilePropertiesDialog::~FilePropertiesDialog()
{
    delete m_ui;
}

void FilePropertiesDialog::detectTextFileSettings()
{
    QFile file(m_filePath.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        m_ui->lineEndings->setText(tr("Unknown"));
        m_ui->indentation->setText(tr("Unknown"));
        return;
    }

    char lineSeparator = '\n';
    const QByteArray data = file.read(50000);
    file.close();

    // Try to guess the files line endings
    if (data.contains("\r\n")) {
        m_ui->lineEndings->setText(tr("Windows (CRLF)"));
    } else if (data.contains("\n")) {
        m_ui->lineEndings->setText(tr("Unix (LF)"));
    } else if (data.contains("\r")) {
        m_ui->lineEndings->setText(tr("Mac (CR)"));
        lineSeparator = '\r';
    } else {
        // That does not look like a text file at all
        m_ui->lineEndings->setText(tr("Unknown"));
        return;
    }

    auto leadingSpaces = [](const QByteArray &line) {
        for (int i = 0, max = line.size(); i < max; ++i) {
            if (line.at(i) != ' ') {
                return i;
            }
        }
        return 0;
    };

    // Try to guess the files indentation style
    bool tabIndented = false;
    int lastLineIndent = 0;
    std::map<int, int> indents;
    const QList<QByteArray> list = data.split(lineSeparator);
    for (const QByteArray &line : list) {
        if (line.startsWith(' ')) {
            int spaces = leadingSpaces(line);
            int relativeCurrentLineIndent = qAbs(spaces - lastLineIndent);
            // Ignore zero or one character indentation changes
            if (relativeCurrentLineIndent < 2)
                continue;
            indents[relativeCurrentLineIndent]++;
            lastLineIndent = spaces;
        } else if (line.startsWith('\t')) {
            tabIndented = true;
        }

        if (!indents.empty() && tabIndented)
            break;
    }

    const std::map<int, int>::iterator max = std::max_element(
                indents.begin(), indents.end(),
                [](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return a.second < b.second;
    });

    if (!indents.empty()) {
        if (tabIndented) {
            m_ui->indentation->setText(tr("Mixed"));
        } else {
            m_ui->indentation->setText(tr("%1 Spaces").arg(max->first));
        }
    } else if (tabIndented) {
        m_ui->indentation->setText(tr("Tabs"));
    } else {
        m_ui->indentation->setText(tr("Unknown"));
    }
}

void FilePropertiesDialog::refresh()
{
    Utils::withNtfsPermissions<void>([this] {
        const QFileInfo fileInfo = m_filePath.toFileInfo();
        QLocale locale;

        m_ui->name->setText(fileInfo.fileName());
        m_ui->path->setText(QDir::toNativeSeparators(fileInfo.canonicalPath()));

        const Utils::MimeType mimeType = Utils::mimeTypeForFile(fileInfo);
        m_ui->mimeType->setText(mimeType.name());

        const EditorTypeList factories = IEditorFactory::preferredEditorTypes(m_filePath);
        m_ui->defaultEditor->setText(!factories.isEmpty() ? factories.at(0)->displayName() : tr("Undefined"));

        m_ui->owner->setText(fileInfo.owner());
        m_ui->group->setText(fileInfo.group());
        m_ui->size->setText(locale.formattedDataSize(fileInfo.size()));
        m_ui->readable->setChecked(fileInfo.isReadable());
        m_ui->writable->setChecked(fileInfo.isWritable());
        m_ui->executable->setChecked(fileInfo.isExecutable());
        m_ui->symLink->setChecked(fileInfo.isSymLink());
        m_ui->lastRead->setText(fileInfo.lastRead().toString(locale.dateTimeFormat()));
        m_ui->lastModified->setText(fileInfo.lastModified().toString(locale.dateTimeFormat()));
        if (mimeType.inherits("text/plain")) {
            detectTextFileSettings();
        } else {
            m_ui->lineEndings->setText(tr("Unknown"));
            m_ui->indentation->setText(tr("Unknown"));
        }
    });
}

void FilePropertiesDialog::setPermission(QFile::Permissions newPermissions, bool set)
{
    Utils::withNtfsPermissions<void>([this, newPermissions, set] {
        QFile::Permissions permissions = m_filePath.permissions();
        if (set)
            permissions |= newPermissions;
        else
            permissions &= ~newPermissions;

        if (!m_filePath.setPermissions(permissions))
            qWarning() << "Cannot change permissions for file" << m_filePath;
    });

    refresh();
}

} // Core
