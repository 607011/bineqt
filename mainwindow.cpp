// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.
// $Id: mainwindow.cpp 9a383954a478 2012/08/08 14:55:58 Oliver Lau <oliver@von-und-fuer-lau.de> $

#include <QSettings>
#include <QFileDialog>
#include <QMessageBox>
#include <QtCore/QtDebug>

#include "nui.h"
#include "mainwindow.h"
#include "stereogramsaveform.h"
#include "ui_mainwindow.h"

#ifdef _OPENMP
#include <omp.h>
#endif


const QString MainWindow::Company = "ct.de";
const QString MainWindow::AppName = QObject::tr("Live Autostereogramm");
#ifndef QT_NO_DEBUG
const QString MainWindow::AppVersion = "0.1 DEBUG $Date: 2012/08/08 14:55:58 $";
#else
const QString MainWindow::AppVersion = "0.1";
#endif


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mDepthFrameFrozen(false)
{
    QCoreApplication::setOrganizationName(MainWindow::Company);
    QCoreApplication::setOrganizationDomain(MainWindow::Company);
    QCoreApplication::setApplicationName(MainWindow::AppName);
    QSettings::setDefaultFormat(QSettings::NativeFormat);

    ui->setupUi(this);
    setWindowTitle(tr("%1 %2 %3").arg(MainWindow::AppName).arg(MainWindow::AppVersion).arg(_OPENMP));

    ui->modeComboBox->addItem(tr("Kacheln"), QVariant(StereogramWidget::TileTexture));
    ui->modeComboBox->addItem(tr("Aufspannen"), QVariant(StereogramWidget::StretchTexture));
    ui->modeComboBox->addItem(tr("Zufällig, Farbe"), QVariant(StereogramWidget::RandomColor));
    ui->modeComboBox->addItem(tr("Zufällig, S/W"), QVariant(StereogramWidget::RandomBlackAndWhite));

    ui->stereogramSizeComboBox->addItem(tr("480 Zeilen"), 480);
    ui->stereogramSizeComboBox->addItem(tr("720 Zeilen"), 720);
    ui->stereogramSizeComboBox->addItem(tr("1024 Zeilen"), 1024);
    ui->stereogramSizeComboBox->addItem(tr("1080 Zeilen"), 1080);
    ui->stereogramSizeComboBox->addItem(tr("1280 Zeilen"), 1280);
    ui->stereogramSizeComboBox->addItem(tr("DIN A4, 150 dpi"), 1240);
    ui->stereogramSizeComboBox->addItem(tr("DIN A3, 150 dpi"), 1753);
    ui->stereogramSizeComboBox->addItem(tr("DIN A2, 150 dpi"), 2480);
    ui->stereogramSizeComboBox->addItem(tr("DIN A1, 150 dpi"), 3507);

    mDepthWidget = new DepthImageWidget;

    ui->horizontalLayout->addWidget(mDepthWidget);
    QObject::connect(&mNUIThread, SIGNAL(depthDataReady(const quint16*, const QSize&)), mDepthWidget, SLOT(setDepthData(const quint16*, const QSize&)));
    QObject::connect(&mNUIThread, SIGNAL(videoFrameReady(QImage)), mDepthWidget, SLOT(setFrame(QImage)));

    mStereogramWidget = new StereogramWidget;
    mStereogramWidget->setRequestedStereogramSize(QSize(640, 480));

    QObject::connect(mStereogramWidget, SIGNAL(attaching()), SLOT(attachStereogramWidget()));
    QObject::connect(mStereogramWidget, SIGNAL(detaching()), SLOT(detachStereogramWidget()));

    QObject::connect(&mNUIThread, SIGNAL(depthDataReady(const quint16*, const QSize&)), mStereogramWidget, SLOT(setDepthData(const quint16*, const QSize&)));
    QObject::connect(&mNUIThread, SIGNAL(videoFrameReady(QImage)), mStereogramWidget, SLOT(setFrame(QImage)));

    QObject::connect(mDepthWidget, SIGNAL(selectionChanged(QImage)), SLOT(setSelection(QImage)));
    QObject::connect(mDepthWidget, SIGNAL(stopStreaming()), SLOT(freezeToggled()), Qt::DirectConnection);
    QObject::connect(mDepthWidget, SIGNAL(depthDataReady(const quint16*, const QSize&)), mStereogramWidget, SLOT(setDepthData(const quint16*, const QSize&)), Qt::DirectConnection);
    QObject::connect(mDepthWidget, SIGNAL(clippingChanged(int, int)), mStereogramWidget, SLOT(setClipping(int, int)), Qt::DirectConnection);

    QObject::connect(ui->actionSaveDepthImageAs, SIGNAL(triggered()), SLOT(saveDepthImage()));
    QObject::connect(ui->actionSaveStereogramAs, SIGNAL(triggered()), SLOT(saveStereogram()));
    QObject::connect(ui->actionOpenTexture, SIGNAL(triggered()), SLOT(openTexture()));
    QObject::connect(ui->actionQuit, SIGNAL(triggered()), SLOT(close()));
    QObject::connect(ui->actionFreeze, SIGNAL(toggled(bool)), SLOT(freezeToggled(bool)), Qt::DirectConnection);
    QObject::connect(ui->freezePushButton, SIGNAL(toggled(bool)), SLOT(freezeToggled(bool)), Qt::DirectConnection);
    QObject::connect(ui->nearSlider, SIGNAL(valueChanged(int)), NUI::instance(), SLOT(setNearClipping(int)));
    QObject::connect(ui->farSlider, SIGNAL(valueChanged(int)), NUI::instance(), SLOT(setFarClipping(int)));
    QObject::connect(ui->nearSlider, SIGNAL(valueChanged(int)), mDepthWidget, SLOT(setNearClipping(int)));
    QObject::connect(ui->farSlider, SIGNAL(valueChanged(int)), mDepthWidget, SLOT(setFarClipping(int)));
    QObject::connect(ui->nearSlider, SIGNAL(valueChanged(int)), mStereogramWidget, SLOT(setNearClipping(int)));
    QObject::connect(ui->farSlider, SIGNAL(valueChanged(int)), mStereogramWidget, SLOT(setFarClipping(int)));
    QObject::connect(ui->muSlider, SIGNAL(valueChanged(int)), mStereogramWidget, SLOT(setMu(int)));
    QObject::connect(ui->resolutionSpinBox, SIGNAL(valueChanged(int)), mStereogramWidget, SLOT(setResolution(int)));
    QObject::connect(ui->swapFrontBackCheckBox, SIGNAL(toggled(bool)), mStereogramWidget, SLOT(setFrontBackSwap(bool)));
    QObject::connect(ui->overlayDepthSlider, SIGNAL(valueChanged(int)), mDepthWidget, SLOT(setOverlayFrameOpacity(int)));
    QObject::connect(ui->tiltSpinBox, SIGNAL(valueChanged(int)), SLOT(setTilt(int)));
    QObject::connect(ui->eyeDistanceSpinBox, SIGNAL(valueChanged(int)), SLOT(setEyeDistance(int)));
    QObject::connect(ui->actionDetachStereogram, SIGNAL(changed()), SLOT(placeStereogramWidget()));
    QObject::connect(ui->fitFrameCheckBox, SIGNAL(toggled(bool)), SLOT(fitFrameIntoDepthFrame(bool)));
    QObject::connect(ui->modeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(modeChanged(int)));
    QObject::connect(ui->stereogramSizeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(stereogramSizeChanged(int)));

    ui->nearSlider->setMinimum(NUI_IMAGE_DEPTH_MINIMUM);
    ui->nearSlider->setMaximum(NUI_IMAGE_DEPTH_MAXIMUM);
    ui->farSlider->setMinimum(NUI_IMAGE_DEPTH_MINIMUM);
    ui->farSlider->setMaximum(NUI_IMAGE_DEPTH_MAXIMUM);

    restoreAppSettings();
}


MainWindow::~MainWindow()
{
    delete ui;
}


void MainWindow::closeEvent(QCloseEvent* e)
{
    saveAppSettings();
    e->accept();
}


void MainWindow::freezeToggled(bool running)
{
    mDepthFrameFrozen = !running;
    mNUIThread.setFrozen(mDepthFrameFrozen);
    ui->freezePushButton->blockSignals(true);
    ui->freezePushButton->setChecked(!mDepthFrameFrozen);
    ui->freezePushButton->blockSignals(false);
    ui->actionFreeze->blockSignals(true);
    ui->actionFreeze->setChecked(mDepthFrameFrozen);
    ui->actionFreeze->blockSignals(false);
}


void MainWindow::placeStereogramWidget(void)
{
    Q_ASSERT(mStereogramWidget != NULL);
    if (ui->actionDetachStereogram->isChecked()) {
        ui->horizontalLayout->removeWidget(mStereogramWidget);
        mStereogramWidget->setWindowFlags(Qt::Window);
        mStereogramWidget->showFullScreen();
        mStereogramWidget->window()->setWindowTitle(tr("Stereogramm"));
        this->resize(210, 488);
    }
    else {
        mStereogramWidget->setWindowFlags(Qt::Widget);
        ui->horizontalLayout->addWidget(mStereogramWidget);
    }
    mStereogramWidget->show();
}


void MainWindow::attachStereogramWidget(void)
{
    ui->actionDetachStereogram->setChecked(false);
}


void MainWindow::detachStereogramWidget(void)
{
    ui->actionDetachStereogram->setChecked(true);
}


void MainWindow::saveAppSettings(void)
{
    QSettings settings(MainWindow::Company, MainWindow::AppName);
    settings.setValue("Kinect/tilt", NUI::instance()->tilt());
    settings.setValue("MainWindow/geometry", saveGeometry());
    settings.setValue("MainWindow/windowState", saveState());
    settings.setValue("MainWindow/textureFileName", mTextureFileName);
    settings.setValue("MainWindow/nearClippingValue", ui->nearSlider->value());
    settings.setValue("MainWindow/farClippingValue", ui->farSlider->value());
    settings.setValue("MainWindow/swapFrontBack", ui->swapFrontBackCheckBox->checkState() == Qt::Checked);
    settings.setValue("MainWindow/overlayDepthOpacity", ui->overlayDepthSlider->value());
    settings.setValue("MainWindow/mu", ui->muSlider->value());
    settings.setValue("MainWindow/frozen", ui->freezePushButton->isChecked());
    settings.setValue("MainWindow/fitFrame", ui->fitFrameCheckBox->isChecked());
}


void MainWindow::restoreAppSettings(void)
{
    QSettings settings(MainWindow::Company, MainWindow::AppName);
    NUI::instance()->setTilt(0 /*settings.value("Kinect/tilt").toInt()*/);
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    mTextureFileName = settings.value("MainWindow/textureFileName").toString();
    if (mTextureFileName == "")
        mTextureFileName = ":/Texturen/Stereogram_Tut_Random_Dot_S.png";
    loadTexture(mTextureFileName);
    ui->nearSlider->setValue(settings.value("MainWindow/nearClippingValue").toInt());
    ui->farSlider->setValue(settings.value("MainWindow/farClippingValue").toInt());
    ui->swapFrontBackCheckBox->setChecked(settings.value("MainWindow/swapFrontBack").toBool());
    ui->overlayDepthSlider->setValue(settings.value("MainWindow/overlayDepthOpacity").toInt());
    ui->muSlider->setValue(settings.value("MainWindow/mu").toInt());
    ui->freezePushButton->setChecked(settings.value("MainWindow/frozen").toBool());
    freezeToggled(ui->freezePushButton->isChecked());
    ui->fitFrameCheckBox->setChecked(settings.value("MainWindow/fitFrame").toBool());
    placeStereogramWidget();
}


void MainWindow::loadTexture(const QString& filename)
{
    if (filename != "") {
        const bool rc = mTexture.load(filename);
        if (rc) {
            mDepthWidget->resetSelection();
            mStereogramWidget->setTexture(mTexture);
            statusBar()->showMessage(tr("Textur '%1' geladen.").arg(mTextureFileName), 3000);
        }
        else {
            QMessageBox::warning(this, tr("Fehler beim Laden der Textur"), tr("Textur konnte nicht geladen werden."));
        }
    }

}


void MainWindow::openTexture(void)
{
    mTextureFileName = QFileDialog::getOpenFileName(this, tr("Textur laden"));
    loadTexture(mTextureFileName);
}


void MainWindow::saveDepthImage(void)
{
    const QString& depthImageFilename = QFileDialog::getSaveFileName(this, tr("Tiefenbild speichern"));
    const bool rc = mDepthWidget->depthImage().save(depthImageFilename);
    if (rc) {
        statusBar()->showMessage(tr("Tiefenbild unter '%1' gespeichert.").arg(depthImageFilename), 3000);
    }
    else {
        QMessageBox::critical(this, tr("Fehler beim Speichern des Tiefenbilds"), tr("Das Tiefenbild konnte nicht unter dem Namen '%1' gespeichert werden.").arg(depthImageFilename));
    }
}


void MainWindow::saveStereogram(void)
{
    if (mDepthWidget->depthImage().isNull()) {
        QMessageBox::critical(this, tr("Stereogramm kann nicht gespeichert werden"), tr("Stereogramm kann nicht gespeichert werden, weil Tiefeninformationen fehlen"));
        return;
    }
    StereogramSaveForm saveForm(mStereogramWidget, mDepthWidget->depthImage().size(), this);
    int rc = saveForm.exec();
    Q_UNUSED(rc);
}


void MainWindow::setEyeDistance(int mm)
{
    mStereogramWidget->setEyeDistance((float)mm / 2.54f / 10);
}


void MainWindow::setTilt(int degree)
{
    NUI::instance()->setTilt(degree);
    mDepthWidget->update();
    mStereogramWidget->update();
}


void MainWindow::setSelection(const QImage& selection)
{
    mStereogramWidget->setTexture(selection.isNull()? mTexture : selection);
}


void MainWindow::fitFrameIntoDepthFrame(bool doFit)
{
    mDepthWidget->setFitFrameIntoDepthFrame(doFit);
}


void MainWindow::modeChanged(int)
{
    const int textureMode = ui->modeComboBox->itemData(ui->modeComboBox->currentIndex()).toInt();
    mStereogramWidget->setTextureMode((StereogramWidget::TextureMode)textureMode);
}


void MainWindow::stereogramSizeChanged(int)
{
    const int lines = ui->stereogramSizeComboBox->itemData(ui->stereogramSizeComboBox->currentIndex()).toInt();
    const int columns = DepthImageWidget::WIDTH * lines / DepthImageWidget::HEIGHT;
    const QSize& requestedSize = QSize(columns, lines);
    mStereogramWidget->setRequestedStereogramSize(requestedSize);
}
