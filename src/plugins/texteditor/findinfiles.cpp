// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinfiles.h"

#include "texteditortr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/icore.h>

#include <utils/historycompleter.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QComboBox>
#include <QGridLayout>
#include <QLabel>
#include <QStackedWidget>

using namespace Core;
using namespace Utils;

namespace TextEditor {

static FindInFiles *m_instance = nullptr;
static const char HistoryKey[] = "FindInFiles.Directories.History";

FindInFiles::FindInFiles()
{
    m_instance = this;
    connect(EditorManager::instance(), &EditorManager::findOnFileSystemRequest,
            this, &FindInFiles::findOnFileSystem);
}

FindInFiles::~FindInFiles() = default;

bool FindInFiles::isValid() const
{
    return m_isValid;
}

QString FindInFiles::id() const
{
    return QLatin1String("Files on Disk");
}

QString FindInFiles::displayName() const
{
    return Tr::tr("Files in File System");
}

FileContainerProvider FindInFiles::fileContainerProvider() const
{
    return [nameFilters = fileNameFilters(), exclusionFilters = fileExclusionFilters(),
            filePath = searchDir()] {
        return SubDirFileContainer({filePath}, nameFilters, exclusionFilters,
                                   EditorManager::defaultTextCodec());
    };
}

QString FindInFiles::label() const
{
    QString title = currentSearchEngine()->title();

    const QChar slash = QLatin1Char('/');
    const QStringList &nonEmptyComponents = searchDir().toFileInfo().absoluteFilePath()
            .split(slash, Qt::SkipEmptyParts);
    return Tr::tr("%1 \"%2\":")
            .arg(title)
            .arg(nonEmptyComponents.isEmpty() ? QString(slash) : nonEmptyComponents.last());
}

QString FindInFiles::toolTip() const
{
    //: the last arg is filled by BaseFileFind::runNewSearch
    QString tooltip = Tr::tr("Path: %1\nFilter: %2\nExcluding: %3\n%4")
            .arg(searchDir().toUserOutput())
            .arg(fileNameFilters().join(','))
            .arg(fileExclusionFilters().join(','));

    const QString searchEngineToolTip = currentSearchEngine()->toolTip();
    if (!searchEngineToolTip.isEmpty())
        tooltip = tooltip.arg(searchEngineToolTip);

    return tooltip;
}

void FindInFiles::syncSearchEngineCombo(int selectedSearchEngineIndex)
{
    QTC_ASSERT(m_searchEngineCombo && selectedSearchEngineIndex >= 0
               && selectedSearchEngineIndex < searchEngines().size(), return);

    m_searchEngineCombo->setCurrentIndex(selectedSearchEngineIndex);
}

void FindInFiles::setValid(bool valid)
{
    if (valid == m_isValid)
        return;
    m_isValid = valid;
    emit validChanged(m_isValid);
}

void FindInFiles::searchEnginesSelectionChanged(int index)
{
    setCurrentSearchEngine(index);
    m_searchEngineWidget->setCurrentIndex(index);
}

QWidget *FindInFiles::createConfigWidget()
{
    if (!m_configWidget) {
        m_configWidget = new QWidget;
        auto gridLayout = new QGridLayout(m_configWidget);
        gridLayout->setContentsMargins(0, 0, 0, 0);
        m_configWidget->setLayout(gridLayout);

        int row = 0;
        auto searchEngineLabel = new QLabel(Tr::tr("Search engine:"));
        gridLayout->addWidget(searchEngineLabel, row, 0, Qt::AlignRight);
        m_searchEngineCombo = new QComboBox;
        connect(m_searchEngineCombo, &QComboBox::currentIndexChanged,
                this, &FindInFiles::searchEnginesSelectionChanged);
        searchEngineLabel->setBuddy(m_searchEngineCombo);
        gridLayout->addWidget(m_searchEngineCombo, row, 1);

        m_searchEngineWidget = new QStackedWidget(m_configWidget);
        const QVector<SearchEngine *> searchEngineVector = searchEngines();
        for (const SearchEngine *searchEngine : searchEngineVector) {
            m_searchEngineWidget->addWidget(searchEngine->widget());
            m_searchEngineCombo->addItem(searchEngine->title());
        }
        gridLayout->addWidget(m_searchEngineWidget, row++, 2);

        QLabel *dirLabel = new QLabel(Tr::tr("Director&y:"));
        gridLayout->addWidget(dirLabel, row, 0, Qt::AlignRight);
        m_directory = new PathChooser;
        m_directory->setExpectedKind(PathChooser::ExistingDirectory);
        m_directory->setPromptDialogTitle(Tr::tr("Directory to Search"));
        connect(m_directory.data(), &PathChooser::textChanged, this,
                [this] { setSearchDir(m_directory->filePath()); });
        connect(this, &BaseFileFind::searchDirChanged, m_directory, &PathChooser::setFilePath);
        m_directory->setHistoryCompleter(HistoryKey, /*restoreLastItemFromHistory=*/ true);
        if (!HistoryCompleter::historyExistsFor(HistoryKey)) {
            auto completer = static_cast<HistoryCompleter *>(m_directory->lineEdit()->completer());
            const QStringList legacyHistory = ICore::settings()->value(
                        "Find/FindInFiles/directories").toStringList();
            for (const QString &dir: legacyHistory)
                completer->addEntry(dir);
        }
        dirLabel->setBuddy(m_directory);
        gridLayout->addWidget(m_directory, row++, 1, 1, 2);

        const QList<QPair<QWidget *, QWidget *>> patternWidgets = createPatternWidgets();
        for (const QPair<QWidget *, QWidget *> &p : patternWidgets) {
            gridLayout->addWidget(p.first, row, 0, Qt::AlignRight);
            gridLayout->addWidget(p.second, row, 1, 1, 2);
            ++row;
        }
        m_configWidget->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        // validity
        auto updateValidity = [this] {
            setValid(currentSearchEngine()->isEnabled() && m_directory->isValid());
        };
        connect(this, &BaseFileFind::currentSearchEngineChanged, this, updateValidity);
        for (const SearchEngine *searchEngine : searchEngineVector)
            connect(searchEngine, &SearchEngine::enabledChanged, this, updateValidity);
        connect(m_directory.data(), &PathChooser::validChanged, this, updateValidity);
        updateValidity();
    }
    return m_configWidget;
}

void FindInFiles::writeSettings(QtcSettings *settings)
{
    settings->beginGroup("FindInFiles");
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInFiles::readSettings(QtcSettings *settings)
{
    settings->beginGroup("FindInFiles");
    readCommonSettings(settings, "*.cpp,*.h", "*/.git/*,*/.cvs/*,*/.svn/*,*.autosave");
    settings->endGroup();
}

void FindInFiles::setBaseDirectory(const FilePath &directory)
{
    m_directory->setBaseDirectory(directory);
}

void FindInFiles::findOnFileSystem(const QString &path)
{
    QTC_ASSERT(m_instance, return);
    const QFileInfo fi(path);
    const QString folder = fi.isDir() ? fi.absoluteFilePath() : fi.absolutePath();
    m_instance->setSearchDir(FilePath::fromString(folder));
    Find::openFindDialog(m_instance);
}

FindInFiles *FindInFiles::instance()
{
    return m_instance;
}

} // TextEditor
