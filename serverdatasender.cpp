#include "serverdatasender.h"

#include "ui_mainwindow.h"

#include <QTcpSocket>
#include <QNetworkAccessManager>
#include <QString>

#include "utils.h"

ServerDataSender::ServerDataSender(Ui::MainWindow* mainWindowUI, QObject *parent)
    : super(parent)
    , ui(mainWindowUI)
{
    m_Socket = new QTcpSocket(this);

    connect(m_Socket, &QTcpSocket::connected, this, &ServerDataSender::SocketConnected);
    connect(m_Socket, &QTcpSocket::readyRead, this, &ServerDataSender::SocketReadyRead);
    connect(m_Socket, &QTcpSocket::stateChanged, this, &ServerDataSender::SocketStateChanged);

    connect(ui->serverAddress, &QLineEdit::editingFinished, this, &ServerDataSender::OnServerAddressEditingFinished);
    connect(ui->sendButton, &QPushButton::clicked, this, &ServerDataSender::OnSendButtonClicked);

    QString ipRange = "(?:[0-1]?[0-9]?[0-9]|2[0-4][0-9]|25[0-5])";
    QString portRange = "(0|[1-9][0-9]{0,3}|[1-5][0-9]{4}|6[0-4][0-9]{3}|65[0-4][0-9]{2}|655[0-2][0-9]|6553[0-5])";

    QRegularExpression IpRegex ("^" + ipRange
                                + "(\\." + ipRange + ")"
                                + "(\\." + ipRange + ")"
                                + "(\\." + ipRange + ")"
                                + "(\\:" + portRange +")$");

    QRegularExpressionValidator* ipValidator = new QRegularExpressionValidator(IpRegex, this);
    ui->serverAddress->setValidator(ipValidator);

    ConnectToServer();

    m_NetworkAccessManager = new QNetworkAccessManager(this);
    ui->textToSend->setWordWrapMode(QTextOption::WrapAnywhere);
}

ServerDataSender::~ServerDataSender()
{
    delete m_Socket;
    delete m_NetworkAccessManager;
}

void ServerDataSender::ConnectToServer()
{
    std::string serverInfo = ui->serverAddress->text().toStdString();
    std::size_t separator = serverInfo.find(":");
    if (separator != std::string::npos)
    {
        QString host = serverInfo.substr(0, separator).c_str();
        qint16 port = std::stoi(serverInfo.substr(separator + 1, serverInfo.size()));

        m_Socket->connectToHost(host, port);

        if (m_Socket->state() == QAbstractSocket::ConnectedState)
        {
            ui->infoText->setText(tr("Connected to server\n"));
        }
        else
        {
            ui->infoText->setText(tr("connecting to ") + host + " " + std::to_string(port).c_str());
        }
    }
    else
    {
        ui->infoText->setText(tr("Invalid server info."));
    }
}

void ServerDataSender::OnServerAddressEditingFinished()
{
    ConnectToServer();
}

void ServerDataSender::SocketReadyRead()
{
    while (m_Socket->canReadLine())
    {
        ui->infoText->setText(m_Socket->readLine());
    }
}

void ServerDataSender::SocketConnected()
{
    ui->infoText->setText(tr("Connected to server\n"));
}

void ServerDataSender::SocketClosed()
{
    ui->infoText->setText(tr("Connection closed\n"));
}

void ServerDataSender::SocketStateChanged(QAbstractSocket::SocketState state)
{
    ui->infoText->setText(tr("server connection: ") + QString(Utils::QtEnumToString(state).c_str()));
}

void ServerDataSender::SendToServer(const QString& text)
{
    QUrl serviceURL("http://" + ui->serverAddress->text());

    QByteArray jsonString = text.toUtf8();
    QByteArray postDataSize = QByteArray::number(jsonString.size());

    QNetworkRequest request(serviceURL);

    request.setRawHeader("User-Agent", "InteractiveMap v0.5");
    request.setRawHeader("X-Custom-User-Agent", "InteractiveMap v0.5");
    request.setRawHeader("Content-Type", "application/json");
    request.setRawHeader("Content-Length", postDataSize);

    m_NetworkAccessManager->post(request, jsonString);
}

void ServerDataSender::OnSendButtonClicked()
{
    if (m_Socket->state() == QAbstractSocket::SocketState::ConnectedState)
    {
        SendToServer(ui->textToSend->toPlainText());
        ui->textToSend->setPlainText("");
    }
}
