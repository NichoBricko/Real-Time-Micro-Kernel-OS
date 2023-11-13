#include <stdio.h>
#include <stdlib.h>
#include "kernel_functions.h"



exception wait(uint nTicks){
  
  isr_off();
  ReadyList->pHead->nTCnt = nTicks; //insert given Ticks to pHead nTCnt
  PreviousTask = ReadyList->pHead->pTask;
  
  insert_task(TimerList, extract_head(ReadyList)); //insert Head to TimerList
  NextTask = ReadyList->pHead->pTask; //Sort readylist
  
  SwitchContext();
  if(ReadyList->pHead->pTask->Deadline <= Ticks)
    return DEADLINE_REACHED;
  else
    return OK;

}

void set_ticks(uint nTicks) { 
  Ticks = nTicks;
}

uint ticks(void){
  return Ticks;
}

uint deadline(void){
  return ReadyList->pHead->pTask->Deadline;
}

void set_deadline(uint deadline){
  isr_off();
  
  ReadyList->pHead->pTask->Deadline = deadline; //set deadline for readylist
  PreviousTask = ReadyList->pHead->pTask;
  insert_task(ReadyList, extract_head(ReadyList)); //resort readylist head
  NextTask = ReadyList->pHead->pTask; 
  SwitchContext();
}

void TimerInt(void){ 
  PreviousTask = ReadyList->pHead->pTask;
  set_ticks(ticks()+1); 
  int tempTicks = ticks();
  listobj * temp = TimerList->pHead;

  while(temp != NULL){ //as long as deadline hasn't expired
    if (temp->nTCnt <= ticks() || temp->pTask->Deadline <= ticks()){
      listobj * copy = temp;
      temp = temp->pNext; 
      insert_task(ReadyList, removeNode(TimerList, copy)); 
    }           //remove and insert timerlist head node
    else
      temp = temp->pNext; //move pointer to next step 
  }
  temp = WaitingList->pHead; //move temp pointer to waitinglist
  int counter = 0;
  
  while(temp->pTask->Deadline <= ticks()){ //sort waitinglist tasks for...
     counter++;                            //...expired deadline        
     temp = temp->pNext;
    }
  for (int i = 0; i < counter; i++){
    insert_task(ReadyList, extract_head(WaitingList)); //insert the expired ...
  }                                                    //...task to ReadyList
  NextTask = ReadyList->pHead->pTask; 
}


