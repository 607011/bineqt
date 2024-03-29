// Copyright (c) 2011-2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QFileDialog>
#include <QMessageBox>
#include <QtCore/QDebug>
#include "stereogramsaveform.h"
#include "ui_stereogramsaveform.h"

StereogramSaveForm::StereogramSaveForm(StereogramWidget* stereogramWidget, const QSize& imageSize, QWidget* parent)
    : QDialog(parent)
    , ui(new Ui::StereogramSaveForm)
    , mStereogramWidget(stereogramWidget)
    , mImageSize(imageSize)
{
    ui->setupUi(this);
    ui->widthSpinBox->setValue(mImageSize.width());
    ui->heightSpinBox->setValue(mImageSize.height());
    QObject::connect(ui->saveDepthImagePushButton, SIGNAL(clicked()), SLOT(saveStereogram()));
    QObject::connect(ui->finishedPushButton, SIGNAL(clicked()), SLOT(accept()));
}


StereogramSaveForm::~StereogramSaveForm()
{
    delete ui;
}


void StereogramSaveForm::saveStereogram(void) {
    const QString& stereogramFilename = QFileDialog::getSaveFileName(this, tr("Stereogramm speichern"));
    if (stereogramFilename == "")
        return;
    mChosenImageSize = QSize(ui->widthSpinBox->value(), ui->heightSpinBox->value());
    const QImage& stereogram = mStereogramWidget->stereogram(mChosenImageSize);
    bool success = stereogram.save(stereogramFilename);
    if (success) {
        QMessageBox::information(this, tr("Stereogramm gespeichert"), tr("Stereogramm wurde erfolgreich unter dem Namen '%1' gespeichert.").arg(stereogramFilename));
    }
    else {
        QMessageBox::warning(this, tr("Fehler beim Speichern des Stereogramms"), tr("Stereogramm konnte nicht unter dem Namen '%1' gespeichert werden.").arg(stereogramFilename));
    }
}
