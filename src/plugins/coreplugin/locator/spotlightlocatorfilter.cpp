/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "spotlightlocatorfilter.h"

#include "../messagemanager.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/reaper.h>
#include <utils/algorithm.h>
#include <utils/environment.h>
#include <utils/fancylineedit.h>
#include <utils/macroexpander.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/stringutils.h>
#include <utils/variablechooser.h>

#include <QFormLayout>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMutex>
#include <QMutexLocker>
#include <QProcess>
#include <QRegularExpression>
#include <QWaitCondition>

using namespace Utils;

namespace Core {
namespace Internal {

// #pragma mark -- SpotlightIterator

class SpotlightIterator : public BaseFileFilter::Iterator
{
public:
    SpotlightIterator(const QStringList &command);
    ~SpotlightIterator() override;

    void toFront() override;
    bool hasNext() const override;
    Utils::FilePath next() override;
    Utils::FilePath filePath() const override;

    void scheduleKillProcess();
    void killProcess();

private:
    void ensureNext();

    std::unique_ptr<QProcess> m_process;
    QMutex m_mutex;
    QWaitCondition m_waitForItems;
    QList<FilePath> m_queue;
    QList<FilePath> m_filePaths;
    int m_index;
    bool m_finished;
};

SpotlightIterator::SpotlightIterator(const QStringList &command)
    : m_index(-1)
    , m_finished(false)
{
    QTC_ASSERT(!command.isEmpty(), return );
    m_process.reset(new QProcess);
    m_process->setProgram(
        Utils::Environment::systemEnvironment().searchInPath(command.first()).toString());
    m_process->setArguments(command.mid(1));
    m_process->setProcessEnvironment(Utils::Environment::systemEnvironment().toProcessEnvironment());
    QObject::connect(m_process.get(),
                     QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                     [this] { scheduleKillProcess(); });
    QObject::connect(m_process.get(), &QProcess::errorOccurred, [this, command] {
        MessageManager::writeFlashing(
            SpotlightLocatorFilter::tr("Locator: Error occurred when running \"%1\".")
                .arg(command.first()));
        scheduleKillProcess();
    });
    QObject::connect(m_process.get(), &QProcess::readyReadStandardOutput, [this] {
        QString output = QString::fromUtf8(m_process->readAllStandardOutput());
        output.replace("\r\n", "\n");
        const QStringList items = output.split('\n');
        QMutexLocker lock(&m_mutex);
        m_queue.append(Utils::transform(items, &FilePath::fromUserInput));
        if (m_filePaths.size() + m_queue.size() > 10000) // limit the amount of data
            scheduleKillProcess();
        m_waitForItems.wakeAll();
    });
    m_process->start(QIODevice::ReadOnly);
}

SpotlightIterator::~SpotlightIterator()
{
    killProcess();
}

void SpotlightIterator::toFront()
{
    m_index = -1;
}

bool SpotlightIterator::hasNext() const
{
    auto that = const_cast<SpotlightIterator *>(this);
    that->ensureNext();
    return (m_index + 1 < m_filePaths.size());
}

Utils::FilePath SpotlightIterator::next()
{
    ensureNext();
    ++m_index;
    QTC_ASSERT(m_index < m_filePaths.size(), return FilePath());
    return m_filePaths.at(m_index);
}

Utils::FilePath SpotlightIterator::filePath() const
{
    QTC_ASSERT(m_index < m_filePaths.size(), return FilePath());
    return m_filePaths.at(m_index);
}

void SpotlightIterator::scheduleKillProcess()
{
    QMetaObject::invokeMethod(m_process.get(), [this] { killProcess(); }, Qt::QueuedConnection);
}

void SpotlightIterator::killProcess()
{
    if (!m_process)
        return;
    m_process->disconnect();
    QMutexLocker lock(&m_mutex);
    m_finished = true;
    m_waitForItems.wakeAll();
    if (m_process->state() == QProcess::NotRunning)
        m_process.reset();
    else
        Reaper::reap(m_process.release());
}

void SpotlightIterator::ensureNext()
{
    if (m_index + 1 < m_filePaths.size()) // nothing to do
        return;
    // check if there are items in the queue, otherwise wait for some
    QMutexLocker lock(&m_mutex);
    if (m_queue.isEmpty() && !m_finished)
        m_waitForItems.wait(&m_mutex);
    m_filePaths.append(m_queue);
    m_queue.clear();
}

// #pragma mark -- SpotlightLocatorFilter

static QString defaultCommand()
{
    if (HostOsInfo::isMacHost())
        return "mdfind";
    if (HostOsInfo::isWindowsHost())
        return "es.exe";
    return "locate";
}

static QString defaultArguments()
{
    if (HostOsInfo::isMacHost())
        return "\"kMDItemFSName = '*%{Query:Escaped}*'c\"";
    if (HostOsInfo::isWindowsHost())
        return "-n 10000 -r \"%{Query:Regex}\"";
    return "-i -l 10000 -r \"%{Query:Regex}\"";
}

static QString defaultCaseSensitiveArguments()
{
    if (HostOsInfo::isMacHost())
        return "\"kMDItemFSName = '*%{Query:Escaped}*'\"";
    if (HostOsInfo::isWindowsHost())
        return "-i -n 10000 -r \"%{Query:Regex}\"";
    return "-l 10000 -r \"%{Query:Regex}\"";
}

const char kCommandKey[] = "command";
const char kArgumentsKey[] = "arguments";
const char kCaseSensitiveKey[] = "caseSensitive";

static MacroExpander *createMacroExpander(const QString &query)
{
    MacroExpander *expander = new MacroExpander;
    expander->registerVariable("Query",
                               SpotlightLocatorFilter::tr("Locator query string."),
                               [query] { return query; });
    expander->registerVariable("Query:Escaped",
                               SpotlightLocatorFilter::tr(
                                   "Locator query string with quotes escaped with backslash."),
                               [query] {
                                   QString quoted = query;
                                   quoted.replace('\\', "\\\\")
                                       .replace('\'', "\\\'")
                                       .replace('\"', "\\\"");
                                   return quoted;
                               });
    expander->registerVariable("Query:Regex",
                               SpotlightLocatorFilter::tr(
                                   "Locator query string as regular expression."),
                               [query] {
                                   QString regex = query;
                                   regex = regex.replace('*', ".*");
                                   return regex;
                               });
    return expander;
}

SpotlightLocatorFilter::SpotlightLocatorFilter()
{
    setId("SpotlightFileNamesLocatorFilter");
    setDefaultShortcutString("md");
    setDefaultIncludedByDefault(false);
    setDisplayName(tr("File Name Index"));
    setConfigurable(true);
    reset();
}

void SpotlightLocatorFilter::prepareSearch(const QString &entry)
{
    const EditorManager::FilePathInfo fp = EditorManager::splitLineAndColumnNumber(entry);
    if (fp.filePath.isEmpty()) {
        setFileIterator(new BaseFileFilter::ListIterator(Utils::FilePaths()));
    } else {
        // only pass the file name part to allow searches like "somepath/*foo"
        int lastSlash = fp.filePath.lastIndexOf(QLatin1Char('/'));
        const QString query = fp.filePath.mid(lastSlash + 1);
        std::unique_ptr<MacroExpander> expander(createMacroExpander(query));
        const QString argumentString = expander->expand(
            caseSensitivity(fp.filePath) == Qt::CaseInsensitive ? m_arguments
                                                                : m_caseSensitiveArguments);
        setFileIterator(
            new SpotlightIterator(QStringList(m_command) + QtcProcess::splitArgs(argumentString)));
    }
    BaseFileFilter::prepareSearch(entry);
}

bool SpotlightLocatorFilter::openConfigDialog(QWidget *parent, bool &needsRefresh)
{
    Q_UNUSED(needsRefresh)
    QWidget configWidget;
    QFormLayout *layout = new QFormLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    configWidget.setLayout(layout);
    PathChooser *commandEdit = new PathChooser;
    commandEdit->setExpectedKind(PathChooser::ExistingCommand);
    commandEdit->lineEdit()->setText(m_command);
    FancyLineEdit *argumentsEdit = new FancyLineEdit;
    argumentsEdit->setText(m_arguments);
    FancyLineEdit *caseSensitiveArgumentsEdit = new FancyLineEdit;
    caseSensitiveArgumentsEdit->setText(m_caseSensitiveArguments);
    layout->addRow(tr("Executable:"), commandEdit);
    layout->addRow(tr("Arguments:"), argumentsEdit);
    layout->addRow(tr("Case sensitive:"), caseSensitiveArgumentsEdit);
    std::unique_ptr<MacroExpander> expander(createMacroExpander(""));
    auto chooser = new VariableChooser(&configWidget);
    chooser->addMacroExpanderProvider([expander = expander.get()] { return expander; });
    chooser->addSupportedWidget(argumentsEdit);
    chooser->addSupportedWidget(caseSensitiveArgumentsEdit);
    const bool accepted = openConfigDialog(parent, &configWidget);
    if (accepted) {
        m_command = commandEdit->rawFilePath().toString();
        m_arguments = argumentsEdit->text();
        m_caseSensitiveArguments = caseSensitiveArgumentsEdit->text();
    }
    return accepted;
}

void SpotlightLocatorFilter::saveState(QJsonObject &obj) const
{
    if (m_command != defaultCommand())
        obj.insert(kCommandKey, m_command);
    if (m_arguments != defaultArguments())
        obj.insert(kArgumentsKey, m_arguments);
    if (m_caseSensitiveArguments != defaultCaseSensitiveArguments())
        obj.insert(kCaseSensitiveKey, m_caseSensitiveArguments);
}

void SpotlightLocatorFilter::restoreState(const QJsonObject &obj)
{
    m_command = obj.value(kCommandKey).toString(defaultCommand());
    m_arguments = obj.value(kArgumentsKey).toString(defaultArguments());
    m_caseSensitiveArguments = obj.value(kCaseSensitiveKey).toString(defaultCaseSensitiveArguments());
}

void SpotlightLocatorFilter::reset()
{
    m_command = defaultCommand();
    m_arguments = defaultArguments();
    m_caseSensitiveArguments = defaultCaseSensitiveArguments();
}

} // Internal
} // Core
