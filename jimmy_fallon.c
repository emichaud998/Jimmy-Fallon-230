#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <stdbool.h>

static int next_id = 0; //Global variable that is incremented from 1-[# of callers] to serve as IDs for each call
static sem_t initalize_lock; //Global semaphore used to initialize the semaphores in the thread function only once
static sem_t callers_lock; //Global semaphore used when decrementing the number of callers left to buy a ticket- used so the last caller is the one that destroys the semaphores in the thread function

/*
*Thread function that each newly created thread invokes and starts executing. Function simulates the process of multiple callers calling in to buy a ticket for the Jimmy Fallon show.
*Each thread represents a caller. The caller attempts to connect and checks if a connection can be made as there are only 5 call lines. If the line is busy (meaning that there are already 5 caller connected), the caller 
*redials and tries to connect again continuously until there is an open line, where the caller will then wait for an operator to be available (only three operators). The caller will then buy a ticket which takes 3 seconds, and then hang up. 
*Parameters:
    vargp = void pointer that in this case is an integer pointer to the number of total callers there are (must be casted back to an integer pointer)
*Return:
    Once the caller (thread) has successfully completed buying a ticket and hangs up, the thread returns with a value of 0
*/
void *phonecall(void *vargp){

    /*Cast the void pointer back to an integer pointer. callersLeft points to a dynamically created integer on the heap (meaning all threads have access to it)
    *Initially has the value of the total number of callers that will be buying tickets. Every time a ticket is bought, callersLeft is decremented. 
    *When callersLeft hits 0, that means that current thread is the last caller and thus needs to destory all of the semaphores initialized in this function*/
    int *callersLeft = (int *)vargp;

    //Each of these semaphores are static, meaning they are initalized in the data segment of memory so each thread has access to these semaphores
    static sem_t connected_lock; //Binary semaphore that only allows one thread at a time to access the connected variable to check if a connection can be made
    static sem_t operators; //Counting semaphore that allows 3 threads/callers access to the limited number of operators 
    static sem_t id_count; //Binary semaphore that only allows one thread to access the global next_id variable at a time to avoid race conditions for assigning and incrementing the ids
    
    static int NUM_LINES = 5; //The number of available lines that callers can connect to at a time
    static int NUM_OPERATORS = 3; //The number of operators available to the callers to manage the buying of tickets 

    /*The number of threads/callers currently connected to the 5 lines, incremented whenever a caller connects and decrements when a caller hangs up. 
    *If it equals 5 a connection cannot be made until a caller hangs up freeing up one of the 5 lines.
    *Because it is static, this it is initialized in the data segment of memory and every thread has access and can manipulate it*/
    static int connected = 0;
    static bool initalized = false; //Initally set to false, once the semaphores are initalized by the first thread it is set to true to ensure the semaphores are not initalized more than once
    int id; //Function local variable that holds each thread's id. It is not created as a static variable so that each thread has its own seperate copy of their unique IDs on their stack

    sem_wait(&initalize_lock); //Locks the initialize_lock semaphore to check the initalized variable- sees if the semaphores have been intialized yet
    if (initalized == false){  //If initalized is false this is the first thread running the phonecall function- initalize the semaphores needed for the function
        initalized = true; //Set initalized to true so prior threads do not initialize the semaphores again
        sem_init(&operators, 0, NUM_OPERATORS); //Initialize the operators semaphore- 3 (NUM_OPERATORS) threads allowed to enter critical section of code
        sem_init(&connected_lock, 0, 1); //Initalize the connected_lock semaphore- 1 thread allowed to enter critical section of code
        sem_init(&id_count, 0, 1); //Initalize the id_count semaphore- 1 thread allowed to enter critical section of code
    }
    sem_post(&initalize_lock); //Unlocks the initalize_lock semaphore so the next waiting thread function can enter this critical section of code and see if initalized is true

    sem_wait(&id_count); //Locks the id_count semaphore so only one thread at a time can increment the next_id global variable and set its local id variable to the incremented id
    id = ++next_id; //Increments next_id first and sets this value to the threads local id variable (only accessible in memory to itself)
    sem_post(&id_count); //Unlocks the id_count semaphore so the next waiting thread function can enter this critical section and recieve an id from the incrementation of next_id

    printf("Caller [%i] attempting to connect. \n", id);

    //Keeps looping checking if the thread/caller can connect to the limited 5 lines locking and unlocking the connected_lock semaphore until it can finally connect and breaks out of loop
    while(1){
        sem_wait(&connected_lock); //Locks the connected lock semaphore to allow one thread at a time to check the connected variable to see if it can connect or if there are already 5 connected 
        if (connected != NUM_LINES){ //Checks if the number of callers connected does not equal the total number of lines available, if it doesn't the caller has connected
            connected++; //Increases the connected variable to account for the current thread connecting
            sem_post(&connected_lock); //Unlocks the connected_lock semaphore to allow the next waiting thread/caller to check if there are any available lines to connect to
            break; //Breaks out of the loop to move on to the next step since it has connected
        }
        else{ //If connected does equal the total number of lines available, the caller must call again until one of the lines becomes not busy
            sem_post(&connected_lock); //Unlocks the connected_lock semaphore to allow the next waiting caller to check if it can connect while this one recalls
            printf("Caller [%i] calling again, busy signal. \n", id); 
            sleep(3); //Sleeps for 3 seconds to slow down how many times the thread will loop through and check if it can connect to allow time for connected callers to buy tickets and unconnect
        }
    }
    printf("Caller [%i] has an available line, waiting for operator. \n", id);

    sem_wait(&operators); //Locks the counting semaphore operators, which only allows 3 callers/threads access at a time, the rest of the threads block until one of the threads in the critical section exits and unlocks the semaphore and allows another waiting one access
    printf("Caller [%i] is talking to an operator. \n", id);
    sleep(3); //Sleeps for 3 seconds to simulate the ordering of a ticket
    printf("Caller [%i] has bought a ticket! \n", id);
    
    sem_wait(&connected_lock); //Locks the connected_lock semaphore allowing one thread access to decrease connected variable
    connected--; //Connected decreases since the caller has bought a ticket and now is no longer connected 
    sem_post(&connected_lock); //Unlocks the connected_lock semaphore to allow other waiting threads access to this variable 

    sem_wait(&callers_lock); //Locks the callers_lock variable semaphore allowing one thread at a time access to the callersLeft pointer to decrease the integer it points to
    (*callersLeft)--; //Decreases the integer pointed to by the callersLeft int pointer, since this caller has completed buying a ticket
    sem_post(&callers_lock); //Unclocks the callers_lock variable semaphore allowing a next waiting thread access to the callersLeft pointer

    sem_post(&operators); //Unlocks the operators semaphore, allowing another waiting thread access to the critical section for ticket buying 

    printf("Caller [%i] has hung up. \n", id);

    sem_wait(&callers_lock); //Unlocks the callers_lock semaphore allowing one thread access to the callersLeft pointer to check if it's value is zero indicating it is the last thread/caller
    //Need to check if callersLeft equals zero to see if it is the last thread running, since the thread with the last id is not necessarily the going to be the last thread running due to the CPU schedular 
    if (*callersLeft == 0){ //If there are 0 callers left, it is the last thread/caller and therefore all the semaphores initialized in this function must be destroyed 
        sem_destroy(&operators); //Destoys the operators semaphore
        sem_destroy(&connected_lock); //Destroys the connected_lock semaphore
        sem_destroy(&id_count); //Destroys the id_count semaphore 
    }
    sem_post(&callers_lock); //Unlocks the callers_lock semaphore to give any remaining threads access to the callersLeft pointer and to check if it must destroy the semaphores in this function

    return 0;
}

/*
*Main function that gets the total number of total callers from the command line or if nothing is passed in uses 240 as the default. 
*Initializes the initalize_lock and callers_lock global semaphores- used to make sure semaphores in the thread function are initialized only once and are destroyed only by the last thread
*Creates a thread for each of the total callers, and then joins with each thread to wait for each of them to complete 
*Parameters:
	argc = the total number of arguments passed into main with one being the name of the program
	argv = array containing all the arguments written in the command line- argv[0] always stores the name of the program
*Returns:
	*Returns 0 if the function ran and completed successfully 
*/
int main(int argc, char *argv[]){
    int totalCalls; //Variable to hold the total number of calls either passed in through the command line or 240 as the default if nothing is passed into the command line

    //If argc is greater than one, this means an additional argument has been passed in through the command line
    if (argc > 1){ 
        //If a valid number is not passed in through the command line, atoi will return 0 so the total number of callers will be zero causing nothing to happen in the program
        totalCalls = atoi(argv[1]); //Converts the string number passed in through the command line of the total callers (held in argv[1]) to an integer
        if (totalCalls > 240){
            printf("Too many callers entered, 240 is the maximum seating capacity for the show. \n");
            totalCalls = 240;
        }
    }
    else{ //If argc is not greater than one, this means only the name of the function is referenced by argc so no additional arguments were passed in
        totalCalls = 240; //Uses the default 240 callers as the total number of callers when another number is not passed in through the command line
    }

    /*Creates a dynamically created integer pointer using malloc to point initally to the total number of callers
    *On the heap so the threads all have access to this pointer and can all access and manipulate it to decrement it every time a caller completes buying a ticket*/
    int *callersLeft = (int *)malloc(sizeof(int));
    *callersLeft = totalCalls; //Sets the integer pointed to by callersLeft to the total number of callers 
    
    pthread_t tids[totalCalls]; //Creates an array of type pthread_t of size equal to the total number of callers to hold each of the thread ids

    sem_init(&initalize_lock, 0, 1); //Initializes the initialize lock binary semaphore
    sem_init(&callers_lock, 0, 1); //Initializes the callers_lock binary semaphore

    //Loops through from 0 to the total number of callers and creates a thread, storing each thread id in the tids array 
    for (int i = 0; i<totalCalls; i++){
        pthread_create(&tids[i], NULL, phonecall, (void *)callersLeft); //Creates a new thread that will begin execution in the phonecall function and casts the callersLeft int pointer to a void pointer to pass into the phonecall function
    }

    //Loops through from 0 to the total number of callers to use pthread_join on each thread to block the main thread to wait for each thread to complete 
    for(int i = 0; i<totalCalls; i++){
        pthread_join(tids[i], NULL); //Waits for the thread specified by tids[i] to terminate 
    }

    sem_destroy(&initalize_lock); //Destroys the intialize_lock semaphore
    sem_destroy(&callers_lock); //Destroys the callers_lock semaphore
    free(callersLeft); //Frees the dynamically created callersLeft integer pointer

    return 0;
}