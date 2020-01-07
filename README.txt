Group: Emily Michaud, Nicholas Michaud, and TJ Goldblatt

Video: https://youtu.be/yO-G4lfl2Qw

General Overview:
The intent of our Threads and Synchronization project was to simulate getting tickets by multiple people at the same time for the tonight show with Jimmy Fallon. 
This project involved the use of creating multiple threads for each phone call, and synchronization of these threads using semaphores in critical sections of code. In this 
project we assume there are 5 call lines that callers can connect to at a time, and only 3 operators to buy the tickets. When a phone call arrives, first we had to check if 
there was a free line. If so, the caller will connect and then have to wait for 1 of the 3 operators to be available to buy a ticket, if not the phone line is busy and the caller 
then has to call again. 

For this program we created a makefile, which can be used to compile the jimmy_fallon.c file into an executable called jimmy_fallon. We also have a header file called jimmy_fallon.h, 
which contains the prototype for the phonecall function and defines JIMMY_FALLON_H to ensure it is not included and compiled more than once.

Our implementation of the Jimmy Fallon project begins in the main function of the jimmy_fallon.c file, which first checks to see if an integer argument for the amount of callers has 
been passed in through the command line. If argc is greater than 1, meaning an argument has been included in the command line, we change the string argument held in argv[1] to an integer
using the atoi() function. If the number of callers passed in is greater than 240 or if no argument was been passed in at all, then we use 240 as the default value. We then create a dynamically
created integer pointer called callersLeft which is set to the number of total callers, which will be used to indicate when the last thread is running. Two binary semaphores, meaning semaphores 
that only allow one thread at a time to access a critical section, are created called initalize_lock and callers_lock, which are global semaphores that are used for the initialization and destruction 
of the semaphores in the thread function. Because they are global semaphores, they are accessible to all threads since they reside in the data segment of memory. Next we use a for loop to create a 
number of threads equal to the number of callers using pthread_create(). We pass a pointer to the phonecall() function to be the function each newly created thread will begin execution in, and we pass 
in callersLeft integer pointer casted to a void pointer. The next for loop calls pthread_join() using the thread id's of each thread to wait for every thread to finish executing to reap them.

Our thread function is called phonecall(), which simulates the process of each caller calling in to buy a ticket to the Jimmy Fallon show. First we take our argument, vargp, and case it back to an integer pointer
called callersLeft as this was the pointer that was passed in from the main function pointing to the total number of callers yet to buy a ticket for the show. We then create 3 static semaphores called connected_lock, 
operators, and id_count. Since they are static semaphores this means they are initalized in the data segment of memory, which means that each thread has access to these semaphores. connected_lock is a binary semaphore 
that allows one thread at a time to access the connected static variable to check if a connection can be made. The connected static variable keeps track of how many callers/threads are currently connected to the 5 open
lines and it is protected with the connected_lock semaphore to ensure only one thread can manipulate the variable at a time to avoid race condition problems. The id_count semaphore is another binary semaphore that only allows
one thread at a time to access the global next_id variable, meaning that only one thread at a time can increment the next_id variable so that each thread gets a unique id from 1 to the total number of callers. This is again to 
avoid race conditions when incrementing and assigning the global variable next_id to the id of each thread, since, due to it being a global variable, each thread has access to it. The last semaphore, operators, is a counting 
semaphore that allows 3 callers/threads access to the limited number of operators at a time to buy a ticket. This function also initalizes an integer called id, which is a automatic local variable that stores the id recieved from
incrementing the next_id global variable. This variables's value is stored on the stack in memory, and since each thread has it's own individual stack, it cannot be accessed by any thread other than its own thread. 

Because we only want to initialize all of the semaphores once, we use the initalize_lock global semaphore to protect this critical region. We use the sem_wait() function to lock the initalize_lock semaphore, and then we check the boolean
variable initialized to see if it equals false (which is what it is initialized to originally). If it is, we then set initialized to true so that we do not initialize the semaphores more than once, and then all 3 semaphores are initialized 
before we call the sem_post() function to unlock the initalized_lock semaphore to allow the next thread access to this critical region. We then use sem_wait on the id_count semaphore to increment the next_id global variable and set 
it equal to the local variable id, then call sem_post() to unlock this semaphore and allow the next waiting thread access to this critical region to increment the next_id variable again and set it's id to this new value and so on. 

The next part of this function simulates the caller trying to connect to one of the 5 open lines. We enter a while loop that loops forever and can only be exited once the caller/thread "connects" to one of the open lines. A message 
is printed before this while loop to notify us that the caller with its specific id is attempting to connect. In this while loop we first call sem_wait on the connected_lock semaphore to lock it. We then check if the connected variable equals NUM_LINES, 
which is a static variable initalized to 5 for the number of open lines. If it does not equal 5 this means there is at least one open line available, so the thread increments the connected variable to account for this one connecting, then calls sem_post() 
to unlock the connected_lock semaphore and allow the next waiting thread to test the connected variable, and then the thread breaks out of the while loop. Otherwise if connected does equal 5, this means all the lines are busy, and so we call sem_post() 
on the connected_lock semaphore and then the caller must call again by going through the while loop again after printing a message that the caller needs to call again due to all the lines being busy and sleeping for 3 seconds to slow down how many times 
the thread will loop through and check if it can connect, which allows time for the currently connected threads to buy tickets and disconnect. Each thread will continue looping through this while loop testing the connected variable until they all finally 
connect and break out of it.

Once the thread breaks out of the while loop, a message is printed that the caller with its id is now waiting for an operator. The next step is for the caller to wait for an available operator. We call the sem_wait() function with a pointer to the operators 
semaphore to lock this counting semaphore, which only allows 3 threads access to the critical section of the code that deals with buying tickets at a time. If three threads are already accessing this part of the code, they will block until one of the threads 
in the critical section exits using sem_post() and unlocks the semaphore. Once a thread obtains access to the critical section of this counting semaphore, which simulates the caller obtaining access to one of the operators, a message is printed to indicate that 
the caller, with its id, is now talking to an operator. The thread then sleeps for 3 seconds to simulate the ticket transaction. The program then prints out that the caller with its specific id has bought a ticket. We then call the sem_wait() function on the 
connected_lock semaphore in order to obtain access to the connected variable to decrement it, since the caller has now bought a ticket and is going to hang up and disconnect, (which will allow the next caller that checks connected in the while loop to connect and 
increment the number of threads connected, and then break out of the while loop to then wait for an operator). We then call sem_wait() on the global callers_lock semaphore, which allows a single thread access to the callersLeft pointer that points to an integer 
stored on the heap that contains the total number of callers left to buy a ticket. Since the thread just completed buying a ticket, we decrement the integer value pointed to by callersLeft. The function then calls sem_post() on the operators semaphore to 
exit the critical region for connecting with the operators and buying a ticket, allowing the next blocked thread to obtain access to this critical region to buy a ticket. A message is then printed that the caller with it's specific id has hung up. Finally, 
we call sem_wait() on the callers_lock semaphore again, which then allows a single thread at a time access to the callerLeft pointer to check if it equals 0, which indicates it is the last thread running as there are 0 callers left that need to buy a ticket. 
If it is indeed the last thead running, we call sem_destroy() on all of the semaphores initialized in this function. We then call sem_post with the callers_lock pointer to unlock this critical region, and then return 0.

Finally, at the very end of the main function we destroy the two global semaphores initalize_lock and callers_lock using sem_destroy(), and then we free the memory allocated on the heap for the callersLeft integer pointer using the free() function and then return 0. 

