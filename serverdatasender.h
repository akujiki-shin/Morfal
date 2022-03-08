#pragma once

#include <QObject>

#include <QAbstractSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class QTcpSocket;
class QNetworkAccessManager;
class QNetworkReply;
class QTcpServer;
class QString;

class ServerDataSender : public QObject
{
private:
    Q_OBJECT

    using super = QObject;

public:
    explicit ServerDataSender(Ui::MainWindow* mainWindowUI, int serverPort, QObject *parent = nullptr);
    ~ServerDataSender();

private slots:
    void OnServerAddressEditingFinished();

    void SocketReadyRead();
    void SocketConnected();
    void SocketClosed();
    void SocketStateChanged(QAbstractSocket::SocketState);
    void OnNewConnection();

    void OnSendButtonClicked();

private:
    void ConnectToServer();
    void SendToServer(const QString& text);

private:
    Ui::MainWindow* ui { nullptr };
    QTcpSocket* m_Socket { nullptr };
    QTcpSocket* m_ServerSocket { nullptr };
    QNetworkAccessManager* m_NetworkAccessManager { nullptr };

    QTcpServer* m_Server { nullptr };
};
