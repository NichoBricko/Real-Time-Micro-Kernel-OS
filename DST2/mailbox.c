#include <stdio.h>
#include <stdlib.h>
#include "kernel_functions.h"


mailbox * create_mailbox(uint nMessages, uint nDataSize){
  mailbox * new_mailbox;
  new_mailbox = (mailbox *)calloc(1, sizeof(mailbox)); //allocate memory
  if(new_mailbox == NULL)
    return NULL;
  
  new_mailbox->pHead = NULL;    //insert properties for new mailbox
  new_mailbox->pTail = NULL;
  new_mailbox->nMaxMessages = nMessages;
  new_mailbox->nMessages = 0; 
  new_mailbox->nDataSize = nDataSize;
  new_mailbox->nBlockedMsg = 0;

  return new_mailbox;
}

exception remove_mailbox (mailbox* mBox){
  if(mBox->nMessages == 0){ //Check if mailbox is empty
    free(mBox); //Free memory for mailbox
    return OK; //OK if successfull
  }
  else
    return NOT_EMPTY;
  
}


                              
exception send_wait( mailbox* mBox, void* pData){
  if(mBox == NULL || pData == NULL){
    return FAIL;
  }
  isr_off();                                           
  if(mBox->pHead != NULL && mBox->pHead->Status == RECEIVER){
    memcpy(mBox->pHead->pData, pData, mBox->nDataSize);   //Copy senders data to receivers data
    msg* message = pop_msg(mBox);    //Remove receiving task's msg from mailbox
    PreviousTask = ReadyList->pHead->pTask;      
    insert_task(ReadyList, message->pBlock); //Insert blocked task in readylist
    NextTask = mBox->pHead->pBlock->pTask;       
  }else{
    msg* temp = (msg*)malloc(sizeof(msg)); //Allocate temporary message struct
    if(temp == NULL)
      return FAIL;
    temp->pData = (char*)malloc(mBox->nDataSize);
    if(temp->pData == NULL)
      return FAIL;
    memcpy(temp->pData, pData, mBox->nDataSize); //Set data pointer
    temp->Status = SENDER;
    temp->pNext = NULL;
    temp->pPrevious = NULL;
    temp->pBlock = ReadyList->pHead;
    ReadyList->pHead->pMessage = temp;
    
    if(mBox->nMessages == mBox->nMaxMessages)
      pop_msg(mBox);
    
    Msg_add_toMailbox(mBox, temp); //Add msg to mailbox                   
    
    if(temp->pBlock != NULL)
      mBox->nBlockedMsg++;
    
    PreviousTask = ReadyList->pHead->pTask;             //Update previousTask
    insert_task(WaitingList, extract_head(ReadyList));  //Moving sending task from ReadyList to WatingList
    NextTask = ReadyList->pHead->pTask;                 //Update NextTask
  }
                              
  SwitchContext();
  if(ReadyList->pHead->pTask->Deadline <= Ticks){     //Checks if deadline is reached
    isr_off();                                        
    msg * temp = remove_msg(mBox, mBox->pHead); 
    free(temp);
    mBox->nBlockedMsg--;
    isr_on();                                         
    return DEADLINE_REACHED;
  }
  else
    return OK;
}

exception receive_wait ( mailbox* mBox, void*pData){
  isr_off();
  
  if(mBox->pHead != NULL && mBox->pHead->Status == SENDER){
    memcpy( pData, mBox->pHead->pData, mBox->nDataSize); //Copy from sender to receiver
    msg * temp = pop_msg(mBox);
    if(temp->pBlock != NULL){ //If the sender was waiting and in the need of unblocking
      PreviousTask = ReadyList->pHead->pTask;
      insert_task(ReadyList, removeNode(WaitingList, temp->pBlock)); 
      NextTask = ReadyList->pHead->pTask;                  
    }
    else {
      if(temp->pBlock != NULL)
        mBox->nBlockedMsg--;
      free(temp->pData);
      free(temp);
    }  
  }
  else {
    msg *message = (msg*)calloc(1, sizeof(msg));
    if(message == NULL)
      return FAIL;
    message->pData = pData;
    message->Status = RECEIVER;
    message->pNext = NULL;
    message->pPrevious = NULL;
    message->pBlock = ReadyList->pHead;
    
    
    Msg_add_toMailbox(mBox, message);
    PreviousTask = ReadyList->pHead->pTask;
    insert_task(WaitingList, extract_head(ReadyList)); //Switch extract_ReadyList_head()
    NextTask = ReadyList->pHead->pTask;
  }
  
  SwitchContext();
  if(ReadyList->pHead->pTask->Deadline <= Ticks){
    isr_off();
    free(remove_msg(mBox, ReadyList->pHead->pMessage)); 
    ReadyList->pHead->pMessage = NULL;
    isr_on();
    return DEADLINE_REACHED;
  }
  else 
    return OK; 
}

exception send_no_wait( mailbox *mBox, void *pData){
  
  isr_off();
  
  if (mBox->pHead != NULL && mBox->pHead->Status == RECEIVER){
    memcpy(mBox->pHead->pData, pData, mBox->nDataSize);
    msg* temp = pop_msg(mBox);
    PreviousTask = ReadyList->pHead->pTask;
    insert_task(ReadyList, removeNode(WaitingList, temp->pBlock));
    NextTask = ReadyList->pHead->pTask;
    SwitchContext();
  }
  else {
    msg*message = (msg*)calloc(1, sizeof(msg));
    if (message == NULL)
      return FAIL;
    message->pData = (char*)malloc(mBox->nDataSize);
    if(message->pData == NULL)
      return FAIL;
    memcpy(message->pData, pData, mBox->nDataSize);
    //Set properties
    message->Status = SENDER;
    message->pBlock = NULL;
    message->pNext = NULL;
    message->pPrevious = NULL;
    if (mBox->nMessages >= mBox->nMaxMessages){
      free(mBox->pHead->pData);
      free(pop_msg(mBox));
    }
    Msg_add_toMailbox(mBox, message);
  }
  return OK;
}

exception receive_no_wait( mailbox* mBox, void*pData){
  if(mBox == NULL)
    return FAIL;
  
  isr_off(); 
  if(mBox->pHead->Status == SENDER){
    memcpy(pData, mBox->pHead->pData, mBox->nDataSize);
    msg* temp = pop_msg(mBox);
    if(temp->pBlock != NULL){
      PreviousTask = ReadyList->pHead->pTask;
      insert_task(ReadyList, removeNode(WaitingList, temp->pBlock));
      NextTask = ReadyList->pHead->pTask;
      SwitchContext();
    }else{
      free(temp->pData);
      free(temp);
    }
    
  }else{
    return FAIL;
  }
  return OK; 
}
                            
//Functions that complement the pseudocode
msg * pop_msg(mailbox *mBox){
  if(mBox->pHead == NULL) //Check if the mailbox is empty
    return NULL;
  
  msg * message = mBox->pHead;
  mBox->pHead = mBox->pHead->pNext;
  message->pNext = NULL;
  message->pPrevious = NULL;
  
  
  if(mBox->pHead == NULL){
    mBox->pTail = NULL;
  }
  else{
    mBox->pHead->pPrevious = NULL;
  }
  if(message->pBlock != NULL)
    mBox->nBlockedMsg--;
  
  mBox->nMessages--;
  
  return message; 
}

msg* remove_msg(mailbox* mBox, msg* message){
  if(mBox == NULL || message == NULL)
    return NULL;
  //removes message from the mailbox based on position
  if(mBox->pHead == message && mBox->pTail == message){ //if it's only 1 in list
    message->pNext = NULL;
    message->pPrevious = NULL;
    mBox->pHead = NULL;
    mBox->pTail = NULL;
  }  
  else if(mBox->pTail == message){ //if it is on last place
    mBox->pTail = mBox->pTail->pPrevious;
    mBox->pTail->pNext = NULL;
    message->pPrevious = NULL;
  }
  else if (mBox->pHead == message){ //if it's first
    mBox->pHead = mBox->pHead->pNext;
    mBox->pHead->pPrevious = NULL;
    message->pNext = NULL;
    message->pPrevious = NULL;
  }
  else { //somewhere inbetween
    message->pNext->pPrevious = message->pPrevious;
    message->pPrevious->pNext = message->pNext;
    message->pNext = NULL;
    message->pPrevious = NULL;
    
  } //decrement amount of messages
  mBox->nMessages--;
  return message;
}

                           
exception Msg_add_toMailbox(mailbox* mBox, msg* message){ //Adds msg to mailbox at the end of list
  if(message == NULL || mBox == NULL)
    return FAIL;
  
  if(mBox->pHead == NULL){
    mBox->pHead = message;
    mBox->pTail = message;
  }
  else{
    mBox->pTail->pNext = message;
    message->pPrevious = mBox->pTail;
    mBox->pTail = message;
  } 

  mBox->nMessages++;
  return OK;

}                          