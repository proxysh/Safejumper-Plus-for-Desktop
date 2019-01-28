/***************************************************************************
 *   Copyright (C) 2017 by Jeremy Whiting <jeremypwhiting@gmail.com>       *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation version 2 of the License.                *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.          *
 ***************************************************************************/

#define _WINSOCKAPI_
#include "mainwindow.h"

#include "application.h"
#include "authmanager.h"
#include "common.h"
#include "log.h"
#include "pathhelper.h"
#include "setting.h"
#include "trayiconmanager.h"
#include "vpnservicemanager.h"

#include <QClipboard>
#include <QDesktopServices>
#include <QDesktopWidget>
#include <QHttpMultiPart>
#include <QKeyEvent>
#include <QMessageBox>
#include <QProgressDialog>
#include <QQmlContext>
#include <QQmlEngine>
#include <QStyle>
#include <QTextStream>

#ifdef Q_OS_DARWIN
#include "smjobbless.h"
#endif

MainWindow::MainWindow() :
    QQuickView(),
    mNam(nullptr),
    mProgressDialog(nullptr)
{
    rootContext()->setContextProperty("authmanager", AuthManager::instance());
    rootContext()->setContextProperty("serversModel", AuthManager::instance()->serversModel());
    rootContext()->setContextProperty("hubsModel", AuthManager::instance()->hubsModel());
    rootContext()->setContextProperty("settings", Setting::instance());
    rootContext()->setContextProperty("mainwindow", this);
    rootContext()->setContextProperty("vpnservicemanager", VPNServiceManager::instance());

#ifdef Q_OS_DARWIN
    bool helperInstalled = installPrivilegedHelperTool();

    while (!helperInstalled) {
        helperInstalled = installPrivilegedHelperTool();
        usleep(100);
    }
#endif

    setMaximumHeight(604);
    setMinimumHeight(604);
    setMaximumWidth(300);
    setMinimumWidth(300);

    setResizeMode(QQuickView::SizeRootObjectToView);

    setSource(QUrl("qrc:/qml/MainWindow.qml"));

    connect(Setting::instance(), &Setting::languageChanged,
            this, &MainWindow::languageChanged);

    // Call it here, since the language was loaded by the above instantiation
    // of the setting object
    languageChanged();

    setFlags(Qt::Dialog);
    setIcon(QIcon(":/images/logo.png"));
    setTitle(kAppName);

    QSize windowSize(300, 604);

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            windowSize,
            qApp->desktop()->availableGeometry()
        )
    );

    // Setting::Instance()->LoadServer();
    Setting::instance()->loadProtocol();

    connect(TrayIconManager::instance(), &TrayIconManager::quitApplication,
            this, &MainWindow::exitTriggered);
    connect(TrayIconManager::instance(), &TrayIconManager::logout,
            this, &MainWindow::logoutTriggered);
    connect(TrayIconManager::instance(), &TrayIconManager::statusTriggered,
            this, &MainWindow::showMap);
    connect(TrayIconManager::instance(), &TrayIconManager::settingsTriggered,
            this, &MainWindow::showSettings);
    connect(TrayIconManager::instance(), &TrayIconManager::logsTriggered,
            this, &MainWindow::showLogs);

#ifndef Q_OS_DARWIN
    connect(g_pTheApp, &QtSingleApplication::messageReceived,
            this, &MainWindow::messageReceived);
#endif

    if (Setting::instance()->autoconnect() && !Setting::instance()->login().isEmpty()
            && !Setting::instance()->password().isEmpty())
        AuthManager::instance()->login(Setting::instance()->login(), Setting::instance()->password());
    else {
        AuthManager::instance()->getServerList();
    }
}

void MainWindow::shutDown()
{
    // Shut down application
    g_pTheApp->quit();
}

void MainWindow::launchUrl(const QString &url)
{
    hide();
    QDesktopServices::openUrl(QUrl(url, QUrl::TolerantMode));
}

//bool MainWindow::event(QEvent *ev)
//{
//    if (ev->type() == QEvent::Close) {
//        qDebug() << "Close event, calling exitTriggered";
//        ev->accept();
//        exitTriggered();
//        return true;
//    } else {
//        return QWindow::event(ev);
//    }
//}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QSize size = event->size();
    Log::logt(QString("Resize event changed window size to width: %1, height: %2")
              .arg(size.width()).arg(size.height()));
    QWindow::resizeEvent(event);
}

bool MainWindow::exists()
{
    return (!mInstance.isNull());
}

void MainWindow::cleanup()
{
    if (VPNServiceManager::exists())
        VPNServiceManager::cleanup();

    if (AuthManager::exists())
        AuthManager::cleanup();

    if (!mInstance.isNull()) {
        mInstance->closeWindow();
        mInstance->deleteLater();
    }
}

MainWindow::~MainWindow()
{
    {
        if (this->isVisible()) {
            closeWindow();
        }
    }
}

void MainWindow::showFeedback()
{
//    ui->titleLineEdit->clear();
//    ui->feedbackTextEdit->clear();

//    ui->stackedWidget->setCurrentIndex(kFeedbackPage);
}

void MainWindow::showConnection()
{
    //    ui->stackedWidget->setCurrentIndex(kConnectionPage);
}

const QString MainWindow::logsContent() const
{
    qDebug() << "logsContent called";
    QString retval;

    QFile serviceFile(PathHelper::Instance()->serviceLogFilename());
    if (serviceFile.open(QFile::ReadOnly | QFile::Text)) {
        QTextStream serviceTextStream(&serviceFile);
        retval = "Service log:\n";
        retval += serviceTextStream.readAll();
    } else {
        retval += "Unable to open service log at: " + PathHelper::Instance()->serviceLogFilename() + "\n";
    }

    QFile f(PathHelper::Instance()->applicationLogFilename());
    if (!f.open(QFile::ReadOnly | QFile::Text))
        return retval;

    QTextStream in(&f);
    retval += "\nGui log:\n";
    retval += in.readAll();

    f.close();
    return retval;
}

void MainWindow::copyLogsToClipboard() const
{
    QString logText = logsContent();
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(logText);
}

void MainWindow::launchCustomerService() const
{
    OpenUrl_Support();
}

QPointer<MainWindow> MainWindow::mInstance;
MainWindow * MainWindow::instance()
{
    if (mInstance.isNull()) {
        mInstance = new MainWindow();
    }
    return mInstance;
}

void MainWindow::portDialogResult(bool cyclePorts)
{
    // Switch to next port/node and tell service to connect
    VPNServiceManager::instance()->startPortLoop(cyclePorts);
}

void MainWindow::showAndFocus()
{
    show();
    raise();
//    activateWindow();
}

void MainWindow::showLogs()
{
    // Manipulate qml object to show logs in popup
    showAndFocus();
    emit logsScreen();
}

void MainWindow::showMap()
{
    showAndFocus();
    emit mapScreen();
}

void MainWindow::showSettings()
{
    // Manipulate qml object to show settings page
    showAndFocus();
    emit settingsScreen();
}

void MainWindow::languageChanged()
{
    rootContext()->engine()->retranslate();
}

void MainWindow::on_cancelFeedbackButton_clicked()
{
    showConnection();
}

void MainWindow::on_sendFeedbackButton_clicked()
{
    // Gather the information we need to send
    // Username, log file(s), feedback text
//    QString title = ui->titleLineEdit->text();

//    if (title.isEmpty()) {
//        QMessageBox::warning(this, "Blank title", "Title field is required", QMessageBox::Ok);
//        ui->titleLineEdit->setFocus();
//        return;
//    }

    QString email = AuthManager::instance()->email();
    QString loginName = AuthManager::instance()->accountName();

    QString feedbackText; // = ui->feedbackTextEdit->toPlainText();
    QFile debugLog(PathHelper::Instance()->applicationLogFilename());
    debugLog.open(QIODevice::ReadOnly|QIODevice::Text);
    // Only send last 4k of log
    QString logText = QString(debugLog.readAll()).right(4096);

    // Construct post
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);

    QHttpPart loginPart;
    loginPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"login\""));
    loginPart.setBody(loginName.toLatin1());

    QHttpPart emailPart;
    emailPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"email\""));
    emailPart.setBody(email.toLatin1());

//    QHttpPart titlePart;
//    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"title\""));
//    titlePart.setBody(title.toLatin1());

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    textPart.setBody(feedbackText.toLatin1());

    QHttpPart logPart;
    logPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"logtext\""));
    logPart.setBody(logText.toLatin1());

    multiPart->append(loginPart);
    multiPart->append(emailPart);
//    multiPart->append(titlePart);
    multiPart->append(textPart);
    multiPart->append(logPart);

    QNetworkRequest request(QUrl("https://proxy.sh/api-feedback.php"));
    request.setRawHeader("cache-control:", "no-cache");

    if (mNam)
        mNam->deleteLater();
    mNam = new QNetworkAccessManager(this);
//    request.setRawHeader(QByteArray("UDID"), Log::udid().toLatin1());
    QNetworkReply *reply = mNam->post(request, multiPart);
    multiPart->setParent(reply);

    bool result = connect(reply, static_cast<void(QNetworkReply::*)(QNetworkReply::NetworkError)>(&QNetworkReply::error),
                          this, &MainWindow::postError);
    qDebug() << "result of connecting to error signal is " << result;
    result = connect(reply, &QNetworkReply::finished,
                     this, &MainWindow::sendFeedbackFinished);
    qDebug() << "result of connecting to finished signal is " << result;
    qDebug() << "Sent feedback with title "
             << " login " << loginName
             << " email " << email
             << " awaiting response";

    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = nullptr;
    }

    mProgressDialog = new QProgressDialog();
    mProgressDialog->setLabelText("Sending Bug Report\nPlease wait...");
    mProgressDialog->setRange(0, 0);
    mProgressDialog->setMinimumDuration(0);
    mProgressDialog->setValue(0);
}

void MainWindow::postError(QNetworkReply::NetworkError error)
{
    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = nullptr;
    }

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    qDebug() << "Got error from post request " << error
             << " request url was " << reply->url().toString();
    QByteArray response = reply->readAll();
//    QMessageBox::information(this, "Post error", response);
}

void MainWindow::sendFeedbackFinished()
{
    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = nullptr;
    }

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() != QNetworkReply::NoError) {
//        QMessageBox::information(this, "send feedback error", reply->errorString());
        return;
    }

    QByteArray response = reply->readAll();

    qDebug() << "issue response: " << response;

//    QMessageBox::information(this, "Issue created", QString("Issue created."));

    showConnection();
}

void MainWindow::logoutTriggered()
{
    showAndFocus();
    emit logout();
}

void MainWindow::closeWindow()
{
    QSettings settings;
    settings.setValue("pos", position());
    hide();
}

void MainWindow::exitTriggered()
{
    showAndFocus();
    emit confirmExit();
}

void MainWindow::messageReceived(const QString &message)
{
    if (message == "show") {
        showAndFocus();
    }
}

