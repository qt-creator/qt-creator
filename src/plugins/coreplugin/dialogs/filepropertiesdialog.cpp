// Copyright (C) 2018 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "filepropertiesdialog.h"

#include "../coreplugintr.h"
#include "../editormanager/editormanager.h"
#include "../editormanager/ieditorfactory.h"

#include <utils/fileutils.h>
#include <utils/layoutbuilder.h>
#include <utils/mimeutils.h>

#include <QCheckBox>
#include <QDateTime>
#include <QDebug>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QLabel>
#include <QLocale>

using namespace Utils;

namespace Core {

FilePropertiesDialog::FilePropertiesDialog(const FilePath &filePath, QWidget *parent)
    : QDialog(parent)
    , m_name(new QLabel)
    , m_path(new QLabel)
    , m_mimeType(new QLabel)
    , m_defaultEditor(new QLabel)
    , m_lineEndings(new QLabel)
    , m_indentation(new QLabel)
    , m_owner(new QLabel)
    , m_group(new QLabel)
    , m_size(new QLabel)
    , m_lastRead(new QLabel)
    , m_lastModified(new QLabel)
    , m_readable(new QCheckBox)
    , m_writable(new QCheckBox)
    , m_executable(new QCheckBox)
    , m_symLink(new QCheckBox)
    , m_filePath(filePath)
{
    resize(400, 395);

    m_name->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_path->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_mimeType->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_defaultEditor->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lineEndings->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_indentation->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_owner->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_group->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_size->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastRead->setTextInteractionFlags(Qt::TextSelectableByMouse);
    m_lastModified->setTextInteractionFlags(Qt::TextSelectableByMouse);

    m_symLink->setEnabled(false);

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Close);

    using namespace Layouting;
    // clang-format off
    Column {
        Form {
            Tr::tr("Name:"), m_name, br,
            Tr::tr("Path:"), m_path, br,
            Tr::tr("MIME type:"), m_mimeType, br,
            Tr::tr("Default editor:"), m_defaultEditor, br,
            Tr::tr("Line endings:"), m_lineEndings, br,
            Tr::tr("Indentation:"), m_indentation, br,
            Tr::tr("Owner:"), m_owner, br,
            Tr::tr("Group:"), m_group, br,
            Tr::tr("Size:"), m_size, br,
            Tr::tr("Last read:"), m_lastRead, br,
            Tr::tr("Last modified:"), m_lastModified, br,
            Tr::tr("Readable:"), m_readable, br,
            Tr::tr("Writable:"), m_writable, br,
            Tr::tr("Executable:"), m_executable, br,
            Tr::tr("Symbolic link:"), m_symLink, br
        },
        buttonBox
    }.attachTo(this);
    // clang-format on

    connect(m_readable, &QCheckBox::clicked, this, [this](bool checked) {
        setPermission(QFile::ReadUser | QFile::ReadOwner, checked);
    });
    connect(m_writable, &QCheckBox::clicked, this, [this](bool checked) {
        setPermission(QFile::WriteUser | QFile::WriteOwner, checked);
    });
    connect(m_executable, &QCheckBox::clicked, this, [this](bool checked) {
        setPermission(QFile::ExeUser | QFile::ExeOwner, checked);
    });
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    refresh();
}

FilePropertiesDialog::~FilePropertiesDialog() = default;

void FilePropertiesDialog::detectTextFileSettings()
{
    QFile file(m_filePath.toString());
    if (!file.open(QIODevice::ReadOnly)) {
        m_lineEndings->setText(Tr::tr("Unknown"));
        m_indentation->setText(Tr::tr("Unknown"));
        return;
    }

    char lineSeparator = '\n';
    const QByteArray data = file.read(50000);
    file.close();

    // Try to guess the files line endings
    if (data.contains("\r\n")) {
        m_lineEndings->setText(Tr::tr("Windows (CRLF)"));
    } else if (data.contains("\n")) {
        m_lineEndings->setText(Tr::tr("Unix (LF)"));
    } else if (data.contains("\r")) {
        m_lineEndings->setText(Tr::tr("Mac (CR)"));
        lineSeparator = '\r';
    } else {
        // That does not look like a text file at all
        m_lineEndings->setText(Tr::tr("Unknown"));
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
            m_indentation->setText(Tr::tr("Mixed"));
        } else {
            m_indentation->setText(Tr::tr("%1 Spaces").arg(max->first));
        }
    } else if (tabIndented) {
        m_indentation->setText(Tr::tr("Tabs"));
    } else {
        m_indentation->setText(Tr::tr("Unknown"));
    }
}

void FilePropertiesDialog::refresh()
{
    Utils::withNtfsPermissions<void>([this] {
        const QFileInfo fileInfo = m_filePath.toFileInfo();
        QLocale locale;

        m_name->setText(fileInfo.fileName());
        m_path->setText(QDir::toNativeSeparators(fileInfo.canonicalPath()));

        const Utils::MimeType mimeType = Utils::mimeTypeForFile(m_filePath);
        m_mimeType->setText(mimeType.name());

        const EditorFactories factories = IEditorFactory::preferredEditorTypes(m_filePath);
        m_defaultEditor->setText(!factories.isEmpty() ? factories.at(0)->displayName()
                                                      : Tr::tr("Undefined"));

        m_owner->setText(fileInfo.owner());
        m_group->setText(fileInfo.group());
        m_size->setText(locale.formattedDataSize(fileInfo.size()));
        m_readable->setChecked(fileInfo.isReadable());
        m_writable->setChecked(fileInfo.isWritable());
        m_executable->setChecked(fileInfo.isExecutable());
        m_symLink->setChecked(fileInfo.isSymLink());
        m_lastRead->setText(fileInfo.lastRead().toString(locale.dateTimeFormat()));
        m_lastModified->setText(fileInfo.lastModified().toString(locale.dateTimeFormat()));
        if (mimeType.inherits("text/plain")) {
            detectTextFileSettings();
        } else {
            m_lineEndings->setText(Tr::tr("Unknown"));
            m_indentation->setText(Tr::tr("Unknown"));
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
