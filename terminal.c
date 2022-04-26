// use MinGW gcc
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <inttypes.h>
#include <getopt.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

#define VERSION "v0.3"

#include "rs232.h"

int CPORT_NR = 4;
int BDRATE = 115200;
char FORMAT = 's';
char mode[]={'8','N','1',0};

//char keyin_buf[512]= {0};
char tx_buf[512]= {0};

void RS232_tx();
void RS232_rx();

uint8_t IsConnected = 0;

int tx_formating(void);
void rx_formating(uint8_t *buf_p,uint8_t len);

int main(int argc, char **argv) {
	int ch;
	while (1) {
        static struct option long_options[] =
          {
            {"help"     , no_argument      , 0, '?'},
			{"version"  , no_argument      , 0, 'v'},
            {"port"     , required_argument, 0, 'p'},
			{"baudrate" , required_argument, 0, 'b'},
			{"Format"	, required_argument, 0,	'f'},
            {0, 0, 0, 0}
          };
        /* getopt_long stores the option index here. */
        int option_index = 0;

        ch = getopt_long (argc, argv, "p:b:f:v",
                         long_options, &option_index);

        /* Detect the end of the options. */
        if (ch == -1)
          break;

        switch (ch) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (long_options[option_index].flag != 0)
					break;
				printf ("option %s", long_options[option_index].name);
				if (optarg)
					printf (" with arg %s", optarg);
				printf ("\n");
			break;

			case 'b':
				BDRATE = (int)strtoimax(optarg,NULL,10);
				break;
			case 'p':
				CPORT_NR = (int)strtoimax(optarg,NULL,10)-1;
				break;
			case 'f':
				FORMAT = *optarg;
				break;
			case 'v':
				printf("Now version is %s\n",VERSION);
				return 0;
				break;
			case '?':
				printf("\nUsage: termainal.exe [--port <com>] [--baudrate <baud>]\n");
				printf("  --port <com>        Use desinated port <com>\n");
				printf("  -p <com>            Same as --port <com>\n");
				printf("  --baudrate <baud>   Set baudrate\n");
				printf("  -b <baud>           Same as -b <baud>\n");
				printf("  --help              Display help information\n");
				printf("  -?                  Same as --help\n");
				printf("  --version           Display version\n");
				printf("  -v                  Same as --version\n");
				return 0;
				break;
			default:
				return 0;
		}
      }



	pthread_t id;

	int ret;
	ret=pthread_create(&id,NULL,(void *) RS232_rx,NULL);
	if(ret!=0) {
		printf ("Create pthread error!\n");
		exit (1);
	}

	if(RS232_OpenComport(CPORT_NR, BDRATE, mode)) {
		printf("Can not connect to COM%d\n",CPORT_NR+1);
	} else {
		printf("Connect to COM%d , Format is %c\n",CPORT_NR+1,FORMAT);
		IsConnected = 1;
		while ( 1 ) {
			scanf("%s",tx_buf);
			if(strcmp(tx_buf,"q")==0) break;
			int len = tx_formating();
			RS232_SendBuf(CPORT_NR,tx_buf,len);
		}
		RS232_CloseComport(CPORT_NR);
		printf("Disconnect from COM%d\n",CPORT_NR+1);
		IsConnected = 0;
	}
	return (0);
}

void RS232_tx() {
    while(1) {
        scanf("%s",tx_buf);
        RS232_cputs(CPORT_NR,tx_buf);
        #ifdef _WIN32
            Sleep(1000);
        #else
            usleep(1000000);  /* sleep for 1 Second */
        #endif
    }
  return;
}


int tx_formating(void){
	int offset=0;
	unsigned int data=0;
	int input_idx = 0;

	char temp[512]={0};
	int temp_idx = 0;

	while(sscanf(tx_buf+input_idx,"%2x%n",&data,&offset)!=EOF) {
		input_idx+=offset;
		temp[temp_idx] = (unsigned char)data;
		temp_idx++;
	}
	memcpy(tx_buf,temp,temp_idx);
	return temp_idx;
}

void RS232_rx() {
    int n;
	uint8_t rx_buf[4096];
	while(1)
    while( IsConnected ) {
        n = RS232_PollComport(CPORT_NR, rx_buf, 4095);
        if(n > 0) {
			rx_formating(rx_buf,n);
            printf_s("%s\n",(char *)rx_buf);
        }

        #ifdef _WIN32
            Sleep(100);
        #else
            usleep(100000);  /* sleep for 100 milliSeconds */
        #endif
    }

}
void rx_formating(uint8_t *buf_p,uint8_t len){
	int n=0;
	int16_t offset=0;
	char temp[4096];
	int temp_idx = 0;
	unsigned int data=0;

	while(n<len) {
		data = *(buf_p+n);
		offset = sprintf(temp+temp_idx,"%02X",data);
		temp_idx+=offset;
		n++;
	}
	sprintf(temp+temp_idx,"%c",'\0');
	memcpy((char*)buf_p,temp,temp_idx+1);
}