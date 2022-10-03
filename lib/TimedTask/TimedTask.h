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
    uint16_t id;
public:
    uint8_t type;
    time_t trigger_time;
    time_t interval;

    Task();
    Task(TaskCallback *task, uint16_t id, uint8_t type, time_t trigger_time);
    Task(TaskCallback *task, uint16_t id, uint8_t type, time_t start_time, time_t interval);
    ~Task();
    void run();
    uint16_t getID();
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
    void deleteTask(uint16_t id);
    void loop();
};



#endif