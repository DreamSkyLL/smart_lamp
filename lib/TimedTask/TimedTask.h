#ifndef TIMED_TASK_H
#define TIMED_TASK_H

#include "TimedTask.h"
#include <Arduino.h>
#include <NTPClient.h>
#include <LinkedList.h>

#define DISPOSABLE 1
#define PERIODIC 2

typedef void TaskCallback();


class Task
{
private:
    TaskCallback *task;
    bool active;
public:
    uint8_t type;
    time_t trigger_time;
    time_t interval;

    Task();
    Task(TaskCallback *task, uint8_t type, time_t trigger_time);
    Task(TaskCallback *task, uint8_t type, time_t start_time, time_t interval);
    ~Task();
    void run();
    void update();
    bool shouldRemove();
};


class TimedTask
{
private:
    LinkedList<Task> tasks;
    uint8_t count = 0;
    NTPClient timeClient;
public:
    // TimedTask();
    TimedTask(NTPClient timeClient);
    ~TimedTask();
    void addTask(Task task);
    void deleteTask(uint8_t index);
    void loop();
};



#endif