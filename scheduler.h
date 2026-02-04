#ifndef SCHEDULER_H
#define SCHEDULER_H

// Function Prototypes (The Menu)
void *logger_thread(void *arg);
void *scheduler_thread(void *arg);
void enqueue_log(char *msg);

#endif
