#include <cstdlib>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <sstream>
#include <string>
#include <printf.h>
#include <unistd.h>
#include <RF24/RF24.h>

int PayloadSize = 8;

// Radio pipe addresses for the 2 nodes to communicate.
const uint64_t pipes[2] = { 0xF0F0F0F0E1LL, 0xABCDABCD71LL };

// ======[ Set DBUG to true if you want to troublshoot ]====
bool DBUG=true;

// RPi generic:
//RF24 radio(0,24);
RF24 radio(RPI_V2_GPIO_P1_15, RPI_V2_GPIO_P1_24, BCM2835_SPI_SPEED_8MHZ);

void dbug(const char *msg){
  if (DBUG) { printf("%s \n",msg); }
}

void dbug_int(int num, const char *msg, bool NL){
  if (DBUG) { 
    if (NL){
      printf("%s %d \n",msg,num);
    }else{
      printf("%s %d",msg,num);
    }
  }
}

bool ACKreceive(unsigned long * ACKPAYLOAD){
    dbug("        ACKreceive: Entered fuctions.") ;

    bool vReturn = false;
    bool done=false;
    int i=0;

    radio.startListening();    

    while (i<2000 && !done){


      if ( radio.isAckPayloadAvailable () ){
        // Grab the response, compare, and send to debugging spew
        unsigned long response;
        radio.read( &response, sizeof(unsigned long) );

        if (response == 100){
          dbug_int(response,"        ACKreceived: AckPayloadAvailable: ACKp is ",true);
          vReturn = true;
          done=true;
          *ACKPAYLOAD=response;
        }else{
          dbug_int(response,"        ACKreceived: AckPayloadAvailable FAILED VALUE: ACKp is ",true);
          vReturn = false;
          done=true;
        }
      }
      i++;
      if (i>1990 && i<1999){
        radio.startListening();        
        delay(5);
      }
    }
    if (!done){
      dbug("        ACKreceive: Radio never became available to receive ACK.");
      vReturn = false;
    }
    return vReturn;
}

bool sendOrder(unsigned long * ACKPAYLOAD, const char *order){
    dbug("     sendOrder: Entered fuctions.");

    char msg[PayloadSize];
    strcpy(msg,order);
    bool vReturn=false;
    bool done=false;
    bool ok=false;
    int i=0;
    unsigned long ACKp;

    dbug("     sendOrder: Sending.......");          // Use a simple byte counter as payload 
    
    while (i<20 && !done){
      radio.stopListening();                                  // First, stop listening so we can talk.
      radio.flush_tx();
      //delay(5);

      ok = radio.write(msg,PayloadSize,0);
      if (ok){
        dbug("WRITE OK");
      }else{
        dbug("WRITE ***NOT*** OK");
      }

      if (ACKreceive(ACKPAYLOAD)){
        ACKp=*ACKPAYLOAD;
        dbug_int(ACKp,"     SendOrder: Ack received value is ",true);
        vReturn = true;
        done = true;
      }
      i++;
      if (i>1){dbug_int(i,"     SendOrder: Number of times to get ACK back ",true);}
    }
      if (!done) {
        dbug("     SendOrder: ACKreceive:false - Message sent faild."); 
        vReturn = false;
      }
      return vReturn;
} 

int main(int argc, char * argv[]){

  radio.begin();
  // radio.setRetries(15,15);
  // optionally, reduceing the payload size, seems to improve reliability (8 is good, 32 not as good)
  radio.setAutoAck(false);
  radio.enableAckPayload();               // Allow optional ack payloads
  //radio.setPALevel(RF24_PA_MAX);
  // Open pipes to other nodes for communication. The Write Pipe Address must go to the Read Pipe on the 
  // other node and vice versa on the other node.  Open 'our' pipe for writing.
  // Open the 'other' pipe for reading, in position #1 (we can have up to 5 pipes open for reading)
  radio.openWritingPipe(pipes[0]);
  radio.openReadingPipe(0,pipes[1]);
  radio.setRetries(15,15);
  // Start listening
  radio.printDetails();

  char vOrder[PayloadSize];
  unsigned long ACKPAYLOAD;

  strcpy(vOrder,argv[1]);

  dbug("main: Calling sendOrder()");
  if (sendOrder(&ACKPAYLOAD, vOrder)){
      dbug("main: sendOrder: good it was received");
      dbug_int(ACKPAYLOAD,"main: sendOrder: ACK VALUE = ",true);
  }else{
      dbug("main: sendOrder: Error sending");
  }

}
