// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "mailaddressdialog.h"
#include "ui_mailaddressdialog.h"

MailAddressDialog::MailAddressDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::MailAddressDialog)
{
    ui->setupUi(this);
}


MailAddressDialog::~MailAddressDialog()
{
    delete ui;
}


QString MailAddressDialog::getAddress(void)
{
    return ui->mailLineEdit->text();
}
