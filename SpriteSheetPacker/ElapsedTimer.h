#ifndef ELAPSEDTIMER_H
#define ELAPSEDTIMER_H

#include <QtCore>

class ElapsedTimer: public QTimer
{
    Q_OBJECT
public:
    ElapsedTimer(QObject* parent);

    void start();
    void start(int msec);

    int elapsed();

signals:
    void timeout(int elapsed);

private:
    QTime _time;
};


#endif // ELAPSEDTIMER_H
