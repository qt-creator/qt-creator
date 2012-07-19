/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
**
** Contact: http://www.qt-project.org/
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
**
**************************************************************************/

#ifndef ANALYZER_VALGRIND_SUPPRESSIONDIALOG_H
#define ANALYZER_VALGRIND_SUPPRESSIONDIALOG_H

#include "xmlprotocol/error.h"

#include <utils/pathchooser.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
class QDialogButtonBox;
QT_END_NAMESPACE

namespace Analyzer { class AnalyzerSettings; }

namespace Valgrind {
namespace Internal {

class MemcheckErrorView;

class SuppressionDialog : public QDialog
{
    Q_OBJECT

public:
    SuppressionDialog(MemcheckErrorView *view,
                      const QList<XmlProtocol::Error> &errors);
    static void maybeShow(MemcheckErrorView *view);

private slots:
    void validate();

private:
    void accept();
    void reject();

    MemcheckErrorView *m_view;
    Analyzer::AnalyzerSettings *m_settings;
    bool m_cleanupIfCanceled;
    QList<XmlProtocol::Error> m_errors;

    Utils::PathChooser *m_fileChooser;
    QPlainTextEdit *m_suppressionEdit;
    QDialogButtonBox *m_buttonBox;
};

} // namespace Internal
} // namespace Valgrind

#endif // ANALYZER_VALGRIND_SUPPRESSIONDIALOG_H
