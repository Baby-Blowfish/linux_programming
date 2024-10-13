#ifndef THREAD_H
#define THREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QLabel>
class Thread : public QThread {
    Q_OBJECT

public:
    enum { Stop, Play };
    Thread(QObject *obj = 0);

private:
    void run( ) override;   // 부모 클래스를 재구현 virtual

    QLabel *m_label;
    QWaitCondition m_waitCondition;
    QMutex m_mutex;
    qint32 m_stopFlag;

signals:
    void setLabeled(QString);

public slots:
    void stopThread( );
    void resumeThread( );
};

#endif // THREAD_H
