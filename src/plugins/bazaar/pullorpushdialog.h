/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef PULLORPUSHDIALOG_H
#define PULLORPUSHDIALOG_H

#include <QtGui/QDialog>

namespace Bazaar {
namespace Internal {

namespace Ui {
class PullOrPushDialog;
}

class PullOrPushDialog : public QDialog
{
    Q_OBJECT
public:
    enum Mode {
        PullMode,
        PushMode
    };

    PullOrPushDialog(Mode mode, QWidget *parent = 0);
    ~PullOrPushDialog();

    // Common parameters and options
    QString branchLocation() const;
    bool isRememberOptionEnabled() const;
    bool isOverwriteOptionEnabled() const;
    QString revision() const;
    // Pull-specific options
    bool isLocalOptionEnabled() const;
    // Push-specific options
    bool isUseExistingDirectoryOptionEnabled() const;
    bool isCreatePrefixOptionEnabled() const;

protected:
    void changeEvent(QEvent *e);

private:
    Mode m_mode;
    Ui::PullOrPushDialog *m_ui;
};

} // namespace Internal
} // namespace Bazaar

#endif // PULLORPUSHDIALOG_H
