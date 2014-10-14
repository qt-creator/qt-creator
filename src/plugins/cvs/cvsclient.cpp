/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cvsclient.h"
#include "cvssettings.h"
#include "cvsconstants.h"

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <utils/synchronousprocess.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;

namespace Cvs {
namespace Internal {

class CvsDiffExitCodeInterpreter : public ExitCodeInterpreter
{
    Q_OBJECT
public:
    CvsDiffExitCodeInterpreter(QObject *parent) : ExitCodeInterpreter(parent) {}
    SynchronousProcessResponse::Result interpretExitCode(int code) const;

};

SynchronousProcessResponse::Result CvsDiffExitCodeInterpreter::interpretExitCode(int code) const
{
    if (code < 0 || code > 2)
        return SynchronousProcessResponse::FinishedError;
    return SynchronousProcessResponse::Finished;
}

// Collect all parameters required for a diff to be able to associate them
// with a diff editor and re-run the diff with parameters.
struct CvsDiffParameters
{
    QString workingDir;
    QStringList extraOptions;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
class CvsDiffParameterWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT
public:
    explicit CvsDiffParameterWidget(CvsClient *client,
                                    const CvsDiffParameters &p,
                                    QWidget *parent = 0);
    QStringList arguments() const;
    void executeCommand();

private:

    CvsClient *m_client;
    const CvsDiffParameters m_params;
};

CvsDiffParameterWidget::CvsDiffParameterWidget(CvsClient *client,
                                               const CvsDiffParameters &p,
                                               QWidget *parent)
    : VcsBaseEditorParameterWidget(parent), m_client(client), m_params(p)
{
    mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace")),
               client->settings()->boolPointer(CvsSettings::diffIgnoreWhiteSpaceKey));
    mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore Blank Lines")),
               client->settings()->boolPointer(CvsSettings::diffIgnoreBlankLinesKey));
}

QStringList CvsDiffParameterWidget::arguments() const
{
    QStringList args;
    args = m_client->settings()->stringValue(CvsSettings::diffOptionsKey).split(QLatin1Char(' '), QString::SkipEmptyParts);
    args += VcsBaseEditorParameterWidget::arguments();
    return args;
}

void CvsDiffParameterWidget::executeCommand()
{
    m_client->diff(m_params.workingDir, m_params.files, m_params.extraOptions);
}

CvsClient::CvsClient(CvsSettings *settings) :
    VcsBaseClient(settings)
{
}

CvsSettings *CvsClient::settings() const
{
    return dynamic_cast<CvsSettings *>(VcsBaseClient::settings());
}

Core::Id CvsClient::vcsEditorKind(VcsCommandTag cmd) const
{
    switch (cmd) {
    case DiffCommand:
        return "CVS Diff Editor"; // TODO: replace by string from cvsconstants.h
    default:
        return Core::Id();
    }
}

ExitCodeInterpreter *CvsClient::exitCodeInterpreter(VcsCommandTag cmd, QObject *parent) const
{
    switch (cmd) {
    case DiffCommand:
        return new CvsDiffExitCodeInterpreter(parent);
    default:
        return 0;
    }
}

void CvsClient::diff(const QString &workingDir, const QStringList &files,
                     const QStringList &extraOptions)
{
    VcsBaseClient::diff(workingDir, files, extraOptions);
}

QString CvsClient::findTopLevelForFile(const QFileInfo &file) const
{
    Q_UNUSED(file)
    return QString();
}

QStringList CvsClient::revisionSpec(const QString &revision) const
{
    Q_UNUSED(revision)
    return QStringList();
}

VcsBaseClient::StatusItem CvsClient::parseStatusLine(const QString &line) const
{
    Q_UNUSED(line)
    return VcsBaseClient::StatusItem();
}

VcsBaseEditorParameterWidget *CvsClient::createDiffEditor(
        const QString &workingDir, const QStringList &files, const QStringList &extraOptions)
{
    Q_UNUSED(extraOptions)
    CvsDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.extraOptions = extraOptions;
    return new CvsDiffParameterWidget(this, p);
}

} // namespace Internal
} // namespace Cvs

#include "cvsclient.moc"
