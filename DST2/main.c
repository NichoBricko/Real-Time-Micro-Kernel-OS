        /*  Unit test to check if terminate() works correctly, and if create_task( )
              (1) correctly initializes TCBs, and,
              (2) correctly updates the ReadyList
            assumes that you have used the name ReadyList to point to the Ready list
            and assumes that you have written all of your kernel functions in a single
            C file called kernel_functions.c
            And of course, init_kernel() and run() also need to work correctly
         */

#include "system_sam3x.h"
#include "at91sam3x8.h"
#include "kernel_functions.h"


unsigned int g0=0, g1=0, g2=0; 
/* gate flags for various stages of unit test */

unsigned int low_deadline  = 1000;    
unsigned int high_deadline = 100000;

mailbox *charMbox; 
mailbox *intMbox; 

//task 1 function body
void task_body_1(mailbox * mBox) {
    int received_data = 0;

    //wait for some time
    wait(5);

    //receive a message from the mailbox
    exception result = receive_wait(mBox, &received_data);
    if(result != OK) 
      while(1); //failed to recieve message
    terminate();
}

//task 2 function body
void task_body_2(mailbox * mBox) {
    int data = 123;

    //wait for some time
    wait(10);

    //send a message to the mailbox
    exception result = send_wait(mBox, &data);
    if(result != OK) 
        while(1); //failed to send message
    terminate();
}

void test_inter_process_communication() {
  
    //create a mailbox for message communication
    mailbox* mBox = create_mailbox(5, sizeof(int));

    //create task 1
    create_task(task_body_1, 5);

    //create task 2
    create_task(task_body_2, 20);

    //wait for some time
    wait(10);

    //send a message from task 1 to task 2
    int data = 123;
    exception result = send_wait(mBox, &data);
    if(result != OK) {
        while(1); //fail
    }

    //receive the message in task 2
    int received_data = 0;
    result = receive_wait(mBox, &received_data);
    if(result != OK) {
        while(1); //fail
    }

    //check if the received message is correct
    if(received_data != data) {
        while(1); //fail
    }

    //delete the mailbox
    free(&mBox);
    while(1){
      
      //code works as intended
      
    }
}




int test_wait(){
  set_ticks(0);
  int tickcounter = 0;
  
  mailbox* mBox = create_mailbox(5, sizeof(int));
  
  create_task(task_body_1, 100);
  create_task(task_body_2, 200);
  
  wait(10);
  
  char message = 'a';
  send_wait(mBox, &message);
  
  while(mBox->nMessages == 0){
    wait(1);
    tickcounter++;
  }
  //test passed
  return tickcounter;
}


void main()
{
  SystemInit(); 
  SysTick_Config(100000); 
  SCB->SHP[((uint32_t)(SysTick_IRQn) & 0xF)-4] =  (0xE0);      
  isr_off();
  
  g0 = OK;
  exception retVal = init_kernel(); 
  
  
  
  test_inter_process_communication();
 
  
  run();
  
  while(1){ /* something is wrong with run */}
}



