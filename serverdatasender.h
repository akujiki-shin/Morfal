// Morfal is a Virtual Pen and Paper (VPP): a Game Master's toolbox designed for those who play Role Playing Games with pen and paper only,
// as opposed to Virtual Table Tops (VTT) which are made to handle tactical gameplay (tokens, square/hexagons, area of effect, etc...).
// Copyright (C) 2022  akujiki
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

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
