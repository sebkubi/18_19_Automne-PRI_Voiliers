#include "serialdata.h"

/**
 * @brief SerialData::SerialData
 * @param port
 * @param parent
 */
SerialData::SerialData(QString port, QObject *parent) : QObject(parent) {
    mPort = new QSerialPort(this);
    connect(mPort, SIGNAL(readyRead()), this, SLOT(readData()));

    mPort->setPortName(port);
    mPort->setBaudRate(QSerialPort::Baud115200);
    mPort->setDataBits(QSerialPort::Data8);
    mPort->setParity(QSerialPort::NoParity);
    mPort->setStopBits(QSerialPort::OneStop);
    mPort->setFlowControl(QSerialPort::NoFlowControl);

    if(mPort->open(QIODevice::ReadWrite)){
        mPort->setTextModeEnabled(true);
        qDebug() << "Port open at " << mPort->portName();
    }
}

SerialData::~SerialData() {
    delete mPort;
}

/**
 * @brief SerialData::sendData
 * @param msg
 */
void SerialData::sendData(Message msg) {
    // Encode the data
    QString data = msg.encodeData();
    // Send it to the UART
    mPort->write(data.toStdString().c_str(), data.length());
}

/**
 * @brief SerialData::readData
 */
void SerialData::readData() {
    QString data = mPort->readAll();
    qDebug() << "Serial data IN : " << data;
    // Decode data
    Message msg;
    msg.decodeData(data);
    // Emit a signal to inform the server we received
    // datas from the UART
    emit receivedDataFromUART(msg);
}
