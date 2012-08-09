// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: stereogramsaveform.h da5b9776da65 2012/06/06 14:36:01 Oliver Lau <oliver@von-und-fuer-lau.de> $

#ifndef __STEREOGRAMSAVEFORM_H_
#define __STEREOGRAMSAVEFORM_H_

#include <QDialog>

#include "stereogramwidget.h"

namespace Ui {
    class StereogramSaveForm;
}

class StereogramSaveForm : public QDialog
{
    Q_OBJECT

public:
    explicit StereogramSaveForm(StereogramWidget* stereogramWidget, const QSize& imageSize, QWidget* parent = NULL);
    ~StereogramSaveForm();

private:
    Ui::StereogramSaveForm *ui;
    StereogramWidget* mStereogramWidget;
    QSize mImageSize;

private slots:
    void saveStereogram(void);

};

#endif // __STEREOGRAMSAVEFORM_H_
