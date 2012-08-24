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
    explicit DebugOutputForm(QWidget *parent = 0);
    ~DebugOutputForm();

    void print(const QString& msg);

private:
    Ui::DebugOutputForm *ui;

};

#endif // DEBUGOUTPUTFORM_H
