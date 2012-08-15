// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

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
    const QSize& chosenImageSize(void) const { return mChosenImageSize; }

private:
    Ui::StereogramSaveForm *ui;
    StereogramWidget* mStereogramWidget;
    QSize mImageSize;
    QSize mChosenImageSize;

private slots:
    void saveStereogram(void);

};

#endif // __STEREOGRAMSAVEFORM_H_
