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

#ifndef BASEEDITORDOCUMENTPARSER_H
#define BASEEDITORDOCUMENTPARSER_H

#include "cpptools_global.h"
#include "cppworkingcopy.h"
#include "projectpart.h"

#include <QObject>
#include <QMutex>

namespace CppTools {

class CPPTOOLS_EXPORT BaseEditorDocumentParser : public QObject
{
    Q_OBJECT

public:
    using Ptr = QSharedPointer<BaseEditorDocumentParser>;;
    static Ptr get(const QString &filePath);

    struct Configuration {
        bool stickToPreviousProjectPart = true;
        bool usePrecompiledHeaders = false;
        QByteArray editorDefines;
        ProjectPart::Ptr manuallySetProjectPart;
    };

public:
    BaseEditorDocumentParser(const QString &filePath);
    virtual ~BaseEditorDocumentParser();

    QString filePath() const;
    Configuration configuration() const;
    void setConfiguration(const Configuration &configuration);

    void update(const WorkingCopy &workingCopy);

    ProjectPart::Ptr projectPart() const;

protected:
    struct State {
        QByteArray editorDefines;
        ProjectPart::Ptr projectPart;
    };
    State state() const;
    void setState(const State &state);

    static ProjectPart::Ptr determineProjectPart(const QString &filePath,
                                                 const Configuration &config,
                                                 const State &state);

    mutable QMutex m_stateAndConfigurationMutex;

private:
    virtual void updateHelper(const WorkingCopy &workingCopy) = 0;

    const QString m_filePath;
    Configuration m_configuration;
    State m_state;
    mutable QMutex m_updateIsRunning;
};

} // namespace CppTools

#endif // BASEEDITORDOCUMENTPARSER_H
