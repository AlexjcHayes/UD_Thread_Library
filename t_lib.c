#include "t_lib.h"
#include <signal.h>
#include <string.h>
tcb *running;  // head of running queue
tcb *ready;    // head of ready queue
//int mbox_create(mbox **mb);

tcb * endQ(tcb *iter){
  while(iter->next!=NULL){
    iter=iter->next;
  }
  return iter;
}


void appendTCB(tcb *queue, tcb * tcb){
  if(queue==NULL){
    queue=tcb;
    queue->next=NULL;
  }else{
    queue=endQ(queue);
    queue->next=tcb;
    tcb->next=NULL;
  }
}

tcb *removeQueueHead(tcb *queue, int tcb_ID){
  if(queue==NULL){
    return NULL;
  }else if((tcb_ID < 0) || (queue->thread_id==tcb_ID)){
    tcb *tmp=queue;
    queue=queue->next;
    return tmp;
  }
}

struct messageNode *removeMboxHead(struct messageNode *mbox){
  if(mbox==NULL){
    return NULL;
  }else{
    struct messageNode *tmp=mbox;
    //struct messageNode tmp=*(mbox->message);
    mbox=mbox->next;
    while(mbox->next !=NULL){
      mbox=mbox->next;
    }
    return tmp;
  }
}

mbox *findThread(int tid){ // finds the thread that matches the tid
// check to see if it's in the running queue
if(running->thread_id==tid){
  return (mbox *)(running->mailbox);
}else{ // then check to see if it's in the ready queue
  if(ready != NULL){
    tcb* tmp=ready;
    while(tmp !=NULL){
      if(tmp->thread_id == tid){ // check to see if it matches the id
      return (mbox *)(tmp->mailbox); // return the mailbox of the thread
      }
        tmp=tmp->next;
    }
  }
}

return NULL; // its doesn't exist in either
}

void t_yield()
{
  tcb* tmp;
  tmp = running;         // store the currently running thread in tmp
  tmp->next = NULL;      // just to make sure
  
  if (ready != NULL) {   // only yield if there is a TCB in ready queue
    running = ready;     // first TCB in ready queue becomes running 
    ready = ready->next; // ready to next thread
    running->next = NULL;
    tcb* iter;
    iter = ready;
    if (iter == NULL)    // if there is nothing else in ready queue
      ready = tmp;
    else { 
      while (iter->next != NULL) // loop till end of queue
        iter = iter->next;
      iter->next = tmp;  // add tmp to end of queue
    }
	                 // context switch
    swapcontext(tmp->thread_context, running->thread_context);
  }
  else {
    // printf("??? there is no other thread in the application ???\n");
    // the calling thread continues...
  }
}


void t_init()
{
  tcb *tmp = (tcb *) malloc(sizeof(tcb));  // malloc TCB
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));
  getcontext(tmp->thread_context);         // get context for "main"

  tmp->next = NULL;
  tmp->thread_id = 0;    // main thread has ID 0 
  running = tmp;
  ready = NULL; 
}


void t_create(void (*fct)(int), int id, int pri)
{ 
  size_t sz = 0x10000;                      // stack size
  tcb* tmp = (tcb*) malloc(sizeof(tcb));    // malloc TCB
  tmp->thread_context = (ucontext_t *) malloc(sizeof(ucontext_t));

  getcontext(tmp->thread_context);          // get context
  tmp->thread_context->uc_stack.ss_sp = malloc(sz);  // malloc stack 
  tmp->thread_context->uc_stack.ss_size = sz;        // set stack size
  tmp->thread_context->uc_stack.ss_flags = 0;
  // tmp->thread_context->uc_link = running->thread_context;
  makecontext(tmp->thread_context, (void (*)(void)) fct, 1, id);
  tmp->thread_id = id;                      // set up thread ID
  tmp->thread_priority = pri;               // set up priority
  tmp->next = NULL;

  if (ready == NULL)                        // insert into the END of
    ready = tmp;                            // ready queue
  else {
    tcb* t = ready;
    while(t->next != NULL) {                // find the end
      t = t->next;
    }
    t->next = tmp;                          // insert to the end
  }

  mbox_create(&tmp->mailbox);
}


void t_terminate()
{
  tcb* tmp;
  tmp = running;
  running = ready;        // 1st ready TCB becomes running
  ready = ready->next;    // every TCB in ready moves forward

  free(tmp->thread_context->uc_stack.ss_sp);
  free(tmp->thread_context);
  free(tmp);

  setcontext(running->thread_context);  // resume running TCB
}

// free all the TCBs to avoid memory leaks

void t_shutdown()
{
  if (ready != NULL) {
    tcb* tmp;
    tmp = ready;
    while (tmp != NULL) {
      ready = ready->next;
      free(tmp->thread_context->uc_stack.ss_sp);
      free(tmp->thread_context);
      free(tmp);
      tmp = ready;
    }
  }

  free(running->thread_context);
  free(running);	
}

int sem_init(sem_t **sp, int sem_count)
{
  *sp = malloc(sizeof(sem_t));
  (*sp)->count = sem_count;
  (*sp)->q = NULL;
}

void sem_wait(sem_t *sem)
{
  tcb * temp_current=sem->q;
  sem->count-=1; // decrement the counter
  if(sem->count<0){
    // pause thread in here
    if(sem->q ==NULL){ // nothing has been assigned to this semaphore
    sem->q = running; // give it the tcb in the running queue
    }else{
      temp_current=endQ(sem->q);
      temp_current->next = running;
    }
    tcb * temp=running;
    running = ready; // running is head of ready
    ready = ready->next; // ready head is shifted to the next thread
    running->next = NULL; // running next is null
    swapcontext(temp->thread_context,running->thread_context);
  }
}



void sem_signal(sem_t *sem)
{

  tcb * temp= sem->q;
  tcb * temp2= sem->q;
  sem->count++;
  if(sem->count <=0 ){
      if(temp2->next!=NULL){
      temp2=temp2->next;
      appendTCB(ready,temp);
    }
  }
}

void sem_destroy(sem_t **sem)
{
  tcb *iter = (*sem)->q;
  while(iter!=NULL){
    tcb *destroy = iter;
    iter = iter->next;
    appendTCB(ready, destroy);
  }
  free(*sem);
}


void mbox_destroy(mbox **mb) {   //-------------------------------
    free((*mb)->msg);
    //sem_destroy((*mb)->mbox_sem);
    free((*mb)->mbox_sem);
    free(*mb);

}

int mbox_create(mbox **mb){    //---------------------------------
  *mb= malloc(sizeof(mbox));
  sem_init(&((*mb)->mbox_sem),1);
}

void mbox_deposit(mbox *mb, char *msg, int len) {//------------------------
  //insert a message into our mailbox queue
  struct messageNode* msg_n = malloc(sizeof(struct messageNode));
  msg_n->len = len;
  msg_n->message = msg;
  struct messageNode* messageHead = mb->msg;
  if (messageHead == NULL) { // check to see if its empty
     mb->msg = msg_n;
     //printf("MB messge; %s\n",mb->msg->message);
     messageHead=msg_n;
  }
  else {

    // struct messageNode* iter = mb->msg;
    while(messageHead->next != NULL) {
      messageHead = messageHead->next;
    }

    messageHead->next = msg_n;
    //printf("MB messge; %s\n",mb->msg->message);
    msg_n->next=NULL;
  }

}



void send(int tid, char *msg, int len) {// ---------------------
    //mbox *destination = fetchMailbox(tid);
    
    mbox *destination = findThread(tid);
    //printf("Send function tid: %d\n",tid);
    mbox_deposit(destination, msg, len);
}

void mbox_withdraw(mbox *mb, char *msg, int *len){//----------------------------------------
    //withdrawl the first item from the mailbox and put it in msg
  if (mb->msg == NULL) { // set the len to 0 if nothing in in mailbox
    *len = 0;
  }
  struct messageNode* mailHead = removeMboxHead(mb->msg);

  if(mailHead != NULL){
    strncpy(msg, mailHead->message,mailHead->len +1);
    *len = mailHead->len + 1;
    
  }
}


void mbox_tid_withdraw(mbox *mb, int *tid, char *msg, int *len) { //----------------------
    //withdrawl the first item from the mailbox and put it in msg
  if (mb->msg == NULL) { // set the len to 0 if nothing in in mailbox
    *len = 0;
  }
  struct messageNode* mailHead = removeMboxHead(mb->msg);

    while(mailHead != NULL){
        //printf("given tid: %ls\n", tid);
        //printf("Sender: %ls\n",mailHead->sender);
      if(mailHead->sender == *tid){
        strncpy(msg, mailHead->message,mailHead->len +1);
        *len = mailHead->len + 1;}
      else{ 
      mailHead = mailHead->next;
    }
  }
}


void receive(int *tid, char *msg, int *len){
    mbox *recipient = (mbox *)(running->mailbox);
    if (*tid == 0) {
        mbox_withdraw(recipient, msg, len);
    } else {
        //printf("TID Sent: ");
        //printf("%d\n",*tid);
        mbox_tid_withdraw(recipient, tid, msg, len);
    }
}


