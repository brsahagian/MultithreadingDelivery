#include <stdio.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <stdbool.h>
#define MAX 40

char choices[4] = {'w','b','m','j'};
int count = 0;
int last = 0;
int totalPackages = 20;
pthread_cond_t red, blue, yellow, green;
pthread_cond_t w,b,m,j,queueLock,queueLockB,queueLockR,queueLockY;
pthread_mutex_t mutex,worker,mutexB,mutexR,mutexY,workerB,workerY,workerR;
pthread_mutex_t shared, shared2;

int countB = 0, countG = 0, countR = 0, countY = 0;
int queueCount,queueCountB,queueCountY,queueCountR;
int dq,dqB,dqR,dqY;

//Package struct contains id and array of choices
struct Package{
   int package_id;
   char package_inst[];
};


//Worker struct contains number and color
struct Worker{
 int number;
 char color;
 bool inQueue;
};



bool mStation = false,jStation = false ,bStation = false,wStation = false;

bool hasChoice(char array[], char choice, int size){
    for(int i = 0; i < size; i++){
       if(choice == array[i]){
          return true;
       }
    }
    return false;
 }

//Creates a randomized instruction set for a package.
struct Package *createPackage (struct Package *p, int id){
   sleep(1);
   srand(time(NULL));
   int randSize = rand()%4+1;
   
   p = malloc(sizeof(*p) + sizeof(char) * randSize);
   p->package_id = id;
   
   if(p->package_id != 0) {
   for(int i = 0; i < randSize; i++){
       char choice = choices[rand()%4];
       bool hasDuplicate = hasChoice(p->package_inst, choice, randSize);
       while(hasDuplicate){
          choice = choices[rand()%4];
          hasDuplicate = hasChoice(p->package_inst, choice, randSize);
       }
       p->package_inst[i] = choice;
   } 
   
   printf("I am package #%d\n", p->package_id);
   printf("My instructions are: { ");
   for(int i = 0; i < randSize; i++){
       printf("%c ", p->package_inst[i]);
   }

   printf("}\n");

   }
   return p;
 }
 
 struct node
{
    int data;
    struct node *next;
};
typedef struct node node;

struct queue
{
    int count; // worker's id 
    node *front;
    node *rear;
};

//Queue implementation and functions
typedef struct queue queue;

void initialize(queue *q)
{
    q->count = 0;
    q->front = NULL;
    q->rear = NULL;
}
int isempty(queue *q)
{
    return (q->rear == NULL);
}
void enqueue(queue *q, int value)
{
    node *tmp;
    tmp = malloc(sizeof(node));
    tmp->data = value;
    tmp->next = NULL;
    if(!isempty(q))
    {
        q->rear->next = tmp;
        q->rear = tmp;
    }
    else
    {
        q->front = q->rear = tmp;
    }
    q->count++;
}
int dequeue(queue *q)
{
    node *tmp;
    int n = q->front->data;
    tmp = q->front;
    q->front = q->front->next;
    q->count--;
    free(tmp);
    return(n);
}

 
struct queue redTeam, blueTeam, yellowTeam, greenTeam;
size_t packageSizeG, packageSizeB,packageSizeR,packageSizeY;
struct Package *thePackage, *thePackageR,*thePackageB,*thePackageY;

//Work function specific to every team color. Will only comment one because they function identically
void *workFunctionGreen(void *arg){
   struct Worker *args = (struct Worker*) arg; 
   pthread_mutex_lock(&mutex);
   
   printf("[Worker #%d, %c] is here for duty! \n", args->number,args->color);
   
   //enqueue workers first
   enqueue(&greenTeam, args->number);
   queueCount++;
   args->inQueue = true;
   pthread_mutex_unlock(&mutex);
  
   pthread_mutex_lock(&worker);
   if(queueCount != 10 ){
   pthread_cond_wait(&queueLock,&worker);}
   
   //dequeues entries only after the queue is completely full
   if(queueCount == 10){
      dq = dequeue(&greenTeam);
      queueCount--;
   }
   pthread_mutex_unlock(&worker);
   pthread_cond_broadcast(&queueLock);
   
   while(count < MAX+1){
 
 //Works if dequeued
      pthread_mutex_lock(&worker); //worker is locked
      while(args->color == 'g' && (dq != args->number)){                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
         pthread_cond_wait(&green,&worker);  //wait and unlock worker

      } 

      if(dq == args->number && count != MAX){
         printf("[Worker #%d, %c] : I'm taking my package\n", args->number, args->color);
 
         pthread_mutex_lock(&shared);
         count++;
         thePackage = createPackage(thePackage, count);
         countG++;
         //args->p = thePackage;
         pthread_mutex_unlock(&shared);
         packageSizeG = strlen(thePackage->package_inst);
         printf("[Worker #%d, %c]: I am holding package #%d and will do its instruction(s) \n",args->number,args->color, thePackage->package_id);

         for(int i =0; i< packageSizeG ; i++){
		
		//Goes to correct station or blocks
		if(thePackage->package_inst[i] == 'm' && mStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Measuring station \n", args->number,args->color);
		pthread_cond_wait(&m,&shared);
		}
		if(thePackage->package_inst[i] == 'm' && !mStation){
		pthread_mutex_lock(&shared2);
		mStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Measuring station\n", args->number, args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Measuring station \n", args->number, args->color);
		pthread_mutex_lock(&shared2);
		mStation = false;
		pthread_mutex_unlock(&shared2);
		pthread_cond_signal(&m);}
		
		if(thePackage->package_inst[i] == 'j' && jStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Jostling station \n", args->number, args->color);
		pthread_cond_wait(&j,&shared);
		}

		if(thePackage->package_inst[i] == 'j' && !jStation){
		pthread_mutex_lock(&shared2);
		jStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Jostling station \n", args->number, args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Jostling station \n", args->number, args->color);
		pthread_mutex_lock(&shared2);
		jStation = false;
		pthread_mutex_unlock(&shared2);
		pthread_cond_signal(&j);}
		
		if(thePackage->package_inst[i] == 'w' && wStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Weighing station \n", args->number, args->color);
		pthread_cond_wait(&w,&shared);
		}

		if(thePackage->package_inst[i] == 'w' && !wStation){
		pthread_mutex_lock(&shared2);
		wStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Weighing station \n", args->number, args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Weighing station \n", args->number, args->color);
		pthread_mutex_lock(&shared2);
		wStation = false;
		pthread_mutex_unlock(&shared2);
		pthread_cond_signal(&w);}
		
		if(thePackage->package_inst[i] == 'b' && bStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to Barcoding station \n", args->number, args->color);
		pthread_cond_wait(&b,&shared);
		}

		if(thePackage->package_inst[i] == 'b'){
		pthread_mutex_lock(&shared2);
		bStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Barcoding station \n", args->number, args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Barcoding station \n", args->number, args->color);
		pthread_mutex_lock(&shared2);
		bStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&b);}
		
		pthread_mutex_unlock(&shared);
         }
         
	 printf("[Worker #%d, %c]: The Package is done. Move on to the next worker \n", args->number, args->color);

         dq = dequeue(&greenTeam);    
         if(count == MAX){
         pthread_mutex_lock(&shared);
         last++;
         pthread_mutex_unlock(&shared);
         if(last == 3){
         //Keeps track of total number of packages logged by each team
	 printf("Team blue has done %d package(s) \n", countB);
	 printf("Team green has done %d package(s) \n", countG);
	 printf("Team yellow has done %d package(s) \n", countY);
	 printf("Team red has done %d package(s) \n", countR);
         exit(0);}
         
         break;
         }
         
         //Enqueues back onto the queue
         enqueue(&greenTeam, args->number);
         pthread_mutex_unlock(&worker);
         pthread_cond_broadcast(&green);
      }
}

   return NULL;
   }
void *workFunctionBlue(void *arg){
   struct Worker *args = (struct Worker*) arg; 
   pthread_mutex_lock(&mutexB);
   printf("[Worker #%d, %c] is here for duty! \n", args->number,args->color);
   enqueue(&blueTeam, args->number);
   queueCountB++;
   args->inQueue = true;
   pthread_mutex_unlock(&mutexB);
   pthread_mutex_lock(&workerB);
   if(queueCountB != 10 ){
   pthread_cond_wait(&queueLockB,&workerB);}
   
   if(queueCountB == 10){
      dqB = dequeue(&blueTeam);
      queueCountB--;
   }
   pthread_mutex_unlock(&workerB);
   pthread_cond_broadcast(&queueLockB);
   
   while(count < MAX +1){
 
      pthread_mutex_lock(&workerB); //worker is locked
      while(args->color == 'b' && (dqB != args->number)){
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                         
         pthread_cond_wait(&blue,&workerB);  //wait and unlock worker
      } 

      if(dqB == args->number && count != MAX){
          printf("[Worker #%d, %c] : I'm taking my package\n", args->number, args->color);
         pthread_mutex_lock(&shared);
         count++;
         thePackageB = createPackage(thePackageB, count);
         countB++;
         pthread_mutex_unlock(&shared);
         
         packageSizeB = strlen(thePackageB->package_inst);
         printf("[Worker #%d, %c]: I am holding package #%d and will do its instruction(s) \n",args->number,args->color, thePackageB->package_id);

         
          for(int i =0; i< packageSizeB ; i++){
         
		if(thePackageB->package_inst[i] == 'm' && mStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Measuring station \n", args->number,args->color);
		pthread_cond_wait(&m,&shared);
		}

		if(thePackageB->package_inst[i] == 'm' && !mStation){
		pthread_mutex_lock(&shared2);
		mStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Measuring station \n", args->number,args->color);		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Measuring station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		mStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&m);}
		
		if(thePackageB->package_inst[i] == 'j' && jStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Jostling station \n", args->number,args->color);
		pthread_cond_wait(&j,&shared);
		}

		if(thePackageB->package_inst[i] == 'j'&& !jStation){
		pthread_mutex_lock(&shared2);
		jStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Jostling station \n", args->number,args->color);		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Jostling station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		jStation = false;
		pthread_mutex_unlock(&shared2);
		pthread_cond_signal(&j);}
		
		if(thePackageB->package_inst[i] == 'w' && wStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Weighing station \n", args->number,args->color);
		pthread_cond_wait(&w,&shared);
		}

		if(thePackageB->package_inst[i] == 'w'&& !wStation){
		pthread_mutex_lock(&shared2);
		wStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Weighing station \n", args->number,args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Weighing station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		wStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&w);}
		
		if(thePackageB->package_inst[i] == 'b' && bStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Barcoding station \n", args->number,args->color);
		pthread_cond_wait(&b,&shared);
		}

		if(thePackageB->package_inst[i] == 'b' && !bStation){
		pthread_mutex_lock(&shared2);
		bStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Barcoding station \n", args->number,args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Barcoding station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		bStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&b);}
		
		pthread_mutex_unlock(&shared);
         
         }
         
	 printf("[Worker #%d, %c]: The Package is done. Move on to the next worker \n", args->number, args->color);
         
         dqB = dequeue(&blueTeam);    

         if(count == MAX){
         pthread_mutex_lock(&shared);
         last++;
         pthread_mutex_unlock(&shared);
         if(last == 3){
	 printf("Team blue has done %d package(s) \n", countB);
	 printf("Team green has done %d package(s) \n", countG);
	 printf("Team yellow has done %d package(s) \n", countY);
	 printf("Team red has done %d package(s) \n", countR);
         break;}
         
         break;
         }
         
         enqueue(&blueTeam, args->number);
         pthread_mutex_unlock(&workerB);
         pthread_cond_broadcast(&blue);
      }
}

   return NULL;
   }
   
void *workFunctionRed(void *arg){
   struct Worker *args = (struct Worker*) arg; 
   pthread_mutex_lock(&mutexR);
   printf("[Worker #%d, %c] is here for duty! \n", args->number,args->color);
   enqueue(&redTeam, args->number);
   queueCountR++;
   args->inQueue = true;
   pthread_mutex_unlock(&mutexR);
   pthread_mutex_lock(&workerR);
   if(queueCountR != 10 ){
   pthread_cond_wait(&queueLockR,&workerR);}
   
   if(queueCountR == 10){
      dqR = dequeue(&redTeam);
      queueCountR--;
   }
   pthread_mutex_unlock(&workerR);
   pthread_cond_broadcast(&queueLockR);
   
   while(count < MAX +1){
 
      pthread_mutex_lock(&workerR); //worker is locked
      while(args->color == 'r' && (dqR != args->number)){                                                                                                                                                                                                                                                                                                                                                                                                                                                                           
         pthread_cond_wait(&red,&workerR);  //wait and unlock worker
      } 

      if(dqR == args->number && count != MAX ){
          printf("[Worker #%d, %c] : I'm taking my package\n", args->number, args->color);
         pthread_mutex_lock(&shared);
         count++;
         thePackageR = createPackage(thePackageR, count);
         countR++;
         pthread_mutex_unlock(&shared);
         
         packageSizeR = strlen(thePackageR->package_inst);
         printf("[Worker #%d, %c]: I am holding package #%d and will do its instruction(s) \n",args->number,args->color, thePackageR->package_id);
         
          for(int i =0; i< packageSizeR ; i++){
         
		
		if(thePackageR->package_inst[i] == 'm' && mStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Measuring station \n", args->number,args->color);
		pthread_cond_wait(&m,&shared);
		}

		if(thePackageR->package_inst[i] == 'm' && !mStation){
		pthread_mutex_lock(&shared2);
		mStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Measuring station \n", args->number,args->color);		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Measuring station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		mStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&m);}
		
		if(thePackageR->package_inst[i] == 'j' && jStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Jostling station \n", args->number,args->color);
		pthread_cond_wait(&j,&shared);
		}

		if(thePackageR->package_inst[i] == 'j'&& !jStation){
		pthread_mutex_lock(&shared2);
		jStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Jostling station \n", args->number,args->color);		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Jostling station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		jStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&j);}
		
		if(thePackageR->package_inst[i] == 'w' && wStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Weighing station \n", args->number,args->color);
		pthread_cond_wait(&w,&shared);
		}

		if(thePackageR->package_inst[i] == 'w'&& !wStation){
		pthread_mutex_lock(&shared2);
		wStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Weighing station \n", args->number,args->color);
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Weighing station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		wStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&w);}
		
		if(thePackageR->package_inst[i] == 'b' && bStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Barcoding station \n", args->number,args->color);
		pthread_cond_wait(&b,&shared);
		}

		if(thePackageR->package_inst[i] == 'b' && !bStation){
		pthread_mutex_lock(&shared2);
		bStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Barcoding station \n", args->number,args->color);
		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Barcoding station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		bStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&b);}
		
		pthread_mutex_unlock(&shared);
         
         }
         
	 printf("[Worker #%d, %c]: The Package is done. Move on to the next worker \n", args->number, args->color);
         
         dqR = dequeue(&redTeam);    
         if(count == MAX){
         pthread_mutex_lock(&shared);
         last++;
         pthread_mutex_unlock(&shared);
         if(last == 3){
	 printf("Team blue has done %d package(s) \n", countB);
	 printf("Team green has done %d package(s) \n", countG);
	 printf("Team yellow has done %d package(s) \n", countY);
	 printf("Team red has done %d package(s) \n", countR);
         exit(0);}
         
         break;
         }
         enqueue(&redTeam, args->number);
         pthread_mutex_unlock(&workerR);
         pthread_cond_broadcast(&red);
      }
}

   return NULL;
   }
   
   void *workFunctionYellow(void *arg){
   struct Worker *args = (struct Worker*) arg; 
   pthread_mutex_lock(&mutexY);
   printf("[Worker #%d, %c] is here for duty! \n", args->number,args->color);
   enqueue(&yellowTeam, args->number);
   queueCountY++;
   args->inQueue = true;
   pthread_mutex_unlock(&mutexY);
   pthread_mutex_lock(&workerY);
   if(queueCountY != 10 ){
   pthread_cond_wait(&queueLockY,&workerY);}
   
   if(queueCountY == 10){
      dqY = dequeue(&yellowTeam);
      queueCountY--;
   }
   pthread_mutex_unlock(&workerY);
   pthread_cond_broadcast(&queueLockY);
   
   while(count < MAX + 1){
 
      pthread_mutex_lock(&workerY); //worker is locked
      while(args->color == 'y' && (dqY != args->number)){
                                                                                                                                                                                                                                                                                                                                                                                                                                                                        
         pthread_cond_wait(&yellow,&workerY);  //wait and unlock worker

      } 

      if(dqY == args->number && count != MAX){
         printf("[Worker #%d, %c] : I'm taking my package\n", args->number, args->color);
         pthread_mutex_lock(&shared);
         count++;
         thePackageY = createPackage(thePackageY, count);
         countY++;
         pthread_mutex_unlock(&shared);
         
         packageSizeY = strlen(thePackageY->package_inst);
         printf("[Worker #%d, %c]: I am holding package #%d and will do its instruction(s) \n",args->number,args->color, thePackageY->package_id);

         
          for(int i =0; i< packageSizeY; i++){
         
		if(thePackageY->package_inst[i] == 'm' && mStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Measuring station \n", args->number,args->color);
		pthread_cond_wait(&m,&shared);
		}

		if(thePackageY->package_inst[i] == 'm' && !mStation){
		pthread_mutex_lock(&shared2);
		mStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Measuring station \n", args->number,args->color);		
		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Measuring station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		mStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&m);}
		
		if(thePackageY->package_inst[i] == 'j' && jStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Jostling station \n", args->number,args->color);
		pthread_cond_wait(&j,&shared);
		}

		if(thePackageY->package_inst[i] == 'j'&& !jStation){
		pthread_mutex_lock(&shared2);
		jStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Jostling station \n", args->number,args->color);		
		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Jostling station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		jStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&j);}
		
		if(thePackageY->package_inst[i] == 'w' && wStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Weighing station \n", args->number,args->color);
		pthread_cond_wait(&w,&shared);
		}

		if(thePackageY->package_inst[i] == 'w'&& !wStation){
		pthread_mutex_lock(&shared2);
		wStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Weighing station \n", args->number,args->color);
		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Weighing station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		wStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&w);}
		
		if(thePackageY->package_inst[i] == 'b' && bStation){
		pthread_mutex_lock(&shared);
		printf("[Worker #%d, %c]: Waiting to get to the Barcoding station \n", args->number,args->color);
		pthread_cond_wait(&b,&shared);
		}

		if(thePackageY->package_inst[i] == 'b' && !bStation){
		pthread_mutex_lock(&shared2);
		bStation = true;
		pthread_mutex_unlock(&shared2);
		printf("[Worker #%d, %c]: Entering the Barcoding station \n", args->number,args->color);
		
		sleep(rand() % 4 + 2);
		printf("[Worker #%d, %c]: Leaving the Barcoding station \n", args->number,args->color);
		pthread_mutex_lock(&shared2);
		bStation = false;
		pthread_mutex_unlock(&shared2);
		
		pthread_cond_signal(&b);}
		
		pthread_mutex_unlock(&shared);
         
         }
         
	 printf("[Worker #%d, %c]: The Package is done. Move on to the next worker \n", args->number, args->color);

         dqY = dequeue(&yellowTeam);    
         if(count == MAX){
         pthread_mutex_lock(&shared);
         last++;
         pthread_mutex_unlock(&shared);
         if(last == 3){
	 printf("Team blue has done %d package(s) \n", countB);
	 printf("Team green has done %d package(s) \n", countG);
	 printf("Team yellow has done %d package(s) \n", countY);
	 printf("Team red has done %d package(s) \n", countR);
         exit(0);}
         
         break;
         }
         
         enqueue(&yellowTeam, args->number);
         pthread_mutex_unlock(&workerY);
         pthread_cond_broadcast(&yellow);
      }
}

   return NULL;
   }
   
int main(){

pthread_t r_threads[10], y_threads[10], b_threads[10], g_threads[10];

struct Worker rWorker[10], yWorker[10], bWorker[10], gWorker[10];

//Create red members
for(int i = 0; i < 10; i++){
rWorker[i].number = i;
rWorker[i].color = 'r';
rWorker[i].inQueue == false;
pthread_create(&r_threads[i], NULL, workFunctionRed, (void*)&rWorker[i]);

}

//Create yellow members
for(int i = 0; i < 10; i++){
yWorker[i].number = i;
yWorker[i].color = 'y';
yWorker[i].inQueue == false;
pthread_create(&y_threads[i], NULL, workFunctionYellow, (void*)&yWorker[i]);
}


//Create green members
for(int i = 0; i < 10; i++){
gWorker[i].number = i;
gWorker[i].color = 'g';
gWorker[i].inQueue == false;
pthread_create(&g_threads[i], NULL, workFunctionGreen, (void*)&gWorker[i]);

}

//Create blue members
for(int i = 0; i < 10; i++){
bWorker[i].number = i;
bWorker[i].color = 'b';
bWorker[i].inQueue == false;
pthread_create(&b_threads[i], NULL, workFunctionBlue, (void*)&bWorker[i]);
}



for(int i = 0; i < 10; i++){
         pthread_join(r_threads[i], NULL);}   
for(int i = 0; i < 10; i++){
         pthread_join(y_threads[i], NULL);}
for(int i = 0; i < 10; i++){
         pthread_join(b_threads[i], NULL);}        
for(int i = 0; i < 10; i++){
         pthread_join(g_threads[i], NULL);}

   pthread_exit(NULL); 
   exit(0);

} 
