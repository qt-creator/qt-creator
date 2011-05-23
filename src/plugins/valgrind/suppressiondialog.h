/**************************************************************************
**
** This file is part of Qt Creator Instrumentation Tools
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Author: Milian Wolff, KDAB (milian.wolff@kdab.com)
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
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef ANALYZER_VALGRIND_INTERNAL_SUPPRESSIONDIALOG_H
#define ANALYZER_VALGRIND_INTERNAL_SUPPRESSIONDIALOG_H

#include <QtGui/QDialog>

#include <valgrind/xmlprotocol/error.h>

namespace Analyzer {
class AnalyzerSettings;
}

namespace Valgrind {
namespace Internal {

namespace Ui {
class SuppressionDialog;
}

class MemcheckErrorView;

class SuppressionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SuppressionDialog(MemcheckErrorView *view, QWidget *parent = 0,
                               Qt::WindowFlags f = 0);
    virtual void accept();
    virtual void reject();

    bool shouldShow() const;

private slots:
    void validate();

private:
    MemcheckErrorView *m_view;
    Ui::SuppressionDialog *m_ui;
    Analyzer::AnalyzerSettings *m_settings;
    bool m_cleanupIfCanceled;
    QList<Valgrind::XmlProtocol::Error> m_errors;
};

} // namespace Internal
} // namespace Valgrind

#endif // ANALYZER_VALGRIND_INTERNAL_SUPPRESSIONDIALOG_H
