// Copyright (c) 2012 Oliver Lau <oliver@von-und-fuer-lau.de>
// All rights reserved.

#include <QSettings>
#include <QFileDialog>
#include <QPrinter>
#include <QPrintDialog>
#include <QMessageBox>
#include <QtCore/QtDebug>
#include <QPainter>
#include <QEventLoop>
#include <QDesktopServices>

#include "nui.h"
#include "mainwindow.h"
#include "stereogramsaveform.h"
#include "ui_mainwindow.h"

#include "smtp/src/SmtpMime"

#ifdef _OPENMP
#include <omp.h>
#endif

const QString MainWindow::Company = "c't";
const QString MainWindow::AppName = QObject::tr("Bineqt");
#ifdef QT_NO_DEBUG
const QString MainWindow::AppVersion = "0.9.4-IFA";
#else
const QString MainWindow::AppVersion = "0.9.4-IFA [DEBUG]";
#endif


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , mDepthFrameFrozen(false)
    , mFileSequenceNumber(-1)
{
    QCoreApplication::setOrganizationName(MainWindow::Company);
    QCoreApplication::setOrganizationDomain(MainWindow::Company);
    QCoreApplication::setApplicationName(MainWindow::AppName);
    QSettings::setDefaultFormat(QSettings::NativeFormat);

    ui->setupUi(this);
    setWindowTitle(tr("%1 %2 %3").arg(MainWindow::AppName).arg(MainWindow::AppVersion).arg(_OPENMP >= 200203? "MP" : ""));

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
    QObject::connect(ui->actionPrintAutostereogram, SIGNAL(triggered()), SLOT(printStereogram()));
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
    QObject::connect(ui->overlayDepthSlider, SIGNAL(valueChanged(int)), mDepthWidget, SLOT(setOverlayFrameOpacity(int)));
    QObject::connect(ui->tiltSpinBox, SIGNAL(valueChanged(int)), SLOT(setTilt(int)));
    QObject::connect(ui->eyeDistanceSpinBox, SIGNAL(valueChanged(int)), SLOT(setEyeDistance(int)));
    QObject::connect(ui->actionDetachStereogram, SIGNAL(changed()), SLOT(placeStereogramWidget()));
    QObject::connect(ui->fitFrameCheckBox, SIGNAL(toggled(bool)), SLOT(fitFrameIntoDepthFrame(bool)));
    QObject::connect(ui->modeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(modeChanged(int)));
    QObject::connect(ui->stereogramSizeComboBox, SIGNAL(currentIndexChanged(int)), SLOT(stereogramSizeChanged(int)));
    QObject::connect(ui->actionResetFileSequenceCounter, SIGNAL(triggered()), SLOT(resetFileSequenceCounter()));

    ui->nearSlider->setMinimum(NUI_IMAGE_DEPTH_MINIMUM);
    ui->nearSlider->setMaximum(NUI_IMAGE_DEPTH_MAXIMUM);
    ui->farSlider->setMinimum(NUI_IMAGE_DEPTH_MINIMUM);
    ui->farSlider->setMaximum(NUI_IMAGE_DEPTH_MAXIMUM);

    incrementFileSequenceCounter();
    restoreAppSettings();

    QSettings settings("ifa.ini", QSettings::IniFormat);
    if (settings.status() == QSettings::NoError) {
        mSmtpServer = settings.value("SMTP/server").toString();
        mSmtpPort = (quint16)settings.value("SMTP/port").toInt();
        mSmtpUser = settings.value("SMTP/user").toString();
        mSmtpPass = settings.value("SMTP/password").toString();
        mSmtpSender = settings.value("SMTP/sender").toString();
    }
    else {
        QMessageBox::critical(this, tr("Fehler beim Laden der SMTP-Einstellungen"), tr("SMTP konnte nicht konfiguriert werden, weil die Einstellungen nicht aus der Datei 'ifa.ini' geladen werden konnten."));
    }
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
    settings.setValue("MainWindow/overlayDepthOpacity", ui->overlayDepthSlider->value());
    settings.setValue("MainWindow/mu", ui->muSlider->value());
    settings.setValue("MainWindow/resolution", ui->resolutionSpinBox->value());
    settings.setValue("MainWindow/frozen", ui->freezePushButton->isChecked());
    settings.setValue("MainWindow/fitFrame", ui->fitFrameCheckBox->isChecked());
    settings.setValue("MainWindow/patternMode", ui->modeComboBox->currentIndex());
    settings.setValue("MainWindow/stereogramSize", ui->stereogramSizeComboBox->currentIndex());
    settings.setValue("MainWindow/fileSequenceNumber", mFileSequenceNumber);
    settings.setValue("SavedStereogram/size", mSavedStereogramSize);
}


void MainWindow::restoreAppSettings(void)
{
    QSettings settings(MainWindow::Company, MainWindow::AppName);
    NUI::instance()->setTilt(settings.value("Kinect/tilt").toInt());
    restoreGeometry(settings.value("MainWindow/geometry").toByteArray());
    restoreState(settings.value("MainWindow/windowState").toByteArray());
    mTextureFileName = settings.value("MainWindow/textureFileName", ":/Texturen/tile.png").toString();
    loadTexture(mTextureFileName);
    ui->nearSlider->setValue(settings.value("MainWindow/nearClippingValue", 0).toInt());
    ui->farSlider->setValue(settings.value("MainWindow/farClippingValue", 160).toInt());
    ui->overlayDepthSlider->setValue(settings.value("MainWindow/overlayDepthOpacity", 20).toInt());
    ui->muSlider->setValue(settings.value("MainWindow/mu", 333).toInt());
    ui->resolutionSpinBox->setValue(settings.value("MainWindow/resolution", 94).toInt());
    ui->freezePushButton->setChecked(settings.value("MainWindow/frozen").toBool());
    freezeToggled(ui->freezePushButton->isChecked());
    ui->fitFrameCheckBox->setChecked(settings.value("MainWindow/fitFrame").toBool());
    ui->modeComboBox->setCurrentIndex(settings.value("MainWindow/patternMode").toInt());
    ui->stereogramSizeComboBox->setCurrentIndex(settings.value("MainWindow/stereogramSize").toInt());
    mSavedStereogramSize = settings.value("SavedStereogram/size", QSize(640, 480)).toSize();
    mFileSequenceNumber = settings.value("MainWindow/fileSequenceNumber", -1).toInt();
    incrementFileSequenceCounter();
    placeStereogramWidget();
}


void MainWindow::loadTexture(const QString& filename)
{
    if (filename != "") {
        bool success = mTexture.load(filename);
        if (success) {
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
    bool success = mDepthWidget->depthImage().save(depthImageFilename);
    if (success) {
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
    QSize requestedStereogramSize = mSavedStereogramSize.isEmpty()? mDepthWidget->depthImage().size() : mSavedStereogramSize;
    StereogramSaveForm saveForm(mStereogramWidget, requestedStereogramSize, this);
    int rc = saveForm.exec();
    if (rc == QDialog::Accepted)
        mSavedStereogramSize = saveForm.chosenImageSize();
}


void MainWindow::printStereogram(void)
{
    bool success;
    QPrinter printer(QPrinter::HighResolution);
    QPrintDialog printDialog(&printer, this);
    if (printDialog.exec() == QDialog::Accepted) {
        QPainter painter(&printer);
        QRect rect = painter.viewport();
        const QImage& stereogramPrint = mStereogramWidget->stereogram(QSize(1754, 1240) /* DIN A4 @ 150 dpi */);
        QSize size = stereogramPrint.size();
        size.scale(rect.size(), Qt::IgnoreAspectRatio);
        painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
        painter.setWindow(stereogramPrint.rect());
        painter.drawImage(0, 0, stereogramPrint);
        const QImage& stereogramFile = mStereogramWidget->stereogram(QSize(1440, 1080));
        const QString stereogramFilename = QString("%1.png").arg(mFileSequenceNumber);
        incrementFileSequenceCounter();
        success = stereogramFile.save(stereogramFilename);
        if (success)
            statusBar()->showMessage(tr("Bild '%1' wurde gespeichert und wird nun gedruckt.").arg(stereogramFilename));
        else
            statusBar()->showMessage(tr("Uiuiuiiii, beim Speichern des Bildes '%1' ist irgendwas schiefgegangen =:-/").arg(stereogramFilename));

        MailAddressDialog mailDialog(this);
        mailDialog.setFileName(stereogramFilename);
        int rc = mailDialog.exec();
        if (rc == QDialog::Accepted) {
            QString recipient = mailDialog.getAddress();
            MimeMessage message;
            message.setSender(new EmailAddress(mSmtpSender));
            message.addRecipient(new EmailAddress(recipient));
            message.setSubject("Ihr Autostereogramm vom c't-Stand auf der IFA 2012");
            MimeText body;
            body.setText("Guten Tag!\n\n"
                         "Vielen Dank für Ihren Besuch auf unseren IFA-Stand. Mit dieser Mail erhalten Sie das Autostereogramm, das Sie angefertigt haben.\n\n"
                         "Freundliche Grüße,\n"
                         "Ihre c't-Redaktion\n\n"
                         "Bitte beachten Sie, dass Sie auf diese Mail nicht antworten können.\n\n"
                         "-- \n"
                         "c't - Magazin fuer Computertechnik, http://ct.de\n"
                         "Karl-Wiechert-Allee 10, 30625 Hannover, Germany\n"
                         "fon +49-511-5352-300, fax +49-511-5352-417\n\n"
                         "/* Pflichtangaben gemäß §37a HGB: Heise Zeitschriften Verlag GmbH & Co. KG, Registergericht: Amtsgericht Hannover, HRA 26709; Persönlich haftende Gesellschafterin: Heise Zeitschriften Verlag Geschäftsführung GmbH, Registergericht: Amtsgericht Hannover, HRB 60405, Geschäftsführer: Ansgar Heise, Dr. Alfons Schräder */");
            message.addPart(&body);
            MimeAttachment attachment(new QFile(stereogramFilename));
            attachment.setContentType("image/png");
            message.addPart(&attachment);
            SmtpClient smtp(mSmtpServer, mSmtpPort, SmtpClient::TlsConnection);
            smtp.setUser(mSmtpUser);
            smtp.setPassword(mSmtpPass);
            success = smtp.connectToHost();
            success &= smtp.login();
            success &= smtp.sendMail(message);
            smtp.quit();
            if (success)
                statusBar()->showMessage(tr("Mail wurde an '%1' versendet.").arg(recipient));
            else
                statusBar()->showMessage(tr("Uiuiuiiii, beim Versenden der Mail an %1 ist irgendwas schiefgegangen =:-/").arg(recipient));
        }
    }
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


void MainWindow::incrementFileSequenceCounter(void)
{
    ++mFileSequenceNumber;
    ui->actionResetFileSequenceCounter->setText(tr("Zähler für automatischen Speichern zurücksetzen (aktuell: %1)").arg(mFileSequenceNumber));
}


void MainWindow::resetFileSequenceCounter(void)
{
    const int rc = QMessageBox::question(this, tr("Dateizähler zurücksetzen?"), tr("Zähler für automatisches Speichern des Stereogramms beim Drucken wirklich zurücksetzen?"), QMessageBox::Yes | QMessageBox::No);
    if (rc == QMessageBox::Yes) {
        mFileSequenceNumber = -1;
        incrementFileSequenceCounter();
        statusBar()->showMessage(tr("Der Zähler für automatisches Speichern wurde auf 0 zurückgesetzt."), 5000);
    }
}
