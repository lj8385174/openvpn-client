#include "openvpn.h"

OpenVpn::OpenVpn () {
    this->connectionStable = false;
    this->onDisconnect = false;
    userauth = new UserAuth();
    this->proc = new QProcess(this);
    connect(this, SIGNAL(configSignalIsChanged()), this, SLOT(configIsChanged()));
}

void OpenVpn::openConnect() {
    // Objekte �bergeben
    userauth->pIcon = this->appIcon;
    userauth->parentVpn = this;
    // Felder leeren
    userauth->clearFields();
    // Dialog zeigen
    userauth->show();
}

void OpenVpn::configIsChanged() {
    QMessageBox::information(0, QString("OpenVPN Client"),
                                QString("Config have been changed! Restart Connection?"));
}

void OpenVpn::setTray (QSystemTrayIcon *appIcon){
    this->appIcon = appIcon;
}

void OpenVpn::connectToVpn(){
    this->onDisconnect = false;
    // Connect
    if (!this->isConnectionStable()) {
        // Icon auf connecting setzen
        this->setConnected();
        this->setIcon(Connecting);
        this->openVpnLogData.clear();
        // connect to VPN
        QString cFile;
        QStringList arguments;
        QString program = "./app/bin/openvpn";

        // Pfad f�r die Config bauen
        cFile = this->configPath + QString("/") + this->configName + QString(".ovpn");
        // Als Argument adden
        arguments << cFile;
        // Prozesssignale zur �berwachung an die Slots binden
        connect( this->proc, SIGNAL(error ( QProcess::ProcessError) ), this, SLOT(showProcessError (QProcess::ProcessError)));
        connect( this->proc, SIGNAL(finished (int, QProcess::ExitStatus )), this, SLOT(processFinished (int, QProcess::ExitStatus)));
        connect( this->proc, SIGNAL(started ()), this, SLOT(processIsStarted ()));
        connect( this->proc, SIGNAL(readyReadStandardOutput()), this, SLOT(readProcessData()));
        connect( this->proc, SIGNAL(readyReadStandardError()), this, SLOT(readProcessData()));
        // Programm starten im Config Verzeichnis, sonst findet OpenVPN keine Zertifikate
        this->proc->setWorkingDirectory(this->configPath + QString("/"));
        this->proc->start(program, arguments);
    } else {
        // Connection already stable
        QMessageBox::information(0, QString("OpenVPN Client"),
                                QString("Already connected to this network"));
    }
}

void OpenVpn::showTrayMessage(QString message) {
     this->appIcon->showMessage("OpenVPN Client", message, QSystemTrayIcon::Information, 6000);
}

void OpenVpn::setMenuChildList(QList<QAction*> List) {
    this->menuChildList = List;
}

void OpenVpn::setMenuList(QList<QAction*> List) {
    this->menuList = List;
}

void OpenVpn::setConnected() {
    // Erst alle aktivieren
    this->setDisconnected();
    // Main Menu Punkte deaktivieren
    foreach (QAction *act, this->menuList) {
        if (act->text() != this->configName)
            act->setDisabled(true);
    }
    // VPN eigene Punkte deaktivieren
    foreach (QAction *act, this->menuChildList) {
        if (act->objectName() == this->configName) {
            if(act->text() == "Connect")
                act->setDisabled(true);
            if(act->text() == "Edit config")
                act->setDisabled(true);
            if(act->text() == "Change password")
                act->setDisabled(true);
            if(act->text() == "Disconnect")
                act->setEnabled(true);
        }
    }
}

void OpenVpn::setDisconnected() {
    // Main Menu Punkte deaktivieren
    foreach (QAction *act, this->menuList) {
            act->setEnabled(true);
    }
    // VPN eigene Punkte deaktivieren
    foreach (QAction *act, this->menuChildList) {
        if(act->text() == "Connect")
            act->setEnabled(true);
        if(act->text() == "Edit config")
            act->setEnabled(true);
        if(act->text() == "Change password")
            act->setEnabled(true);
        if(act->text() == "Disconnect")
            act->setDisabled(true);
    }
}

void OpenVpn::setIcon(int index)
{
    QIcon icon;
    switch (index) {
        case Inaktiv:
            icon = QIcon(":/images/inaktiv.svg");
            break;
        case Connected:
            icon = QIcon(":/images/connected.svg");
            break;
        case Error:
            icon = QIcon(":/images/disconnected.svg");
            break;
        case Connecting:
            icon = QIcon(":/images/connecting.svg");
            break;
        default:
            icon = QIcon(":/images/inaktiv.svg");
            break;
    }
    this->appIcon->setIcon(icon);
}

bool OpenVpn::isConnectionStable () {
    return this->connectionStable;
}

void OpenVpn::showProcessError(QProcess::ProcessError error) {
    QString errMessage;
    switch (error) {
        case QProcess::FailedToStart:
            errMessage = QString ("The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.");
            break;
        case QProcess::Crashed:
            errMessage = QString ("The process crashed some time after starting successfully.");
            break;
        case QProcess::Timedout:
            errMessage = QString ("The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.");
            break;
        case QProcess::WriteError:
            errMessage = QString ("An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.");
            break;
        case QProcess::ReadError:
            errMessage = QString ("An error occurred when attempting to read from the process. For example, the process may not be running.");
            break;
        case QProcess::UnknownError:
            errMessage = QString ("An unknown error occurred. This is the default return value of error().");
            break;
        default:
            errMessage = QString ("No valid error code!");
            break;
    }
    if (!this->onDisconnect) {
        // Log schreiben
        this->openVpnLogData.append(errMessage);
        // Daten ausgeben
        QMessageBox::critical(0, QString("OpenVPN Client"),
                                    errMessage);
    }
}

void OpenVpn::processIsStarted() {
   // still not used
}

void OpenVpn::processFinished (int stat, QProcess::ExitStatus status) {
    // still not used
}

void OpenVpn::disconnectVpn() {
    this->onDisconnect = true;
    this->proc->kill();
    this->connectionStable = false;
    this->setIcon(Inaktiv);
    this->setDisconnected();
    this->showTrayMessage(QString("Disconnected from VPN.\nName: ") + this->configName);
}

void OpenVpn::readProcessData() {
    QByteArray line;
    QString output;
    bool showLine = true;
    line = this->proc->readAllStandardError();
    if (line == "")
        line = this->proc->readAllStandardOutput();
    if (line != "") {
        //Enter Private Key Password:
        if (line == "Enter Private Key Password:") {
            if (this->proc->isWritable()) {
                this->showTrayMessage(QString("PEM PASSWORD NEEDED.\nName: ") + this->configName);
            }

        }
        // Enter Username?
        if (line == "Enter Auth Username:") {
            if (this->proc->isWritable()) {
                QByteArray ba;
                ba.append(this->configUser + "\n");
                this->proc->write(ba);
                showLine = false;
            }

        }
        // Enter Password
        if (line == "Enter Auth Password:") {
            if (this->proc->isWritable()) {
                QByteArray ba;
                ba.append(this->configPwd + "\n");
                this->proc->write(ba);
                showLine = false;
            }

        }
        // QByteArray in QString
        QString lineOut(line);
        // Verbindung da?
        if (lineOut.contains("Notified TAP-Win32 driver to set a DHCP IP", Qt::CaseInsensitive)) {
            // IP Suchen und speichern
            QString connIP = lineOut.mid(lineOut.indexOf("Notified TAP-Win32 driver to set a DHCP IP")+54,15);
            // IP durchlaufen und / und spaces entfernen
            int indexOfChar = 0;
            indexOfChar = connIP.indexOf("/",0);
            if (indexOfChar != 0) {
                // Maskeabschneiden
                connIP = connIP.left(indexOfChar);
            } else {
                // Lerrzeichen da?
                indexOfChar = connIP.indexOf(" ",0);
                if (indexOfChar != 0) {
                    // Leerzeichen abschneiden
                    connIP = connIP.left(indexOfChar);
                }
            }

            // Meldung zeigen connected
            this->connectionIP = connIP;
            this->showTrayMessage(QString("Connnected to VPN.\nName: ") + this->configName + QString("\nIP: ") + this->connectionIP);
            // Status speichern und Tray Icon setzen
            this->connectionStable = true;
            this->setIcon(Connected);
        }
        // Fehler abfangen
        bool errorOcurred = false;
        QString errorMessage;
        //"All TAP-Win32 adapters on this system are currently in use"
        if (lineOut.contains("All TAP-Win32 adapters on this system are currently in use", Qt::CaseInsensitive)) {
            errorMessage = QString ("All TAP-Win32 adapters on this system are currently in use");
            errorOcurred = true;
        } else if (lineOut.contains("TLS Error: Need PEM pass phrase for private key", Qt::CaseInsensitive)) {
            errorMessage = QString ("TLS Error: Need PEM pass phrase for private key");
            errorOcurred = true;
        } else if (lineOut.contains("EVP_DecryptFinal:bad decrypt", Qt::CaseInsensitive)) {
            errorMessage = QString ("EVP_DecryptFinal:bad decrypt");
            errorOcurred = true;
        } else if (lineOut.contains("PKCS12_parse:mac verify failure", Qt::CaseInsensitive)) {
            errorMessage = QString ("PKCS12_parse:mac verify failure");
            errorOcurred = true;
        } else if (lineOut.contains("Received AUTH_FAILED control message", Qt::CaseInsensitive)) {
            errorMessage = QString ("Received AUTH_FAILED control message");
            errorOcurred = true;
        } else if (lineOut.contains("Auth username is empty", Qt::CaseInsensitive)) {
            errorMessage = QString ("Auth username is empty");
            errorOcurred = true;
        } else if (lineOut.contains("error=certificate has expired", Qt::CaseInsensitive)) {
            errorMessage = QString ("error=certificate has expired");
            errorOcurred = true;
        } else if (lineOut.contains("error=certificate is not yet valid", Qt::CaseInsensitive)) {
            errorMessage = QString ("error=certificate is not yet valid");
            errorOcurred = true;
        } else if (lineOut.contains("Cannot load certificate file", Qt::CaseInsensitive)) {
            errorMessage = QString ("Cannot load certificate file");
            errorOcurred = true;
        }



        if (errorOcurred) {
            this->showTrayMessage(errorMessage);
            this->connectionStable = false;
            this->setIcon(Error);
            this->setDisconnected();
        }

        // Output ins Log?
        if (showLine) {
            output = QString::fromLatin1(line);
            this->openVpnLogData.append(output);
        }
    }

}

void OpenVpn::editVpnConfig() {
    // Variablen
    QString fileContent = QString("");
    QString cFile;

    cFile = this->configPath + QString("/") + this->configName + QString(".ovpn");
    QFile cf (cFile);

    if (!cf.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox msgBox;
                    msgBox.setText("Cannot read file.");
                    msgBox.exec();
         return;
    }

    QTextStream in(&cf);
    while (!in.atEnd()) {
        QString line = in.readLine();
        fileContent += line + QString("\n");
    }
    cf.close();
    if (fileContent != "") {
        mCon.fileContent->setText(fileContent);
        mCon.fPath->setText(cFile);
        mCon.show();
    }
    emit configSignalIsChanged();
}

void OpenVpn::openVpnLog() {
    QString logdata;
    foreach (QString str, this->openVpnLogData) {
        logdata += str + "\n";
    }
    mLog.logContent->setText(logdata);
    mLog.show();
}

