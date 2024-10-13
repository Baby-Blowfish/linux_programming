#include "thread.h"

Thread::Thread(QObject *obj) : m_stopFlag(Play) {
    m_label = (QLabel*)obj;
}

void Thread::run( )
{
    int count = 0;
    Q_FOREVER { /* forerver */
        m_mutex.lock( );        // 뮤텍스를 반드시 걸어야한다. 깨어나고 자고 , 자고나서 꺠어나야 하므로  -> 꺠어나고 자고를 동시에 하면 안되므로 뮤텍스 사용
        if(m_stopFlag == Stop)  
            m_waitCondition.wait(&m_mutex);
        m_mutex.unlock( );
    // m_label->setText(QString("run %1").arg(count++)); /* Non GUI 스레드 */
    emit setLabeled(QString("run %1").arg(count++));
    sleep(1);
    }
}

void Thread::stopThread( )
{
m_stopFlag = Stop;
}
void Thread::resumeThread( )
{
m_mutex.lock( );
m_stopFlag = Play;
m_waitCondition.wakeAll( );
m_mutex.unlock( );
}
