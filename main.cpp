/* =====================================================================
   Copyright © 2016, Avnet (R)

   Contributors:
     * James M Flynn, www.em.avnet.com 
     * Chuck Swiger
     
   Licensed under the Apache License, Version 2.0 (the "License"); 
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, 
   software distributed under the License is distributed on an 
   "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, 
   either express or implied. See the License for the specific 
   language governing permissions and limitations under the License.

    @file          WNC_Pubnub_obd2b
    @version       1.0b
    @date          Dec 2016

 - This version has the AccelX,Y,Z sensor and code for GPS
======================================================================== */

#include "mbed.h"
#include "WNCInterface.h"
#include "main.h"   // obd2 includes
#include "pubnub_sync.h"
#include "sensors.h"
#include "hardware.h"

#define DEBUG
#define MBED_PLATFORM

#define CRLF    "\n\r"

I2C i2c(PTC11, PTC10);    //SDA, SCL -- define the I2C pins being used

WNCInterface wnc;    
MODSERIAL pc(USBTX,USBRX,256,256);
MODSERIAL obd2(PTC15,PTC14,256,256);     //  (TX,RX,txbuffer,rxbuffer)
                                        // This is the Bluetooth J199 connector - pin3 DCT14 RX,  pin4 DTC15 TX
pubnub_t *pbp = pubnub_alloc();


DigitalOut rled(PTB19);   // RED panel lamp
DigitalOut gled(PTB18);   // GREEN


long OBDBAUD = 9600;
long PCBAUD = 115200;

K64F_Sensors_t  SENSOR_DATA =
{
    .Temperature        = "0",
    .Humidity           = "0",
    .AccelX             = "0",
    .AccelY             = "0",
    .AccelZ             = "0",
    .GPS_Satellites     = "0",
    .GPS_Latitude       = "0",
    .GPS_Longitude      = "0",
    .GPS_Altitude       = "0",
    .GPS_Speed          = "0",
    .GPS_Course         = "0"
};
    
char* itoa(int val, int base){
       static char buf[32] = {0};
       int i = 30;
       for(; val && i ; --i, val /= base)
            buf[i] = "0123456789abcdef"[val % base];
       return &buf[i+1];
}


//Periodic timer
Ticker OneMsTicker;
volatile bool bTimerExpiredFlag = false;
int OneMsTicks = 0;
int iTimer1Interval_ms = 1000;

void OneMsFunction() 
{
    OneMsTicks++;
    if ((OneMsTicks % iTimer1Interval_ms) == 0)
    {
        bTimerExpiredFlag = true;
    }            
} //OneMsFunction()


void GetDTC(void)
{
    char jsonreply[140] = {'\0'};
    dtc_read();    // populate the DTC struct if any
    if(has_dtc) {
        strcpy(jsonreply,"{\"");
        for (int i=0; i<MAX_DTC_READ; i++) {
           strcat(jsonreply,itoa((i+1),10));
           strcat(jsonreply,"\":\"");
           if(i!=MAX_DTC_READ-1) {   // last one?
               strcat(jsonreply,DTC[i].code);
               strcat(jsonreply,"\",\"");
           } else {
               strcat(jsonreply,DTC[i].code);
               strcat(jsonreply,"\"}");
           }
        }
    } else {
        strcpy(jsonreply,"{\"1\":\"none\"}");
    }
    pubnub_res rslt = pubnub_publish(pbp, "carpal2", jsonreply);
    if (rslt != PNR_STARTED)
       pc.printf("Failed to start publishing, rslt=%d"CRLF, rslt);
    else {
       rslt = pubnub_await(pbp);
          if (rslt != PNR_OK)
             pc.printf("Failed to finished publishing, rslt=%d"CRLF, rslt);
          else {
             pc.printf("Published! Response from Pubnub: %s"CRLF, pubnub_last_publish_result(pbp));
          }
    }
        
} // GetDTC

bool parse_JSON(char const* json_string)
{
    char const* beginquote;
    char token[] = "get:";
    beginquote = strstr(json_string, token );
    if ((beginquote != 0))
    {
        char getreq = beginquote[strlen(token)];     // this only gets a single letter at the position after 'get:'
        pc.printf("get Found : %c\r\n", getreq);
        switch(getreq)
        {
            case 'd':
            { // dtc
                GetDTC();
                break;
            }
            default:
            {
                break;
            }
        } //switch(getreq)
        return true;
    }
    else
    {
        return false;
    }
} //parse_JSON


int main()
{
    char data[256];
    int ret;
    
    long vehicle_speed = 0;
    long engine_rpm = 0;
    long fuel_status = 0;
    long engine_load = 0;
    long coolant_temp = 0;
    long stft_b1 = 0;
    long ltft_b1 = 0;
    long stft_b2 = 0;
    long ltft_b2 = 0;
    long timing_adv = 0;
    long int_air_temp = 0;
    long maf_air_flow = 0;
    long throttle_pos = 0;
    long b1s2_o2_v = 0;
    

    rled = 1;   // red panel lamp while initializing
    gled = 0;

    // Configure PC BAUD
    pc.baud(PCBAUD);

    //debug
    wait(10);   // wait for terminal connected
    pc.printf(WHT "STARTING WNCInterface PubNub Test" CRLF);
    
    //Create a 1ms timer tick function:
    iTimer1Interval_ms = SENSOR_UPDATE_INTERVAL_MS;
    OneMsTicker.attach(OneMsFunction, 0.001f) ;

    //Initialize the I2C sensors that are present
    sensors_init();
    read_sensors();

    // init and connect the WNCInterface cell modem
    pc.printf(DEF "init() returned 0x%04X" CRLF, wnc.init(NULL,&pc));
    ret = wnc.connect();     
    wnc.doDebug(0);   // if you want a LOT of AT commands logged (1)

    pc.printf("IP Address: %s " CRLF, wnc.getIPAddress());
    pc.printf("-------------------------------------" CRLF);
    pc.printf("starting PubNub sync..." CRLF);

    // create the Pubnub interface
    pubnub_init(pbp, "pub-c-xxxxxxxx-yyyy-zzzz-aaaa-bbbbbbbbbbbb", "sub-c-xxxxxxxx-yyyy-zzzz-aaaa-bbbbbbbbbbbb");  

    // Setup STN1110 UART
    pc.printf("Configuring OBD UART\r\n");
    Init();
    pc.printf("Configure done..\r\n");
    while(1) {
        if (bTimerExpiredFlag)
        {
           bTimerExpiredFlag = false;
           rled = 1;
           pc.printf("Refresh...\n\r");
           Refresh();
           pc.printf("Get coolant temp...\n\r");
           get_pid(COOLANT_TEMP,&coolant_temp);
           pc.printf("Get rpm...\n\r");
           get_pid(ENGINE_RPM,&engine_rpm);
           pc.printf("Coolant: %d\n\rRPM: %d\n\r",coolant_temp,engine_rpm);
           // short keys to keep air co$t$ down 
           // oc or   -  obd2 coolant temp, rpm
           // la lo   -  gps latitude, longitude
           // ax ay   -  accelarometer X,Y 
           read_sensors();
           snprintf(data,256,"{\"la\":%s,\"lo\":%s,\"ax\":%s,\"ay\":%s,\"oc\":%d,\"or\":%d}",SENSOR_DATA.GPS_Latitude,SENSOR_DATA.GPS_Longitude,SENSOR_DATA.AccelX,SENSOR_DATA.AccelY,coolant_temp,engine_rpm);
           // while trying to get GPS to work
           //snprintf(data,256,"{\"ax\":%s,\"ay\":%s,\"oc\":%d,\"or\":%d}",SENSOR_DATA.AccelX,SENSOR_DATA.AccelY,coolant_temp,engine_rpm);
           // publish json and wait for result
           pubnub_res rslt = pubnub_publish(pbp, "carpal2", data);
           if (rslt != PNR_STARTED) {
              pc.printf("Failed to start publishing, rslt=%d"CRLF, rslt);
              rled = 1;
              gled = 0;
           } else {
              rslt = pubnub_await(pbp);
              if (rslt != PNR_OK) {
                 pc.printf("Failed to finished publishing, rslt=%d"CRLF, rslt);
                 rled = 1;
                 gled = 0;
              } else {
                 pc.printf("Published! Response from Pubnub: %s"CRLF, pubnub_last_publish_result(pbp));
                 rled = 0;
                 gled = 1;
              }
           }
        
           rslt = pubnub_subscribe(pbp, "carpal2", 0);
           if (rslt != PNR_STARTED) {
              pc.printf("Failed to start subscribing, rslt=%d"CRLF, rslt);
              rled = 1;
              gled = 0;
           } else {
              rslt = pubnub_await(pbp);
              if (rslt != PNR_OK) { 
                 pc.printf("Failed to finished subscribing, rslt=%d"CRLF, rslt);\
                 rled = 1;
                 gled = 0;
              } else {
                 pc.printf("Subscribed! Received messages follow:"CRLF);
                 while (char const *msg = pubnub_get(pbp)) {
                    pc.printf("subscribe got: %s"CRLF, msg);
                    parse_JSON(msg);
                }
              }
              rled = 0;
              gled = 1;
           } // subscribe result 
        }  // end timer expired
    } // end infinite loop
 }

char pid_reslen[] =
{
  // pid 0x00 to 0x1F
  4,4,2,2,1,1,1,1,1,1,1,1,2,1,1,1,
  2,1,1,1,2,2,2,2,2,2,2,2,1,1,1,4,

  // pid 0x20 to 0x3F
  4,2,2,2,4,4,4,4,4,4,4,4,1,1,1,1,
  1,2,2,1,4,4,4,4,4,4,4,4,2,2,2,2,

  // pid 0x40 to 0x4E
  4,8,2,2,2,1,1,1,1,1,1,1,1,2,2
};



/***********************
    Init
 ***********************/
char Init()
{
   obd2.baud(OBDBAUD);
   char str[STRLEN];
  // reset
  stn1110_command(str, "ATWS\r");
  // turn echo off
  stn1110_command(str, "ATE0\r");
  
  // send 01 00 until we are connected
  do
  {
    stn1110_command(str, "0100\r");
    wait(1);         // might want to change this to return non-zero for engine off or something, otherwise it's stuck here
  }
  while(stn1110_check_response("0100", str)!=0);
  
  // ask protocol
  stn1110_command(str, "ATDPN\r");
  
  check_supported_pids();
  
  return 0;
}

char stn1110_read(char *str, char size)
{
  int b;
  char i=0;
  
  // wait for something on com port
  while((b=obd2.getc())!=PROMPT && i<size)      // this blocks! Arduino Serial.read would return -1
  {
    if(b>=' ')
      str[i++]=b;
  }

  if(i!=size)  // we got a prompt
  {
    str[i]=NUL;  // replace CR by NUL
    return PROMPT;
  }
  else
    return DATA;
}

void stn1110_write(char *str)
{
  while(*str!=NUL)
    obd2.putc(*str++);
}

char stn1110_check_response(const char *cmd, char *str)
{
  // cmd is something like "010D"
  // str should be "41 0D blabla"
  if(cmd[0]+4 != str[0]
    || cmd[1]!=str[1]
    || cmd[2]!=str[3]
    || cmd[3]!=str[4])
    return 1;

  return 0;  // no error
}

char stn1110_compact_response(char *buf, char *str)
{
  char i=0;

  str+=6;
  while(*str!=NUL)
    buf[i++]=strtoul(str, &str, 16);  // 16 = hex

  return i;
}

char stn1110_command(char *str, char *cmd)
{
  strcpy(str, cmd);
  stn1110_write(str);
  return stn1110_read(str, STRLEN);
}

void check_supported_pids(void)
{

  // on some ECU first PID read attemts some time fails, changed to 3 attempts
  for (char i=0; i<3; i++)
  {
    pid01to20_support = (get_pid(PID_SUPPORT00, &tempLong)) ? tempLong : 0;
    if (pid01to20_support) 
      break; 
  }

  if(is_pid_supported(PID_SUPPORT20))
    if (get_pid(PID_SUPPORT20, &tempLong))
      pid21to40_support = tempLong; 

  if(is_pid_supported(PID_SUPPORT40))
    if (get_pid(PID_SUPPORT40, &tempLong))
      pid41to60_support = tempLong;
}

char is_pid_supported(char pid)
{
  if(pid==0)
    return 1;
  else
  if(pid<=0x20)
  {
    if(1L<<(char)(0x20-pid) & pid01to20_support)
      return 1;
  }
  else
  if(pid<=0x40)
  {
    if(1L<<(char)(0x40-pid) & pid21to40_support)
      return 1;
  }
  else
  if(pid<=0x60)
  {
    if(1L<<(char)(0x60-pid) & pid41to60_support)
      return 1;
  }

  return 0;
}


char get_pid(char pid, long *ret)
{
  char cmd_str[6];   // to send to STN1110
  char str[STRLEN];   // to receive from STN1110
  char i;
  char buf[10];   // to receive the result
  char reslen;
 
  // check if PID is supported
  if(!is_pid_supported(pid))
  {
    // Not Supported
    return 0;
  }
  
  // receive length depends on pid
  reslen=pid_reslen[pid];

  sprintf(cmd_str, "01%02X\r", pid);
  stn1110_write(cmd_str);
  stn1110_read(str, STRLEN);
 
  if(stn1110_check_response(cmd_str, str)!=0)
  {
    return 0;
  }
   stn1110_compact_response(buf, str);

   *ret=buf[0]*256U+buf[1];

  // Calculate different for each PID
  switch(pid)
  {
  case ENGINE_RPM:
    *ret=*ret/4U;
  break;
  case MAF_AIR_FLOW:
  break;
  case VEHICLE_SPEED:
    *ret=buf[0];
  break;
  case FUEL_STATUS:
  case LOAD_VALUE:
  case THROTTLE_POS:
  case REL_THR_POS:
  case EGR:
  case EGR_ERROR:
  case FUEL_LEVEL:
  case ABS_THR_POS_B:
  case ABS_THR_POS_C:
  case ACCEL_PEDAL_D:
  case ACCEL_PEDAL_E:
  case ACCEL_PEDAL_F:
  case CMD_THR_ACTU:
    *ret=(buf[0]*100U)/255U;
  break;
  case ABS_LOAD_VAL:
    *ret=(*ret*100)/255;
  break;
  case B1S1_O2_V:
  case B1S2_O2_V:
  case B1S3_O2_V:
  case B1S4_O2_V:
  case B2S1_O2_V:
  case B2S2_O2_V:
  case B2S3_O2_V:
  case B2S4_O2_V:
  case O2S1_WR_V:
  case O2S2_WR_V:
  case O2S3_WR_V:
  case O2S4_WR_V:
  case O2S5_WR_V:
  case O2S6_WR_V:
  case O2S7_WR_V:
  case O2S8_WR_V:
  case O2S1_WR_C:
  case O2S2_WR_C:
  case O2S3_WR_C:
  case O2S4_WR_C:
  case O2S5_WR_C:
  case O2S6_WR_C:
  case O2S7_WR_C:
  case O2S8_WR_C:
  case CMD_EQUIV_R:
  case DIST_MIL_ON:
  case DIST_MIL_CLR:
  case TIME_MIL_ON:
  case TIME_MIL_CLR:
  case INT_AIR_TEMP:
  case AMBIENT_TEMP:
  case CAT_TEMP_B1S1:
  case CAT_TEMP_B2S1:
  case CAT_TEMP_B1S2:
  case CAT_TEMP_B2S2:
  case STFT_BANK1:
  case LTFT_BANK1:
  case STFT_BANK2:
  case LTFT_BANK2:
  case FUEL_PRESSURE:
  case MAN_PRESSURE:
  case BARO_PRESSURE:
    *ret=buf[0];
    if(pid==FUEL_PRESSURE)
      *ret*=3U;
  break;
  case COOLANT_TEMP:
     *ret=buf[0]-40U;
      break;
  case EVAP_PRESSURE:
    *ret=((int)buf[0]*256+buf[1])/4;
  break;
  case TIMING_ADV:
    *ret=(buf[0]/2)-64;
  break;
  case CTRL_MOD_V:
  break;
  case RUNTIME_START:
  break;
  case OBD_STD:
    *ret=buf[0];
  break;
  default:
    *ret=0;
    for(i=0; i<reslen; i++)
    {
      *ret*=256L;
      *ret+=buf[i];
    }
  break;
  }

  return 1;
}


char verifyECUAlive()
{
  char cmd_str[6];   
  char str[STRLEN];   
  sprintf(cmd_str, "01%02X\r", ENGINE_RPM);
  stn1110_write(cmd_str);
  stn1110_read(str, STRLEN);
  
  if(stn1110_check_response(cmd_str, str) == 0)
  {  
    return 1;
  }
  else
  {
    return 0;
  }
}


char Refresh()
{
    //debug
    isIgnitionOn = verifyECUAlive();
    
    //If ignition is on, check for engine
    if (isIgnitionOn) {
        isEngineOn = (get_pid(ENGINE_RPM, &engineRPM) && engineRPM > 0) ? 1 : 0;
    } else { // else engine must be off
        isEngineOn = 0;
    }
    return 0;
}



bool dtc_clear(void)
{
    char cmd_answer[DTC_BUFFER]="";
    
    stn1110_write("04\r");
    stn1110_read(cmd_answer,DTC_BUFFER);
    strip_answer(cmd_answer);

    if (strcmp(cmd_answer, "44")!=0)
    {
       return false;
    } else
    {
        has_dtc=false;
       return true;
    }
}


bool dtc_read(void)
{
    char cmd_answer[DTC_BUFFER]="";
    has_dtc=false;
     
    stn1110_write("03\r");      
    stn1110_read(cmd_answer,DTC_BUFFER);
                       
    for (char i=0;i<MAX_DTC_READ;i++)
    {
        strcpy(DTC[i].code,"");
    }
    
    strip_answer(cmd_answer);

    if (strstr(cmd_answer, "NODATA"))
    {
    //No errors
        return true;
    }
    
    if (strncmp(cmd_answer, "43", 2)!=0)
    {
    //ERROR: Incorrect answer
    return false;
    }
         
    char *ss=cmd_answer+2;
    char dtclen=0;
    
    while (strlen(ss) >= 4)
    {
    const char *prefix[16]=
        {
        "P0", "P1", "P2", "P3",
        "C0", "C1", "C2", "C3",
        "B0", "B1", "B2", "B3",
        "U0", "U1", "U2", "U3",
        };
    uint8_t p=0;
    if ( ((*ss)>='0') && ((*ss)<='9') ) p=(*ss)-'0'; else
    if ( ((*ss)>='A') && ((*ss)<='F') ) p=(*ss)-'A'+10; else
    if ( ((*ss)>='a') && ((*ss)<='f') ) p=(*ss)-'a'+10;
    char code[6];
    strcpy(code, prefix[p]);
    code[2]=ss[1];
    code[3]=ss[2];
    code[4]=ss[3];
    code[5]=0;
     
    if (strcmp(code, "P0000")!=0)
    {
        strcpy(DTC[dtclen].code,code);
        has_dtc=true;
        dtclen++;
    }
    ss+=4;
    }
    
    return true;
}

char *strip_answer(char *s)
{
    char *ss;
    for (ss=s; *s; s++)
    {
    if ( ((*s)!=' ') && ((*s)!='\t') && ((*s)!='\n') && ((*s)!='\r') )
        (*ss++)=(*s);
    }
    (*ss)=0;
    
    return s;
}

