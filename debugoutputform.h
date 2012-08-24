// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#ifndef __DEBUGOUTPUTFORM_H_
#define __DEBUGOUTPUTFORM_H_

#include <QWidget>
#include <QString>


namespace Ui {
    class DebugOutputForm;
}


class DebugOutputForm : public QWidget
{
    Q_OBJECT

public:
    explicit DebugOutputForm(QWidget* parent = NULL);
    ~DebugOutputForm();

    void print(const QString& msg);

private:
    Ui::DebugOutputForm *ui;

};

#endif // __DEBUGOUTPUTFORM_H_
