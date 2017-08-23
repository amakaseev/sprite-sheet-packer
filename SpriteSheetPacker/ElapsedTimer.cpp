#include "ElapsedTimer.h"

ElapsedTimer::ElapsedTimer(QObject* parent): QTimer(parent) {
    connect(this, &QTimer::timeout, [this](){
        emit timeout(elapsed());
        _time.restart();
    });
}

void ElapsedTimer::start() {
    _time.start();
    QTimer::start();
}

void ElapsedTimer::start(int msec) {
    _time.start();
    QTimer::start(msec);
}

int ElapsedTimer::elapsed() {
    return _time.elapsed();
}

