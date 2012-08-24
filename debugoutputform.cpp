// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include "debugoutputform.h"
#include "ui_debugoutputform.h"

DebugOutputForm::DebugOutputForm(QWidget* parent)
    : QWidget(parent)
    , ui(new Ui::DebugOutputForm)
{
    setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint);
    ui->setupUi(this);
    QObject::connect(ui->clearPushButton, SIGNAL(clicked()), ui->debugPlainTextEdit, SLOT(clear()));
}


DebugOutputForm::~DebugOutputForm()
{
    delete ui;
}


void DebugOutputForm::print(const QString& msg)
{
    ui->debugPlainTextEdit->appendPlainText(msg);
}
