# ARM Micro Kernel

CS 452 "Realtime System" Course Project  
Member: [Greg Wang](https://github.com/gregwym), [Tony Zhao](https://github.com/tonyzx)

## Overview

This is a micro kernel run on TS-7200 (ARM720t device), following the spec from [http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/pdf/kernel.pdf](http://www.cgl.uwaterloo.ca/~wmcowan/teaching/cs452/pdf/kernel.pdf)

## 2. How to...

To compile the source, 

* execute the `make clean; make all` command. 
* *make* will compile the program into `./main.elf`

To execute, 

* Load the program from `ARM/y386wang/main.elf`
* Send `go` to start execution

To control the train, send one of the following command

* `tr <train_id> <speed>` 	assign train speed
* `rv <train_id>` reverse train direction and reaccelerate
* `sw <switch_id> <direction>` assign switch direction
* `q` quit the program
	
Note: 

* `train_id` and `speed` must be within INTEGER range
* `speed` other than [0 - 14] has unspecified side-effects
* `switch_id` is limited in range [1 - 18] and [153 - 156]
* `direction` is limited as either `C` or `S`


## 3. Data Structures

### 3.1 General Purpose ADTs

#### Heap

Heap is a ADT that sort `HeapNode`s according to their `key`s upon each insertion and pop with runtime O(log n). On pop, the `HeapNode` with min/max `key` is removed from the ADT and returned. Whether is min/max depends on the Insert and Pop function. 

* `Heap`
	* maxsize: the max size for the heap
	* heapsize: record the number of `HeapNode` in the heap
	* data: array of pointer of `HeapNode`
* `HeapNode`: the structure that is stored in `Heap`
	* key: the value to be sorted (i.e., priority number, delay time)
	* datum: pointer to the actual datum
	
#### Circular Buffer

Circular Buffer is a ADT that save and pop char/int/pointer in FIFO fashion with runtime O(1). 

* `CircularBuffer`
	* type: Type of data saved in this buffer, one of  
	  [CHARS, INTS, POINTERS, VOIDS]
	* front: The index of the first item
	* back: The index of the last item
	* current_size: The number of item currently in the buffer
	* buffer_size: The max number of item can be saved into the buffer
	* buffer: The address of the item actually save data
		* which is a union of char/int/pointer/void array

### 3.2 Context-switch Handling

* `UserTrapframe`: contains the entire CPU register and task state dump
	* spsr: The `cpsr` of the user task before it comes into the kernel
	* resume_point: Address that `pc` should move to when the task is resumed
	* r0-lr: The CPU registers at the entering kernel time

### 3.3 Task Management

* `Task`: contains the information needs for each task to operate
	* tid: Task ID
	* state: State of the task, one of   
	  [Active, Ready, Zombie, SendBlocked, ReceiveBlocked, ReplyBlocked, EventBlocked]
	* priority: Priority level (0 is the highest level)
	* init_sp: Initial stack position (the `sp` of a newly created task)
	* current_sp: Current stack position (also the pointer to a `UserTrapframe`)
	* next: Pointer of `Task` that is supposed to run after this one
	* parent_tid: The parent's tid

* `ReadyQueue`: used to schedule all ready to run tasks
	* curtask: Address of the current executing `Task`
	* head: Address of the highest priority `Task`, which should be executed next
	* A `Heap` that store and sort all active priorities. 
	* Search the next task to be scheduled takes O(1) and O(log n) in worst, where n is the max number of priority.

* `FreeList`: links all idle task descriptors together, so that we can get a idle task descriptor in O(1) when we create a task
	* head: Head of the list
	* tail: Tail of the list
* `FreeList` is FIFO

### 3.4 Blocked Task Tracking

* `BlockedList`: contains a linked-list of `Task` that is blocked by a Receiver or an Event
	* head: First task that is blocked by a Receiver or an Event
	* tail: Last task that is blocked by a Receiver or an Event
* There are two array of `BlockedList`s
	* `receive_blocked_lists`: Each task is directly mapping to a `BlockedList` to track all senders who are waiting to send to this task.
	* `event_blocked_lists`: Each event is directly mapping to a `BlockedList` to track all tasks who are waiting for this event. 
* Since all receivers and events are directly mapped. The `BlockedList` can be accessed in O(1). 
* `BlockedList` is FIFO

### 3.5 Send/Receive/Reply/AwaitEvent Buffer Tracking

* `MsgBuffer`: store the Send/Receive/Reply/AwaitEvent message information
	* msg: Address of the sender's sending msg buffer
	* msglen: The sending msg length
	* reply: Address of the sender's replying msg buffer
	* replylen: The max length of reply msg can be received
	* receive: Address of the receiver's msg buffer
	* receivelen: The max length of msg can be received
	* sender_tid_ref: Address of where receiver's get the sender's tid
	* event: Address of the volatile data buffer
	* eventlen: The max length of volatile data can be saved
* Each task is associated with a `MsgBuffer` to store its buffer information
	* Each task's `MsgBuffer` is directly mapped in an array, so the Receive, Reply syscalls and Event interrupt handler can find the message buffers in O(1). 

### 3.6 Name Server
#### Registration Records

* `NSRegEntry`: an entry that stores registration name and corresponding tid
	* name: a fixed-size(16 so far) char array that stores a task's registration name
	* tid: the task's tid
* A fixed-size(12 so far) array of `NSRegEntry`, stores all registration records

#### Message Protocol

* `NSQuery`: all query to the Name Server follow this protocol structure
	* type: either `REG` or `WHO`, represents the type of query
	* name: the address of the name string
* `NSReply`: all reply from the Name Server follow this protocol structure
	* result: either `FND` or `NOT`, represents whether has found the desired name
	* tid: the query name's corresponding tid, if the result is `FND`

### 3.7 Clock Server

#### Delayed Task Tracking

- The task being delayed are tracked by a `Heap`. Each `HeapNode`, 
	- key: the expiration time of the delay request
	- datum: the tid of the delayed task

`Heap` ensure the delayed tasks are sorted according to their expiration times and limit the insert and pop time within O(log n)

#### Message Protocol

 - `ClockServerMsg` unions all types of msg query, and distinguish them by type
	- type: type of the query
	- `DelayQuery`: structure that contains information which is needed by "Delay()"
	- `DelayUntilQuery`: structure that contains information which is need by "DelayUntil()"
	- `TimeQuery`: structure that contains information which is need by "Time()"
- `TimeReply` is the reply msg format for "Time()"
	- time: current system time

### 3.8 I/O Server (comserver)

#### Message Buffering

- The received bytes are stored in a char `Circular Buffer`. It provides O(1) time of save and pop. 
- The send bytes are stored in a char array, with a size counter. It provides O(1) save and easier `Reply` to the Send Notifer. 

#### Message Protocol

- `IOMsg` unions all types of msg query, and distinguish them by type
	- type: type of the query
	- ch: sometime one byte is enough
	- `PutcQuery`: message of "Putc()" request
	- `PutsQuery`: message of "Puts()" request
	- `GetcQuery`: message of "Getc()" request


## 4. Program Structure

### 4.1 Initialization

System boots up in SVC mode

1. Turn off Interrupt in SVC mode
2. Config Hardwares
	1. Config UART2 and enable receive interrupt
	2. Config UART1 and enable receive & modem interrupt
	3. Set Timer1 to period mode with 20 as Load Value and 2kHz
	4. Set Timer3 to free-run mode
	5. Enable Timer1, UART2 and UART1 Interrupt in VICs
	6. Enable the instruction cache
3. Initialize Task related data structures
	1. Ready Queue
	2. Free Task Descriptor List
	3. Receive Blocked Task List
	4. Event Blocked Task List
	5. Message Buffer
	2. Allocate a fixed-size space for user stack
3. Config kernel entries
	1. Set the address of `swiEntry()` to `0x28`
	2. Set the address of `irqEntry()` to `0x38`
4. Create and schedule the first task
5. Exit kernel

### 4.2 Context-switch

#### Enter Kernel

In order to handle the IRQ and SWI in the same fashion, two kernel entry point are used. The two entry point does the following things, 

1. Save user trapframe
	1. Switch to SYSTEM mode
	2. Store all registers on user task's stack, except `pc`
	3. Move `sp` down 60 bytes (15 registers in total)
	4. Save `sp` to `r2`
	5. Switch back to SVC/IRQ mode, depends on which entry point it is
	6. Save `spsr` to `r3`
	7. Stroe `r3`(spsr) and `lr` to user task's stack(`r2` is the `sp` of user task)
2. If is in IRQ entry, switch to SVC mode and set `r0` to 0 (indicating IRQ)
3. Restore kernel's trapframe from kernel's stack (include `pc`)
4. Save information and invoke appropriate handler
	1. Enter `handlerRedirection` function with 
		- `r0` as syscall parameters
		- `r1` as kernel global variables
		- `r2` as last user task's `sp`
	2. Save user task's `sp` to the task descriptor
	3. Invoke `irqHandler()` if parameters(`r0`) == NULL
	4. Invoke `syscallHandler()` otherwise

Therefore, there are 17 registers in total has been saved to user task's stack, which forms the structure of `UserTrapFrame`. These value can be accessed by dereferencing the saved user task's `sp` and cast to `UserTrapFrame`. 

#### Exit Kernel

1. Schedule which task should run next. If no task is left to run, break the loop, system halt. 
2. Push kernel trapframe on kernel stack
3. Restore `spsr` and `lr` from the user stack
3. Restore task's user trapframe from the user stack
4. Switch to user mode
	* Use `movs` to set `pc` to task's resume point, and
	* switch back to user mode

### 4.3 Task Manipulation Syscalls

* Create:
	* Pop head of the `FreeList` as new task descriptor
	* Config the task descriptor with given priority and code entry
	* Initialize its trapframe on its stack
	* Add it into `ReadyQueue` and set its state to Ready
* Exit:
	* Remove current task from the `ReadyQueue`, and set its state to Zombie
* Pass:
	* Do nothing
* MyTid:
	* Set current task's tid to `r0` of the user task's trapframe
* MyParentTid
	* Set current task parent's tid to `r0` of the user task's trapframe

### 4.4 Message Passing Syscalls
* Send
	* Save task's reply buffer information into its `MsgBuffer`
	* If the msg recipient is at *SendBlocked* state
		* Copy task's msg to receiver's buffer
		* unblock the receiver task (push it back to the ready queue) and change its state to *Ready*
		* Block itself and change state to *ReplyBlocked*
	* Otherwise, 
		* Save task's msg buffer information into its `MsgBuffer`
		* Add itself to the recipient task's `BlockedList`
		* Block itself and change state to *ReceiveBlocked*
* Receive
	* If there is *ReceiveBlocked* senders in task's `BlockedList`
		* Remove the first one from the `BlockedList` and change its state to *ReplyBlocked*. 
		* Copy the sending msg according to the information in sender's `MsgBuffer`. 
	* Otherwise, block itself and change the state to *SendBlocked*
* Reply
	* Copy the reply msg to the sender according to the information in sender's `MsgBuffer`.
	* Unblock the sender task and change its state to Ready

### 4.5 AwaitEvent and Interrupt Handling

When a task called AwaitEvent, if it is a valid event_id, change the task's state to `EventBlocked` and move it from ready queue to the event's corresponding `EventBlockedList`. The provided `event` buffer and `eventlen` are saved in the MsgBuffer array. 

Depends on the event type, when the event happens, all or one of the tasks in the event's corresponding `EventBlockedList` are moved back to ready queue. If there is any volatile data, the data are written to the `event` buffer. 

#### Timer Event & Interrupt

Timer1 was initialized to generate interrupt every 1/100 second. When a timer interrupt happens, 

* Clear the interrupt on timer register
* Move all tasks that are blocked in Timer Event's blocked list back to the ready queue

#### UART Event & Interrupt

UART1 is initialized to generate RX interrupt only. UART2 is initialized to generate RX and MS interrupt. UART1's CTS waiting flag is initialized to TRUE. 

When a task start to wait for one kind of UART Event, the Event's corresponding interrupt will be turned ON. 

When a UART interrupt happens, 

1. If is a Modem Status interrupt
	* If CTS is true, set the CTS waiting flag to TRUE
	* Clear the interrupt
2. Or is Receive interrupt
	* If there is a blocked task, copy the byte to event buffer and unblock it
	* Otherwise, turn OFF the RX interrupt and return
3. Or if is a UART1 Transmit interrupt and CTS waiting flag is FALSE
	* turn OFF the TX interrupt and return
4. Or if is a Transmit interrupt
	* If there is a blocked task, 
		* Copy the byte from event buffer to TX buffer
		* If is UART1, set CTS
		* unblock the task
	* Otherwise, turn OFF the TX interrupt and return


## 5. Algorithms

### 5.1 Scheduler

Before `syscallHandler()` or `irqHandler()` returns, the head of `TaskList` is always up to date. Thus we can guarantee the head task is always the task with the highest priority task before enter the scheduling process start.

In `syscallHandler()`, if the syscall was `Exit()`, set current task to NULL, otherwise keep the current task.

During scheduling, if the current task is not NULL, move the current task to the end of its priority queue. Then, assign current task to the value of the head task, since head task is always the highest priority task.

### 5.2 ReadyQueue & FreeList

The `ReadyQueue` contains a min `Heap` which sort all priority in ascending order. Each priority is mapped to a `TaskList` which is a linked-list of `Task`

Therefore, base on the `Heap` property, 

- Pop the highest priority task takes O(1) if that priority has more than one ready task. 
- Otherwise, it takes O(log n), where *n* is the number of priority currently active. 
- Insert a task with a priority exists in the `Heap` takes O(1). 
- Otherwise, it takes O(log n). 

The `FreeList` is a linked-list of all *Zombie* task descriptors which store and pop them in O(1). Popping in FIFO ensure that none of the free-running task descriptor will be used much more often than others. 

### 5.3 Name Server

The name server use linear search to find out whether the name is registered. The same name can be replaced by a different task.

The reason of using linear search is, a task should not need to look for a name multiple times. By restricting the number of registration, linear search will not affect the overall performance much. Thus, it is not worth the effort to design and implemented a smarter yet much more complicated algorithm at this stage.

The limitation of our implementation is that, a registered entry will never be removed from the array. If too many tasks are registering to Name Server, the Name Server will override previous records to keep the service alive. 

* RegisterAs
	* Construct a `NSQuery` with `REG` flag and desires registration name. 
	* Send the query with Send and expect zero length reply, since RegisterAs will never fails. 
* WhoIs
	* Construct a `NSQuery` with `WHO` flag and the look up name. 
	* Send the query with Send and expect to receive a `NSReply`. 
	* `NSReply` contains the look up result and tid (if result was `FND`). 

### 5.4 Clock Server

Clock server distinguish different type of messages base on its sender's tid and message type in the union. 

When a task send a delay request to the clock server, the clock server choose a HeapNode that is corresponding to the task's tid. Then set node's key as the delay expiration time, set datum as task's id, and push the node into the min `Heap`. 

When a task send a time request to the clock server, the clock server reply with the current time. 

When notifier sends clock server a msg, tick counter increases by 1, and the clock server checks whether the head of the min `Heap` should be popped in a while-loop. Since the insert and pop a `Heap` take O(log n), we believe this algorithm is efficient enough to handle the function well. 

### 5.5 Clock Server Notifier

Notifier is a forever loop which does two things, 

* Wait for timer event
* Send to Clock Server, notify a tick has been elapsed

Base on the statistic from Interrupt handler, the Notifier is EventBlocked every time the timer interrupt appears, so there should be no missing ticks. 

### 5.6 I/O Server (comserver)

There are two comservers, one for each UART. Each comserver has two notifiers, one for Receive Event and one for Transmit Event. 

The benefits of using two servers and four notifiers structure are, 

* Less running task -> more chance to run for each task
* Less registration in name server -> faster name look up
* Handle echo without Send & Receive -> Less cost on communication

#### Initialization

* Receive the channel id(COM1/COM2) from bootstrap
* Register itself
* Create one circular buffer of chars for receiving: `receive_buffer`
* Create one char array and an size counter for sending: `send_array`, `send_size`
* Create two notifiers, one for sending one for receiving
* Send the channel id to each notifier, so they know which UART and server to serve. 
* Create one circular buffer of ints to store getters' tid
* Set the flag that indicate whether send notifier is waiting for reply to FALSE

#### Server

* Receive a message
	* If it's from send notifier, set the send notifier waiting flag to TRUE
	* Or if it's from receive notifier, reply and push the char received into `receive_buffer`
		* If it is a UART2 server, echo the char by put it into `send_array`
		* And if it is a backspace, echo escape sequence to clear to EOL
	* Or if it's a Getc, push the caller tid into `tid_buffer`
	* Or if it's Putc, reply add push the char into `send_array`
	* Or if it's Puts, reply and copy the entire string into `send_array`
* If send notifier is waiting and `send_array` is not empty, reply with the `send_array` and size of `send_size`. Then set the send notifier waiting flag to FALSE and set `send_size` to 0. 
3. If neither `tid_buffer` nor `receive_buffer` is empty, pop a tid from `tid_buffer`, pop a char from `receive_buffer`, and then reply that tid with that buffer

#### Send Notifier

Send Notifier start with Receive the serving channel and server info from the server, then loop on

1. Send server a msg and wait for the reply with a byte to send
2. Call AwaitEvent to send that byte

#### Receive Notifier

Receive Notifier start with Receive the serving channel and server info from the server, then loop on

1. Call AwaitEvent to receive a byte
2. Send the byte to server

#### Why `Puts`

Since `iprintf` use `Putc` to send msg to server, when there are more than one tasks calls `iprintf`, the print order may be screwed up due to interrupt and context-switch. Therefore, we implement Puts to dump the entire string into send buffer to keep print order consistent. It also reduce the time of Send & Receive, compare to `iprintf`. 


### 5.7 Train Server

Train Server initials the UI format, and then create train command server, train clock server and train sensor server.

#### Train Command Server

Train command server contains a fixed-sized char array to store user input. It's main loop does the following things, 

1. Getc from COM2
	1. If receive '\b', remove a char from buffer if it is not empty
	2. If receive '\n' or '\r', parser the user input and reset buffer
		1. If it's a valid "tr" command, use Puts  send commands to COM1
		2. If it's a valid "rv" command, create a new task "reversetrain", and send train number to the new created task
		3. If it's a valid "sw" command, create a new task "changeswitch", and send switch number and switch state to the new created task
		4. Else do nothing
	3. Else, push the ch into buffer

#### reversetrain

1. Receive the train number
2. Stop the train by Puts command to COM1 
3. Delay 2 second
4. Speed up the train by Puts command to COM1

#### changeswitch

1. Receive the switch number and switch state
2. Change the switch state by Puts command to COM1
3. Delay 0.4 second
4. Use Putc to send 32 to COM1

#### Train Clock Server

Train Clock Server start with `tick = 0`, then continuously does the following, 

1. Add `tick` by 10
2. `DelayUntil(tick)`
3. Update clock message

#### Train Sensor Server

Sensor Server start with Putc 192 to COM1 to enable sensor data auto-reset. Then it loop on, 

1. Putc 133 to COM1
2. Getc 10 times (2 byte for each decoder), and print sensor info if it detects any sensor has changed to one

## Credits

* [Greg Wang](https://github.com/gregwym)
* [Tony Zhao](https://github.com/tonyzx)

## License

[The MIT License](http://opensource.org/licenses/MIT)

Copyright (c) 2013 Greg Wang & Tony Zhao
