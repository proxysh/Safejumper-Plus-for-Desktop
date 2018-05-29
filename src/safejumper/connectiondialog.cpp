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

#include "connectiondialog.h"

#include "ui_connectiondialog.h"
#include "settingsscreen.h"
#include "mapscreen.h"
#include "loginwindow.h"
#include "authmanager.h"
#include "common.h"
#include "wndmanager.h"
#include "setting.h"
#include "vpnservicemanager.h"
#include "pathhelper.h"
#include "log.h"
#include "flag.h"
#include "fonthelper.h"
#include "version.h"

#include <QHttpMultiPart>
#include <QMessageBox>
#include <QProgressDialog>

const int kConnectionPage = 0;
const int kFeedbackPage = 1;

QHash<int, QString> ConnectionDialog::mStateWordImages;

ConnectionDialog::ConnectionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ConnectionDialog),
    mNam(NULL),
    mProgressDialog(NULL)
{
    ui->setupUi(this);
    this->setFixedSize(this->size());

    ui->L_Country->setText("");
    ui->L_Percent->setText("0%");
    ui->L_OldIp->setText("");
    ui->L_NewIp->setText("");
    ui->versionLabel->setText(QString("v ") + SJ_VERSION + ", build " + QString::number(SJ_BUILD_NUM) );
    setNoServer();

    setWindowFlags(Qt::Dialog);
#ifndef Q_OS_DARWIN
    FontHelper::SetFont(this);
#ifdef Q_OS_WIN
    setWindowFlags(this->windowFlags() & ~Qt::WindowContextHelpButtonHint);
    ui->L_Until->setFont(FontHelper::pt(7));
    ui->emailLabel->setFont(FontHelper::pt(10));
#else		// linux
    ui->L_Until->setFont(FontHelper::pt(7));
    ui->L_Package->setFont(FontHelper::pt(10));
#endif
    QPoint p1 = ui->L_LOAD->pos();
    p1.setX(p1.x() + 10);
    ui->L_LOAD->move(p1);
#endif

    stateChanged(vpnStateDisconnected);

    ui->cancelButton->hide();

    ui->L_Until->setText("active until\n-");
    ui->L_Amount->setText("-");
    ui->L_OldIp->setText("");

    connect(AuthManager::instance(), &AuthManager::oldIpLoaded,
            this, &ConnectionDialog::setOldIP);
    connect(AuthManager::instance(), &AuthManager::emailLoaded,
            this, &ConnectionDialog::setEmail);
    connect(AuthManager::instance(), &AuthManager::untilLoaded,
            this, &ConnectionDialog::setUntil);
    connect(AuthManager::instance(), &AuthManager::amountLoaded,
            this, &ConnectionDialog::setAmount);
    connect(AuthManager::instance(), &AuthManager::newIpLoaded,
            this, &ConnectionDialog::setNewIP);

    // Setting::Instance()->LoadServer();
    Setting::instance()->loadProtocol();

    // TODO: -1  get actual data
    ui->L_Until->setText("active until\n-");
    ui->L_Amount->setText("-");
    setOldIP(AuthManager::instance()->oldIP());
    updateEncryption();
    updateProtocol();
    updateServer();

    connect(Setting::instance(), &Setting::encryptionChanged,
            this, &ConnectionDialog::updateEncryption);
    connect(Setting::instance(), &Setting::protocolChanged,
            this, &ConnectionDialog::updateProtocol);
    connect(Setting::instance(), &Setting::serverChanged,
            this, &ConnectionDialog::updateServer);

    connect(VPNServiceManager::instance(), &VPNServiceManager::stateWord,
            this, &ConnectionDialog::stateWordChanged);
    connect(VPNServiceManager::instance(), &VPNServiceManager::stateChanged,
            this, &ConnectionDialog::stateChanged);
}

bool ConnectionDialog::exists()
{
    return (mInstance.get() != NULL);
}

void ConnectionDialog::cleanup()
{
    if (mInstance.get() != NULL)
        delete mInstance.release();
}

void ConnectionDialog::setNoServer()
{
    ui->L_Percent->hide();
    ui->L_Percent->setText("0%");
    ui->L_LOAD->hide();
    ui->b_Flag->hide();
    ui->L_NewIp->hide();
    ui->L_Country->setText("No location specified.");
}

void ConnectionDialog::setServer(int srv)
{
    Log::logt("setServer called with server id " + QString::number(srv));
    if (srv < 0) {	// none
        setNoServer();
    } else {
        const AServer & se = AuthManager::instance()->getServer(srv);
        Log::logt("setServer server name is " + se.name);
        ui->L_Country->setText(se.name);
        ui->b_Flag->show();
        ui->b_FlagBox->show();

        QString nip = AuthManager::instance()->newIP();
        if (nip.isEmpty())
            nip = se.address;
        ui->L_NewIp->setText(nip);
        ui->L_NewIp->show();

        double d = se.load.toDouble();
        int i = se.load.toInt();
        if (i == 0 && se.load != "0")
            i = (int)d;
        ui->L_Percent->setText(QString::number(i) + "%");
        ui->L_Percent->show();
        ui->L_LOAD->show();
        setFlag(srv);
    }
}

void ConnectionDialog::setNewIP(const QString & s)
{
    static const QString self = "127.0.0.1";
    if (s != self) {
        ui->L_NewIp->setText(s);
        ui->L_NewIp->show();
    }
}

void ConnectionDialog::updateEncryption()
{
    int enc = Setting::instance()->encryption();
    ui->L_Encryption->setText(Setting::encryptionName(enc));
}

void ConnectionDialog::setOldIP(const QString & s)
{
    ui->L_OldIp->setText(s);
    ui->L_OldIp->show();
}

void ConnectionDialog::setEmail(const QString & s)
{
    ui->emailLabel->setText(s);
    ui->emailLabel->show();
}

void ConnectionDialog::setAmount(const QString & s)
{
    ui->L_Amount->setText(s);
    ui->L_Amount->show();
}

void ConnectionDialog::setUntil(const QString & date)
{
    ui->L_Until->setText("active until\n" + date);
    ui->L_Until->show();
}

void ConnectionDialog::setFlag(int srv)
{
    QString n = AuthManager::instance()->getServer(srv).name;
    QString fl = flag::IconFromSrvName(n);
    ui->b_Flag->setStyleSheet("QPushButton\n{\n	border:0px;\n	color: #ffffff;\nborder-image: url(:/flags/" + fl + ".png);\n}");
}

void ConnectionDialog::setProtocol(int ix)
{
    if (ix < 0)
        ui->L_Protocol->setText("Not selected");
    else
        ui->L_Protocol->setText(Setting::instance()->protocolName(ix));
}

void ConnectionDialog::updateProtocol()
{
    setProtocol(Setting::instance()->currentProtocol());
}

void ConnectionDialog::updateServer()
{
    setServer(Setting::instance()->serverID());
}

ConnectionDialog::~ConnectionDialog()
{
    {
        if (this->isVisible()) {
            WndManager::Instance()->HideThis(this);
            WndManager::Instance()->SavePos();
        }
    }
    delete ui;
}

void ConnectionDialog::closeEvent(QCloseEvent * event)
{
    event->ignore();
    WndManager::Instance()->HideThis(this);
}

static const char * gs_ConnGreen = "QLabel\n{\n	border:0px;\n	color: #ffffff;\n	border-image: url(:/imgs/connect-status-green.png);\n}";
static const char * gs_ConnRed = "QLabel\n{\n	border:0px;\n	color: #ffffff;\n	border-image: url(:/imgs/connect-status-red.png);\n}";
static const char * gs_Conn_Connecting = "QLabel\n{\n	border:0px;\n	color: #ffffff;\n	border-image: url(:/imgs/connect-status-yellow.png);\n}";
static const char * gs_Conn_Connecting_Template_start = "QLabel\n{\n	border:0px;\n	color: #ffffff;\n	border-image: url(:/imgs/connect-status-y-";
static const char * gs_Conn_Connecting_Template_end =  ".png);\n}";

void ConnectionDialog::initializeStateWords()
{
    if (mStateWordImages.empty()) {
        mStateWordImages.insert(ovnStateAuth, "auth");
        mStateWordImages.insert(ovnStateGetConfig, "config");
        mStateWordImages.insert(ovnStateAssignIP, "ip");
        mStateWordImages.insert(ovnStateTCPConnecting, "connect");
        mStateWordImages.insert(ovnStateResolve, "resolve");

        // CONNECTING - default - must be absent in this collection

        mStateWordImages.insert(ovnStateWait, "wait");
        mStateWordImages.insert(ovnStateReconnecting, "reconn");
    }
}

void ConnectionDialog::stateWordChanged(OpenVPNStateWord word)
{
//	ModifyWndTitle(word);
//	this->StatusConnecting();
    enableButtons(false);
    initializeStateWords();

    QString s;
    QHash<int, QString>::iterator it = mStateWordImages.find(word);
    if (it != mStateWordImages.end()) {
        s = gs_Conn_Connecting_Template_start;
        s += it.value();
        s += gs_Conn_Connecting_Template_end;
    } else {
        s = gs_Conn_Connecting;
        if (word == ovnStateWait) {
            Log::logt("Cannot find WAIT in the collection! Do actions manualy!");
            s = gs_Conn_Connecting_Template_start;
            s += "wait";
            s += gs_Conn_Connecting_Template_end;
        }
    }
    ui->L_ConnectStatus->setStyleSheet(s);
}

void ConnectionDialog::enableButtons(bool enabled)
{
    if (enabled) {
        ui->connectButton->show();
        ui->cancelButton->hide();
    } else {
        ui->connectButton->hide();
        ui->cancelButton->show();
    }

    ui->b_Flag->setEnabled(enabled);
    ui->b_FlagBox->setEnabled(enabled);
    ui->b_Row_Country->setEnabled(enabled);
    ui->b_Row_Ip->setEnabled(enabled);
    ui->b_Row_Protocol->setEnabled(enabled);
}

void ConnectionDialog::showFeedback()
{
    ui->titleLineEdit->clear();
    ui->feedbackTextEdit->clear();

    ui->stackedWidget->setCurrentIndex(kFeedbackPage);
}

void ConnectionDialog::showConnection()
{
    ui->stackedWidget->setCurrentIndex(kConnectionPage);
}

std::auto_ptr<ConnectionDialog> ConnectionDialog::mInstance;
ConnectionDialog * ConnectionDialog::instance()
{
    if (!mInstance.get()) {
        mInstance.reset(new ConnectionDialog());
    }
    return mInstance.get();
}

void ConnectionDialog::on_settingsButton_clicked()
{
    WndManager::Instance()->ToSettings();
}

void ConnectionDialog::showMapWindow()
{
    WndManager::Instance()->ToMap();
}

void ConnectionDialog::showPackageUrl()
{
    OpenUrl_Panel();
}

void ConnectionDialog::on_connectButton_clicked()
{
    VPNServiceManager::instance()->sendConnectToVPNRequest();		// handle visuals inside
}

void ConnectionDialog::on_cancelButton_clicked()
{
    VPNServiceManager::instance()->sendDisconnectFromVPNRequest();
}

void ConnectionDialog::on_jumpButton_clicked()
{
    AuthManager::instance()->jump();
}

void ConnectionDialog::keyPressEvent(QKeyEvent * e)
{
    if(e->key() != Qt::Key_Escape)
        QDialog::keyPressEvent(e);
}

void ConnectionDialog::stateChanged(vpnState state)
{

    switch (state) {
    case vpnStateConnecting: {
        ui->L_ConnectStatus->setStyleSheet(gs_Conn_Connecting);
        enableButtons(false);
    }
    break;
    case vpnStateConnected: {
        ui->L_ConnectStatus->setStyleSheet(gs_ConnGreen);
        enableButtons(true);
        ui->connectButton->hide();
        ui->cancelButton->show();
    }
    break;
    case vpnStateDisconnected: {
        ui->L_ConnectStatus->setStyleSheet(gs_ConnRed);
        enableButtons(true);
    }
    break;
    }
}

void ConnectionDialog::portDialogResult(int action)
{
    if (QDialog::Accepted == action) {
        // Switch to next port/node and tell service to connect
        VPNServiceManager::instance()->startPortLoop(WndManager::Instance()->IsCyclePort());
    }
}

void ConnectionDialog::on_cancelFeedbackButton_clicked()
{
    showConnection();
}

void ConnectionDialog::on_sendFeedbackButton_clicked()
{
    // Gather the information we need to send
    // Username, log file(s), feedback text
    QString title = ui->titleLineEdit->text();

    if (title.isEmpty()) {
        QMessageBox::warning(this, "Blank title", "Title field is required", QMessageBox::Ok);
        ui->titleLineEdit->setFocus();
        return;
    }

    QString email = AuthManager::instance()->email();
    QString loginName = AuthManager::instance()->VPNName();

    QString feedbackText = ui->feedbackTextEdit->toPlainText();
    QFile debugLog(PathHelper::Instance()->safejumperLogFilename());
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

    QHttpPart titlePart;
    titlePart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"title\""));
    titlePart.setBody(title.toLatin1());

    QHttpPart textPart;
    textPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"text\""));
    textPart.setBody(feedbackText.toLatin1());

    QHttpPart logPart;
    logPart.setHeader(QNetworkRequest::ContentDispositionHeader, QVariant("form-data; name=\"logtext\""));
    logPart.setBody(logText.toLatin1());

    multiPart->append(loginPart);
    multiPart->append(emailPart);
    multiPart->append(titlePart);
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
                          this, &ConnectionDialog::postError);
    qDebug() << "result of connecting to error signal is " << result;
    result = connect(reply, &QNetworkReply::finished,
                     this, &ConnectionDialog::sendFeedbackFinished);
    qDebug() << "result of connecting to finished signal is " << result;
    qDebug() << "Sent feedback with title " << title
             << " login " << loginName
             << " email " << email
             << " awaiting response";

    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = NULL;
    }

    mProgressDialog = new QProgressDialog(this);
    mProgressDialog->setLabelText("Sending Bug Report\nPlease wait...");
    mProgressDialog->setRange(0, 0);
    mProgressDialog->setMinimumDuration(0);
    mProgressDialog->setValue(0);
}

void ConnectionDialog::postError(QNetworkReply::NetworkError error)
{
    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = NULL;
    }

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    qDebug() << "Got error from post request " << error
             << " request url was " << reply->url().toString();
    QByteArray response = reply->readAll();
    QMessageBox::information(this, "Post error", response);
}

void ConnectionDialog::sendFeedbackFinished()
{
    if (mProgressDialog) {
        mProgressDialog->close();
        mProgressDialog->deleteLater();
        mProgressDialog = NULL;
    }

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (reply->error() != QNetworkReply::NoError) {
        QMessageBox::information(this, "send feedback error", reply->errorString());
        return;
    }

    QByteArray response = reply->readAll();

    qDebug() << "issue response: " << response;

    QMessageBox::information(this, "Issue created", QString("Issue created."));

    showConnection();
}




