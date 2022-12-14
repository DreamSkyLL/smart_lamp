#include "TimedTask.h"
Task::Task() {}
Task::Task(TaskCallback *task, uint16_t id, uint8_t type, time_t trigger_time) : id(id), task(task), type(type), trigger_time(trigger_time) {}
Task::Task(TaskCallback *task, uint16_t id, uint8_t type, time_t start_time, time_t interval) : id(id), task(task), type(type), trigger_time(start_time), interval(interval) {}
Task::~Task() {}

void Task::run()
{
    this->task();
}

uint16_t Task::getID() {
    return this->id;
}

void Task::update()
{
    switch (this->type)
    {
    case DISPOSABLE:
        this->active = false;
        break;
    case PERIODIC:
        this->trigger_time = this->trigger_time + this->interval;
        Serial.printf("this->trigger_time: %lld, this->interval: %lld\n", this->trigger_time, this->interval);
        break;
    default:
        break;
    }
}

bool Task::shouldRemove()
{
    return !this->active;
}

// TimedTask::TimedTask() {}

TimedTask::TimedTask(NTPClient timeClient) : timeClient(timeClient) {}

TimedTask::~TimedTask() {}

void TimedTask::addTask(Task task)
{
    this->tasks.add(task);
}

void TimedTask::deleteTask(uint16_t id)
{
    for (uint8_t i = 0; i < this->tasks.size(); i++)
    {
        Task task = tasks.shift();
        // Serial.printf("task id: %d, mqtt id: %d\n", )
        if (task.getID() != id)
            tasks.add(task);
    }
}

void TimedTask::loop()
{
    this->count++;
    if (this->count < 250)
        return;
    timeClient.update();
    for (uint8_t i = 0; i < this->tasks.size(); i++)
    {
        Task task = tasks.shift();

        if (this->timeClient.getEpochTime() > task.trigger_time)
        {
            task.update();
            task.run();
        }

        if (!task.shouldRemove())
            tasks.add(task);
    }
}
