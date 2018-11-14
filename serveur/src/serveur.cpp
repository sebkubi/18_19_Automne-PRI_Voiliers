#include "serveur.h"

/**
 * CONSTRUCTOR
 *
 * @brief ServeurTcp::ServeurTcp : TODO
 * @param port
 */
ServeurTcp::ServeurTcp(quint16 port) {
    // Open the connection
	if (!(listen(QHostAddress::Any, port))) {
        cout << "Server: OFF" << endl;
    } else {
        cout << "Server: ON" << endl;
        connect(this, SIGNAL(newConnection()), this, SLOT(demandeConnexion()));
    }

    // Create the UART
    uart = new SerialData(QString("COM1"), nullptr);
    // Connect it -> when receivedDataFromUART signal is emitted, call readDataFromUART slot
    connect(uart, SIGNAL(receivedDataFromUART(Message)), this, SLOT(readDataFromUART(Message)));

    // Set length of message to 0
    tailleMessage = 0;
}

/**
 * DESTRUCTOR
 *
 * @brief ServeurTcp::~ServeurTcp : TODO
 */
ServeurTcp::~ServeurTcp() {
    qDeleteAll(clients);
    delete uart;
    cout << "Server: OFF" << endl;
}

/*--------------------------*
 *                          *
 *         METHODS          *
 *                          *
 *--------------------------*/
/**
 * METHOD
 *
 * @brief ServeurTcp::sendToAll : send to TCP/IP clients and UART clients
 * @param message
 */
void ServeurTcp::sendToAllComputers(Message message) {
    // Préparation du paquet
    QByteArray paquet;
    QDataStream out(&paquet, QIODevice::WriteOnly);

    // Envoi du paquet préparé à tous les clients connectés au serveur
    for (int i = 0; i < clients.size(); i++) {
        out << quint16(0);    // On écrit 0 au début du paquet pour réserver la place pour écrire la taille
        out << message.encodeData();        // On ajoute le message à la suite
        out.device()->seek(0); // On se replace au début du paquet
        out << quint16((paquet.size() - int(sizeof(quint16)))); // On écrase le 0 qu'on avait réservé par la longueur du message
        clients[i]->write(paquet);
    }
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendToAllExceptWeatherStation : TODO
 * @param message
 */
void ServeurTcp::sendToAllExceptWeatherStation(Message message) {
    // Envoi du paquet préparé à tous les clients connectés au serveur
    for (int i = 0; i < clients.size(); i++) {
        // Préparation du paquet
        QByteArray paquet;
        QDataStream out(&paquet, QIODevice::WriteOnly);


        // Create the data
        Message msg;
        msg.setType(new string("S"));
        msg.setIdSender(new int(0));
        msg.setCap(message.getCap());
        msg.setVitesse(message.getVitesse());
        msg.setLatitude(message.getLatitude());
        msg.setLongitude(message.getLongitude());
        msg.setIdConcern(message.getIdSender());

        Computer c;
        if (getComputerWithIndexOfSocket(c, i)) {
            msg.setIdDest(new int(c.getId()));

            out << quint16(0);    // On écrit 0 au début du paquet pour réserver la place pour écrire la taille
            out << msg.encodeData();        // On ajoute le message à la suite
            out.device()->seek(0); // On se replace au début du paquet
            out << quint16((paquet.size() - int(sizeof(quint16)))); // On écrase le 0 qu'on avait réservé par la longueur du message
            // Send the data to the specific computer
            clients[i]->write(paquet);
        }

        // Set the id of  the recipient
        msg.setIdDest(new int(i));
        // Send the data to the specific boat
        uart->sendData(msg);
    }
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendToAllExcept : TODO
 * @param message
 * @param client
 */
void ServeurTcp::sendToAllComputersExcept(Message message, int indexOfSocketToIgnore) {
    // Préparation du paquet
    QByteArray paquet;
    QDataStream out(&paquet, QIODevice::WriteOnly);

    Message msg;
    msg.setType(new string("S"));
    msg.setIdSender(new int(0));
    msg.setIdConcern(message.getIdSender());
    // Envoi du paquet préparé à tous les clients connectés au serveur
    for (int i = 0; i < clients.size(); i++) {
        if (i != indexOfSocketToIgnore) {
            Computer c;
            if (getComputerWithIndexOfSocket(c, i)) {
                msg.setIdDest(new int(c.getId()));
                msg.setLatitude(message.getLatitude());
                msg.setLongitude(message.getLongitude());

                out << quint16(0);    // On écrit 0 au début du paquet pour réserver la place pour écrire la taille
                out << msg.encodeData();        // On ajoute le message à la suite
                out.device()->seek(0); // On se replace au début du paquet
                out << quint16((paquet.size() - int(sizeof(quint16)))); // On écrase le 0 qu'on avait réservé par la longueur du message

                clients[i]->write(paquet);
            }
        }
    }
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendTo : send a message to one and only one client
 * @param message
 * @param client
 */
void ServeurTcp::sendToComputer(Message message, int id) {
    // Préparation du paquet
    QByteArray paquet;
    QDataStream out(&paquet, QIODevice::WriteOnly);

    message.setType(new string("S"));
    message.setIdSender(new int(0));
    message.setIdConcern(new int(id));
    message.setIdDest(new int(id));

    out << quint16(0);    // On écrit 0 au début du paquet pour réserver la place pour écrire la taille
    out << message.encodeData();        // On ajoute le message à la suite
    out.device()->seek(0); // On se replace au début du paquet
    out << quint16((paquet.size() - int(sizeof(quint16)))); // On écrase le 0 qu'on avait réservé par la longueur du message

    // Envoi du paquet préparé au client demandé
    Computer c;
    // Récupération du PC associé à l'id en vérifiant si celui-ci est
    // présent dans le vecteur computers
    if (getComputerWithId(c, id))
        clients[c.getIndexOfSocket()]->write(paquet);
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendToAllBoatsExcept : TODO
 * @param message
 * @param id
 */
void ServeurTcp::sendToAllBoatsExcept(Message message, int id) {
    for (int i=0; i < boatsId.size(); i++) {
        if (i != id) {
            Message msg;
            msg.setType(new string("S"));
            msg.setIdConcern(message.getIdSender());
            msg.setIdDest(message.getIdConcern());
            msg.setIdSender(new int(0));
            msg.setLatitude(message.getLatitude());
            msg.setLongitude(message.getLongitude());
            sendDataToUART(msg);
        }
    }
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendDataToUART : send data to the UART
 * @param msg
 */
void ServeurTcp::sendDataToUART(Message msg) {
    uart->sendData(msg);
}

/**
 * METHOD
 *
 * @brief ServeurTcp::checkConnectionUART : TODO
 * @param message
 * @return
 */
boolean ServeurTcp::checkConnectionUART(Message msg) {
    if (msg.getType()->length() != 2)
        return false;

    if (*msg.getType() == "MC") {
        // Connection d'une station météo
        addNewWeatherStation(msg);
    } else if (*msg.getType() == "BC") {
        // Connection d'un bateau
        addNewBoat(msg);
    }

    return true;
}

/**
 * METHOD
 *
 * @brief ServeurTcp::checkConnectionTCPIP : TODO
 * @param data
 * @param socket
 */
boolean ServeurTcp::checkConnectionTCPIP(Message data, QTcpSocket* socket) {
    if (data.getType()->length() != 2)
        return false;

    int index = clients.indexOf(socket);
    addNewComputer(data, index);

    cout << "Nouveau client:" << *data.getIdSender() << endl;

    return true;
}

/**
 * METHOD
 *
 * @brief ServeurTcp::treatTCPIPMessage : TODO
 * @param msg
 */
void ServeurTcp::treatBoatDatas(Message msg) {
    (void)msg;
}

/**
 * METHOD
 *
 * @brief ServeurTcp::sendToBoat: TODO
 * @param msg
 * @param id
 */
void ServeurTcp::sendToBoat(Message msg, int id) {
    msg.setType(new string("S"));
    msg.setIdConcern(new int (id));
    msg.setIdDest(new int(id));
    msg.setIdSender(new int(0));
    sendDataToUART(msg);
}

/**
 * METHOD
 *
 * @brief ServeurTcp::getComputer : TODO
 * @param c
 * @param id
 * @return
 */
bool ServeurTcp::getComputerWithId(Computer &c, int id) {
    for (unsigned int i=0; i<computers.size(); i++) {
        if (computers[i].getId() == id) {
            c = computers[i];
            return true;
        }
    }
    return false;
}

/**
 * METHOD
 *
 * @brief ServeurTcp::getComputerWithIndexOfSocket : TODO
 * @param c
 * @param indexOfSocket
 * @return
 */
bool ServeurTcp::getComputerWithIndexOfSocket(Computer &c, int indexOfSocket) {
    for (unsigned int i=0; i<computers.size(); i++) {
        if (computers[i].getIndexOfSocket() == indexOfSocket) {
            c = computers[i];
            return true;
        }
    }
    return false;
}



/**
 * METHOD
 *
 * @brief ServeurTcp::addNewBoat : add a new boat to the boats vector
 * @param b
 */
void ServeurTcp::addNewBoat(Message b) {
    Boat boat;
    // Init the boat
    boat.init(b);
    // Push it in the vector boats
    boats.push_back(boat);
}

/**
 * METHOD
 *
 * @brief ServeurTcp::addNewComputer : add a new computer to the computers vector
 * @param c
 * @param indexOfSocket
 */
void ServeurTcp::addNewComputer(Message c, int indexOfSocket) {
    Computer computer;
    // Init the boat
    computer.init(c, indexOfSocket);
    // Push it in the vector boats
    computers.push_back(computer);
}

/**
 * METHOD
 *
 * @brief ServeurTcp::addNewWeatherStation : add a new weatherStation to the weatherStations vector
 * @param ws
 */
void ServeurTcp::addNewWeatherStation(Message ws) {
    WeatherStation weatherStation;
    // Init the boat
    weatherStation.init(ws);
    // Push it in the vector boats
    weatherStations.push_back(weatherStation);
}

/*--------------------------*
 *                          *
 *          SLOTS           *
 *                          *
 *--------------------------*/
/**
 * SLOT -> this slot is called when newConnection() signal is emitted
 *
 * @brief ServeurTcp::demandeConnexion : TODO
 */
void ServeurTcp::demandeConnexion() {
    // On crée une nouvelle socket pour ce client
    QTcpSocket *nouveauClient = nextPendingConnection();
    clients << nouveauClient;

    connect(nouveauClient, SIGNAL(readyRead()), this, SLOT(readDataFromTCPIP()));
    connect(nouveauClient, SIGNAL(disconnected()), this, SLOT(deconnexionClient()));
}

/**
 * SLOT -> this slot is called when readyRead() signal is emitted
 *
 * @brief ServeurTcp::donneesRecues : TODO
 */
void ServeurTcp::readDataFromTCPIP() {
    // On reçoit un paquet (ou un sous-paquet) d'un des clients
    // On détermine quel client envoie le message (recherche du QTcpSocket du client)
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket == nullptr) // Si par hasard on n'a pas trouvé le client à l'origine du signal, on arrête la méthode
        return;

    // Si tout va bien, on continue : on récupère le message
    QDataStream in(socket);

    if (tailleMessage == 0) { // Si on ne connaît pas encore la taille du message, on essaye de la récupérer
        if (socket->bytesAvailable() < int(sizeof(quint16))) // On n'a pas reçu la taille du message en entier
            return;

        in >> tailleMessage; // Si on a reçu la taille du message en entier, on la récupère
    }

    // Si on connaît la taille du message, on vérifie si on a reçu le message en entier
    if (socket->bytesAvailable() < tailleMessage) // Si on n'a pas encore tout reçu, on arrête la méthode
        return;

    // Si ces lignes s'exécutent, c'est qu'on a reçu tout le message : on peut le récupérer.
    QString message;
    in >> message;

    emit received_data(message);

    Message data;
    data.decodeData(message);
    // Verify if an error occured during the decoding
    if (!data.getError()) {
        // Check if it's the first connection
        if (!checkConnectionTCPIP(data, socket)) {
            // If not we must send the datas to the boat
            sendToBoat(data, *data.getIdSender());
        }
    }

    // Remise de la taille du message à 0 pour permettre la réception des futurs messages
    tailleMessage = 0;
}

/**
 * SLOT -> this slot is called when disconnected() signal is emitted
 *
 * @brief ServeurTcp::deconnexionClient : TODO
 */
void ServeurTcp::deconnexionClient() {
    // On détermine quel client se déconnecte
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if (socket == nullptr) // Si par hasard on n'a pas trouvé le client à l'origine du signal, on arrête la méthode
        return;

    int index = clients.indexOf(socket);
    for (unsigned int i=0; i<computers.size(); i++) {
        if (computers[i].getIndexOfSocket() == index)
            computers.erase(computers.begin() + int(i));
    }
    clients.removeOne(socket);

    socket->deleteLater();
}

/**
 * SLOT -> this slot is called when receivedDataFromUART() signal is emitted
 *
 * @brief ServeurTcp::getDataFromUART : TODO
 * @param msg
 */
void ServeurTcp::readDataFromUART(Message msg) {
    if (msg.getError())
        emit received_data(QString("Error when decoding the message from the UART."));
    else {
        emit received_data(msg.encodeData());

        if (!msg.getError() && !checkConnectionUART(msg)) {
            // Transmit datas to computer
            if (*msg.getIdSender() > 0) {
                // This message comes from a boat
                // First of all, treat the datas
                treatBoatDatas(msg);

                Computer c;
                if (getComputerWithId(c, *msg.getIdConcern())) {
                    // Send all datas to the computer which is linked to the boat
                    sendToComputer(msg, *msg.getIdConcern());
                    // Send longitude and latitude to other boats
                    sendToAllBoatsExcept(msg, *msg.getIdConcern());
                    // Send longitude and latitude to other computers
                    sendToAllComputersExcept(msg, c.getIndexOfSocket());
                }
            } else if (*msg.getIdSender() < 0) {
                // This message comes from a weather station
                sendToAllExceptWeatherStation(msg);
            }
        }
    }
}
