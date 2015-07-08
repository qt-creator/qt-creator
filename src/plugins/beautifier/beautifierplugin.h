/**************************************************************************
**
** Copyright (C) 2015 Lorenz Haas
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

#ifndef BEAUTIFIER_BEAUTIFIER_H
#define BEAUTIFIER_BEAUTIFIER_H

#include "command.h"

#include <extensionsystem/iplugin.h>

#include <QFutureInterface>
#include <QPlainTextEdit>
#include <QPointer>
#include <QSignalMapper>

namespace Core { class IEditor; }

namespace Beautifier {
namespace Internal {

class BeautifierAbstractTool;

struct FormatTask
{
    FormatTask(QPlainTextEdit *_editor, const QString &_filePath, const QString &_sourceData,
               const Command &_command, int _startPos = -1, int _endPos = 0) :
        editor(_editor),
        filePath(_filePath),
        sourceData(_sourceData),
        command(_command),
        startPos(_startPos),
        endPos(_endPos),
        timeout(false) {}

    QPointer<QPlainTextEdit> editor;
    QString filePath;
    QString sourceData;
    Command command;
    int startPos;
    int endPos;
    bool timeout;
    QString formattedData;
};

class BeautifierPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "Beautifier.json")

public:
    BeautifierPlugin();
    ~BeautifierPlugin();
    bool initialize(const QStringList &arguments, QString *errorString) override;
    void extensionsInitialized() override;
    ShutdownFlag aboutToShutdown() override;

    QString format(const QString &text, const Command &command, const QString &fileName,
                   bool *timeout = 0);
    void formatCurrentFile(const Command &command, int startPos = -1, int endPos = 0);
    void formatAsync(QFutureInterface<FormatTask> &future, FormatTask task);

    static QString msgCannotGetConfigurationFile(const QString &command);
    static QString msgFormatCurrentFile();
    static QString msgFormatSelectedText();
    static QString msgCommandPromptDialogTitle(const QString &command);

public slots:
    static void showError(const QString &error);

private slots:
    void updateActions(Core::IEditor *editor = 0);
    void formatCurrentFileContinue(QObject *watcher = 0);

signals:
    void pipeError(QString);

private:
    QList<BeautifierAbstractTool *> m_tools;
    QSignalMapper *m_asyncFormatMapper;
};

} // namespace Internal
} // namespace Beautifier

#endif // BEAUTIFIER_BEAUTIFIER_H

