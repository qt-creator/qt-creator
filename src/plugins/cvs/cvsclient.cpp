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

#include "cvsclient.h"
#include "cvssettings.h"

#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditorconfig.h>

#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QDebug>

using namespace Utils;
using namespace VcsBase;

namespace Cvs {
namespace Internal {

// Parameter widget controlling whitespace diff mode, associated with a parameter
class CvsDiffConfig : public VcsBaseEditorConfig
{
    Q_OBJECT
public:
    CvsDiffConfig(VcsBaseClientSettings &settings, QToolBar *toolBar);
    QStringList arguments() const;

private:
    VcsBaseClientSettings &m_settings;
};

CvsDiffConfig::CvsDiffConfig(VcsBaseClientSettings &settings, QToolBar *toolBar) :
    VcsBaseEditorConfig(toolBar),
    m_settings(settings)
{
    mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace")),
               settings.boolPointer(CvsSettings::diffIgnoreWhiteSpaceKey));
    mapSetting(addToggleButton(QLatin1String("-B"), tr("Ignore Blank Lines")),
               settings.boolPointer(CvsSettings::diffIgnoreBlankLinesKey));
}

QStringList CvsDiffConfig::arguments() const
{
    QStringList args;
    args = m_settings.stringValue(CvsSettings::diffOptionsKey).split(QLatin1Char(' '),
                                                                     QString::SkipEmptyParts);
    args += VcsBaseEditorConfig::arguments();
    return args;
}

CvsClient::CvsClient() : VcsBaseClient(new CvsSettings)
{
    setDiffConfigCreator([this](QToolBar *toolBar) {
        return new CvsDiffConfig(settings(), toolBar);
    });
}

CvsSettings &CvsClient::settings() const
{
    return static_cast<CvsSettings &>(VcsBaseClient::settings());
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

ExitCodeInterpreter CvsClient::exitCodeInterpreter(VcsCommandTag cmd) const
{
    if (cmd == DiffCommand) {
        return [](int code) {
            return (code < 0 || code > 2) ? SynchronousProcessResponse::FinishedError
                                          : SynchronousProcessResponse::Finished;
        };
    }
    return Utils::defaultExitCodeInterpreter;
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

} // namespace Internal
} // namespace Cvs

#include "cvsclient.moc"
