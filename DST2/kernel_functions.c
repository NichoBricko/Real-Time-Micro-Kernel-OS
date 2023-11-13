#include "kernel_functions.h"


int Ticks;
int KernelMode;
TCB *PreviousTask, *NextTask;
list *ReadyList, *WaitingList, *TimerList;
static void idle(){
  while(1);
}


  

exception init_kernel(void){
  set_ticks(0);
  
  ReadyList = (list *)malloc(sizeof(list));
  if (ReadyList == NULL)
    return FAIL;
  else{
    ReadyList->pHead = NULL;
    ReadyList->pTail = NULL;
  }

  WaitingList = (list *)malloc(sizeof(list));
  if (WaitingList == NULL)
    return FAIL;
  else{
    WaitingList->pHead = NULL;
    WaitingList->pTail = NULL;
  }

  TimerList = (list *)malloc(sizeof(list));
  if (TimerList == NULL)
    return FAIL;
  else{
    TimerList->pHead = NULL;
    TimerList->pTail = NULL;
  }
 
  if(OK == create_task(idle, UINT_MAX)){
    KernelMode = INIT;
    return OK;
  }else
    return FAIL;
}



exception create_task(void (*task_body)(), uint deadline) {
  if (task_body == NULL)
    return FAIL;
  
  TCB *new_tcb;
  new_tcb = (TCB *)calloc(1, sizeof(TCB));
  if (new_tcb == NULL) //check if allocation succeded
    return FAIL;

  new_tcb->PC = task_body;
  new_tcb->SPSR = 0x21000000;
  new_tcb->Deadline = deadline;

  new_tcb->StackSeg[STACK_SIZE - 2] = 0x21000000;
  new_tcb->StackSeg[STACK_SIZE - 3] = (unsigned int)task_body;
  new_tcb->SP = &(new_tcb->StackSeg[STACK_SIZE - 9]);

  listobj *new_listobj = createNode(new_tcb);
  //new_listobj = (listobj *)calloc(1, sizeof(listobj));
  if (new_listobj == NULL)
    return FAIL;
  new_listobj->pTask = new_tcb;
  new_listobj->pNext = NULL;
  new_listobj->pPrevious = NULL;
  
  
  if (KernelMode == INIT) {
    insert_task(ReadyList, new_listobj);
    return OK;

  } else {
    isr_off();                           // disable interrupts
    PreviousTask = ReadyList->pHead->pTask;              // update previous task
    insert_task(ReadyList, new_listobj); // insert new task
    NextTask = ReadyList->pHead->pTask;                  // update next task

    SwitchContext(); // switch context
    return OK;
  }
  
}


void run(){
  Ticks = 0;
  
  KernelMode = RUNNING;
  NextTask = ReadyList->pHead->pTask;

  LoadContext_In_Run();

  /* supplied to you in the assembly file
  * does not save any of the registers
  * but simply restores registers from saved values
  * from the TCB of NextTask
  */

}




void terminate(){
  listobj *leavingObj;
  isr_off();
  leavingObj = extract_head(ReadyList);
  
  /* extract() detaches the head node from the ReadyList and
  * returns the list object of the running task */
  
  NextTask = ReadyList->pHead->pTask;
  switch_to_stack_of_next_task();


  free(leavingObj->pTask);
  free(leavingObj);
  LoadContext_In_Terminate();
  
  /* supplied to you in the assembly file
  * does not save any of the registers. Specifically, does not save the
  * process stack pointer (psp), but
  * simply restores registers from saved values from the TCB of NextTask
  * note: the stack pointer is restored from NextTask->SP
  */

}



//Completion Functions to pseudocode
listobj *extract_head(list * List) {
  if(List == NULL || List->pHead == NULL)
    return NULL;
  else{
    listobj *extraction;
    extraction = List->pHead;

    List->pHead = List->pHead->pNext;
    extraction->pNext = NULL;
    extraction->pPrevious = NULL;
    if(List->pHead == NULL)
      List->pTail = NULL;
    else
      List->pHead->pPrevious = NULL;
    
    return extraction;
  }
}



exception insert_task(list *listtype, listobj *object) {

   if (object == NULL | listtype == NULL) // Can not add NULL
    return FAIL;
   
  listobj *current; //current task initiated when function was called
  current = listtype->pHead;
  
  
  //Insert into an empty list
  if(listtype->pHead == NULL){ //inserting task to an empty list
    listtype->pHead = object;
    listtype->pTail = object;
    return OK;
  }
  //Insert as tail if it is a Waiting- or TimeList
  if(listtype == WaitingList || listtype == TimerList){
    listtype->pTail->pNext = object;
    object->pPrevious = listtype->pTail;
    listtype->pTail = listtype->pTail->pNext;
    return OK;
  }
  
  //Finding where object should be placed in List
  while(current != NULL){
    
    if(object->pTask->Deadline >= current->pTask->Deadline)
      current = current->pNext; 
    else
      break;
    }
  
  if(current == NULL){   //inserting as tail  
    object->pPrevious = listtype->pTail;
    listtype->pTail->pNext = object;
    listtype->pTail = object;
  }
  else if(listtype->pHead == current){ //inserting as head
    object->pNext = listtype->pHead;
    listtype->pHead->pPrevious = object;
    listtype->pHead = object;
  }
  else{ //inserting task in the middle
    object->pNext = current;
    object->pPrevious = current->pPrevious;
    current->pPrevious = object;
    object->pPrevious->pNext = object;
  }
  return OK;  
}

listobj * createNode(TCB *task) {
  listobj * node = (listobj*)malloc(sizeof(listobj)); 
  if (node == NULL)     //Check if memory allocation failed
    return NULL;
  else{
    node->pTask = task; //insert TCB to created node
    node->nTCnt = 0;
    node->pMessage = NULL;
    node->pNext = NULL;
    node->pPrevious = NULL;
  }
  return node;
}

listobj *removeNode (list* List, listobj* Node){
  if(List == NULL || List->pHead == NULL || Node == NULL)
    return NULL;

  if(List->pHead == Node && List->pTail == Node){ //One item in list
    List->pHead = NULL;
    List->pTail = NULL;
    Node->pPrevious = NULL;
    Node->pNext = NULL;
  }
  else if(List->pHead == Node){ //If head is getting removed
    List->pHead = List->pHead->pNext;
    List->pHead->pPrevious = NULL;
    Node->pNext = NULL;
    Node->pPrevious = NULL;
  }
  else if(List->pTail == Node){
    List->pTail = List->pTail->pPrevious;
    List->pTail->pNext = NULL;
    Node->pPrevious = NULL;
    Node->pNext = NULL;
  }else{
    Node->pPrevious->pNext = Node->pNext;
    Node->pNext->pPrevious = Node->pPrevious;
    Node->pPrevious = NULL;
    Node->pNext = NULL;
  }
  return Node;
}


