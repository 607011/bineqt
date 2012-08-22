// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __MAILADDRESSDIALOG_H_
#define __MAILADDRESSDIALOG_H_

#include <QDialog>

namespace Ui {
    class MailAddressDialog;
}

class MailAddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit MailAddressDialog(QWidget* parent = NULL);
    ~MailAddressDialog();

    QString getAddress(void);
    void setFileName(const QString& hint);

private:
    Ui::MailAddressDialog *ui;
};

#endif // __MAILADDRESSDIALOG_H_
