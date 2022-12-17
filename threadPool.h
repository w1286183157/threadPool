#pragma once

#include <string.h>
#include <pthread.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
/**
 * 线程池 
 */
typedef struct Task
{
    void (*function)(void *arg);
    void *arg;
} Task;

class threadPool
{
public:
    threadPool *threadPoolCreate(int threadNum = 8, int queueCapacity = 10);
    ~threadPool();
    int append(void (*func)(void *), void *arg);

private:
    Task *taskQ;             /*任务队列*/
    int queueFront;          /*队头*/
    int queueRear;           /*队尾*/
    int current_queueLength; /*当前队列长度*/
    int queueCapacity;       /*队列容量*/
    bool m_stop;             /*终止*/

    pthread_t *m_threads;            /*线程池数组*/
    int threadNum;                   /*线程个数*/
    pthread_mutex_t taskMutex;       /*队列锁*/
    pthread_cond_t taskNotEmptyCond; /*通知队列有任务*/

    static threadPool *tp;
    threadPool(int, int);
    static void *worker(void *arg);
};

threadPool *threadPool::tp = nullptr;

threadPool::~threadPool(){
    delete[]taskQ;
    delete[]m_threads;
    m_stop=true;
}

int threadPool::append(void (*func)(void *), void *arg)
{
    pthread_mutex_lock(&taskMutex);
    taskQ[queueRear].function = func;
    taskQ[queueRear].arg = arg;
    queueRear = (queueRear + 1) % queueCapacity;
    current_queueLength++;
    pthread_mutex_unlock(&taskMutex);
    pthread_cond_broadcast(&taskNotEmptyCond);
    return 0;
}

void *threadPool::worker(void *arg)
{
    threadPool *pool = (threadPool *)arg;

    while (!pool->m_stop)
    {
        pthread_mutex_lock(&pool->taskMutex);
        while (0 == pool->current_queueLength)
        {
            pthread_cond_wait(&pool->taskNotEmptyCond, &pool->taskMutex);
        }
        Task task;
        task.function = pool->taskQ[pool->queueFront].function;
        task.arg = pool->taskQ[pool->queueFront].arg;

        pool->queueFront = (pool->queueFront + 1) % pool->queueCapacity;
        pool->current_queueLength--;
        pthread_mutex_unlock(&pool->taskMutex);
        /*开始工作*/
        task.function(task.arg);
    }
    return NULL;
}

threadPool *threadPool::threadPoolCreate(int threadNum, int queueCapacity)
{
    if (tp == nullptr)
    {
        tp = new threadPool(threadNum, queueCapacity);
        return tp;
    }
    else
    {
        return tp;
    }
}

threadPool::threadPool(int tNum, int qCap) : threadNum(tNum), queueCapacity(qCap)
{
    m_stop=false;
    /*创建任务队列*/
    taskQ = (Task *)malloc(sizeof(Task) * queueCapacity);
    if (NULL == taskQ)
    {
        printf("taskQ malloc error\n");
        throw std::exception();
    }
    queueFront = 0;
    queueRear = 0;
    current_queueLength = 0;
    /*创建线程池 并分离*/
    m_threads = (pthread_t *)malloc(sizeof(pthread_t) * threadNum);
    if (NULL == m_threads)
    {
        printf("threads malloc error\n");
        throw std::exception();
    }
    memset(m_threads, 0, sizeof(pthread_t) * threadNum);
    for (int i = 0; i < threadNum; i++)
    {
        pthread_create(&m_threads[i], NULL, worker, this);
        pthread_detach(m_threads[i]);
    }

    /*初始化锁*/
    if (pthread_mutex_init(&taskMutex, NULL) != 0 || pthread_cond_init(&taskNotEmptyCond, NULL) != 0)
    {
        printf("mutex init error\n");
        throw std::exception();
    }
}