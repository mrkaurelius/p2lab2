#include <stdint.h>
#include "inc/tm4c123gh6pm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <stdbool.h>
#include "inc/hw_types.h"
#include "inc/hw_gpio.h"
#include "inc/hw_memmap.h"
#include "inc/hw_sysctl.h"
#include "driverlib/gpio.h"
#include "driverlib/rom.h"
#include "driverlib/sysctl.h"
#include "driverlib/pin_map.h"
#include "driverlib/uart.h"

#include "driverlib/can.h"

#define RS GPIO_PIN_4 // Energia Pin 5
#define EN GPIO_PIN_5 // Energia Pin 6
#define D4 GPIO_PIN_0 // Energia Pin 23
#define D5 GPIO_PIN_1 // Energia Pin 24
#define D6 GPIO_PIN_2 // Energia Pin 25
#define D7 GPIO_PIN_3 // Energia Pin 26
#define ALLDATAPINS  D7 | D6 | D5 | D4 // tüm data bitlerini orlamanın mantigi ne
//degiskene koymak bir fikir vermeli aslında
#define ALLCONTROLPINS RS | EN

#define DATA_PORT_BASE GPIO_PORTD_BASE //bunun tanımı yok galiba generic birsey

//bunlara ulasabiliyorum
#define CMD_PORT_BASE GPIO_PORTE_BASE
#define DATA_PERIPH SYSCTL_PERIPH_GPIOD
#define CMD_PERIPH SYSCTL_PERIPH_GPIOE

#define FALSE 0
#define TRUE 1
#define BAUDRATE 600

void pulseLCD(void);
void sendByte(char, int);
void setCursorPositionLCD(char, char);
void clearLCD(void);
void initLCD(void);
void printLCD(char*);
void setBlockCursorLCD(void);
void setLineCursorLCD(void);
void cursorOnLCD(void);
void cursorOffLCD(void);
void displayOffLCD(void);
void displayOnLCD(void);
void homeLCD(void);

typedef struct urun
{
	char * urun_ad;
	int urun_id;
	int urun_stok;
	int urun_fiyat;
}urun_t;

typedef struct otomat
{
	int k_100;
	int k_50;
	int k_25;
	urun_t urunler[5];
} otomat_t;

void init_UARTstdio() {
	SysCtlPeripheralEnable(SYSCTL_PERIPH_GPIOA);
	GPIOPinConfigure(0x00000001);
	GPIOPinConfigure(0x00000401);
	GPIOPinTypeUART(0x40004000, 0x00000001 | 0x00000002);
	UARTConfigSetExpClk(0x40004000, SysCtlClockGet(), BAUDRATE,
                        	(UART_CONFIG_WLEN_8 | UART_CONFIG_STOP_ONE |
                         	UART_CONFIG_PAR_NONE));
	UARTStdioConfig(0, BAUDRATE, SysCtlClockGet());
}

void init_port_F() {
   volatile unsigned long tmp; // bu degisken gecikme yapmak icin gerekli
   tmp = SYSCTL_RCGCGPIO_R;    	// allow time for clock to start
   SYSCTL_RCGCGPIO_R |= 0x00000020;  // 1) activate clock for Port F
   GPIO_PORTF_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port F
   GPIO_PORTF_CR_R = 0x01;       	// allow changes to PF-0
   // only PF0 needs to be unlocked, other bits can't be locked
   GPIO_PORTF_DIR_R = 0x0E;      	// 5) PF4,PF0 in, PF3-1 out
   GPIO_PORTF_PUR_R = 0x11;      	// enable pull-up on PF0 and PF4
   GPIO_PORTF_DEN_R = 0x1F;      	// 7) enable digital I/O on PF4-0
}

void init_port_B() {
	// DIGER  YERLERE BULASMAYAN SIKINTISIZ BIR PORT BULMAK LAZIM
	// B BICBIR YERE DOKUNMUYOR GIBI
	// only PB0 needs to be unlocked, other bits can't be locked sadece pb0 kitli digerleri kitlenemez unlock ve anable birbirinden farklı
	// giris set setmek 0 cıkıs set etmek 1
	volatile unsigned long tmp; // bu degisken gecikme yapmak icin gerekli
	tmp = SYSCTL_RCGCGPIO_R;    	// allow time for clock to start
	SYSCTL_RCGCGPIO_R |= SYSCTL_RCGC2_GPIOB;  // 1) activate clock for Port D 0x00000008
	GPIO_PORTB_LOCK_R = 0x4C4F434B;   // 2) unlock GPIO Port B 0x4C4F434B bu hex anlamlımı?
	GPIO_PORTB_CR_R = 0x01;		//  0b0000 0001 allow changes to PB-0

	GPIO_PORTB_DIR_R = 0x00;    // 0b00000000 5) PB4,PB0,PB3-1 giris olarak set edildi
	GPIO_PORTB_PUR_R = 0x1F;    // 0b00011111 TUM PULL UPLAR AKTIF
	GPIO_PORTB_DEN_R = 0x1F;	// 0b00011111 7) enable digital I/O on PB4-0
}

void getCash(otomat_t *tmp_otomat){
	// paraların cinslerini de goster
	volatile unsigned long delay; // compiler optimizasyonunu engellemek icin volatile kullandik
	init_port_F();
	init_port_B();

	clearLCD();
	setCursorPositionLCD(0,0);
	printLCD("para girisi yap..");
	for(delay = 0; delay < 1900000; delay++);

	int button_sag;
	//int k_100count=0,k_50count=0,k_25count=0,k_10count=0;
	int button_out_1,button_out_2,button_out_3,button_out_4;
	// kaç para atıldıgını anlık olarak ve toplam olarak goster
	//int toplam_para = (*(tmp_k_100) * 100) + (*(tmp_k_50) * 50) + (*(tmp_k_25 ) * 25);
	char buffer[16];
	clearLCD();
	setCursorPositionLCD(0,0);
	printLCD("T: ");
	itoa((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25),buffer,10);
	printLCD(buffer);
	for(delay = 0; delay < 200000; delay++);
	while (1) {
		clearLCD();
		setCursorPositionLCD(0,0);
		printLCD("T:");
		itoa((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25),buffer,10);
		printLCD(buffer);
		for(delay = 0; delay < 1000000; delay++);

		button_sag = GPIO_PORTF_DATA_R & 0b00001;
	  	button_out_1 = GPIO_PORTB_DATA_R & 0b00001;
	  	button_out_2 = GPIO_PORTB_DATA_R & 0b00010;
	  	button_out_3 = GPIO_PORTB_DATA_R & 0b00100;
	  	button_out_4 = GPIO_PORTF_DATA_R & 0b00001;

	  	if (button_out_1 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		tmp_otomat->k_25 += 1;
	  		//toplam_para += 25;
			for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			clearLCD();
			setCursorPositionLCD(0,0);
			printLCD("T:");
			itoa((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25),buffer,10);
			printLCD(buffer);
	  	}

	  	if (button_out_2 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		tmp_otomat->k_50 += 1;
	  		//toplam_para += 50;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			clearLCD();
			setCursorPositionLCD(0,0);
			printLCD("T:");
			itoa((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25),buffer,10);
			printLCD(buffer);
	  	}

	  	if (button_out_3 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		tmp_otomat->k_100 += 1;
	  		//toplam_para += 100;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			clearLCD();
			setCursorPositionLCD(0,0);
			printLCD("T:");
			itoa((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25),buffer,10);
			printLCD(buffer);
	  	}

	  	if (button_out_4 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b00100; // 0x02
			clearLCD();
			setCursorPositionLCD(0,0);
			printLCD("para alindi");
	  		setCursorPositionLCD(1,0);
			printLCD("urun secin");
	  		for(delay = 0; delay < 2000000; delay++);
	  		GPIO_PORTF_DATA_R &= ~(0b00100);
	  		break;
	  	}
	}

}

void getChoice(otomat_t *tmp_otomat,otomat_t *otomat){
	bool succes=false;
	bool endChoice=false;
	volatile unsigned long delay; // compiler optimizasyonunu engellemek icin volatile kullandik
	init_port_F();
	init_port_B();
	clearLCD();

	int button_sag,button_sol;
	int button_out_1,button_out_2,button_out_3,button_out_4,button_out_5;
	int volatile s1=0,s2=0,s3=0,s4=0,s5=0;
	char buffer[16];

	while (1) {
		clearLCD();
		setCursorPositionLCD(0,0);
		printLCD("su:");
		itoa(s1,buffer,10);
		printLCD(buffer);

		printLCD("cay:");
		itoa(s2,buffer,10);
		printLCD(buffer);

		printLCD("kahve:");
		itoa(s3,buffer,10);
		printLCD(buffer);

		setCursorPositionLCD(1,0);

		printLCD("cklata:");
		itoa(s4,buffer,10);
		printLCD(buffer);

		printLCD("bskvi:");
		itoa(s5,buffer,10);
		printLCD(buffer);

		for(delay = 0; delay < 1000000; delay++);

		button_sag = GPIO_PORTF_DATA_R & 0b00001;
		button_sol = GPIO_PORTF_DATA_R & 0b10000;
	  	button_out_1 = GPIO_PORTB_DATA_R & 0b00001;
	  	button_out_2 = GPIO_PORTB_DATA_R & 0b00010;
	  	button_out_3 = GPIO_PORTB_DATA_R & 0b00100;
	  	button_out_4 = GPIO_PORTB_DATA_R & 0b01000;
	  	button_out_5 = GPIO_PORTB_DATA_R & 0b10000;
	  	//button_out_4 = GPIO_PORTF_DATA_R & 0b00001;

	  	if (button_out_1 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		s1++;
			for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
	  	}

	  	if (button_out_2 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		s2++;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
	  	}

	  	if (button_out_3 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		s3++;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			printLCD(buffer);
	  	}
	  	if (button_out_4 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		s4++;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			printLCD(buffer);
	  	}

	  	if (button_out_5 == 0) {
	  		GPIO_PORTF_DATA_R |= 0b01000; // 0x02
	  		s5++;
	  		for(delay = 0; delay < 1000000; delay++);
	  	} else {
	  		GPIO_PORTF_DATA_R &= ~(0b01000);
			printLCD(buffer);
	  	}

	  	if (button_sag == 0) {
	  		GPIO_PORTF_DATA_R |= 0b00100; // 0x02
	  		endChoice = true;
			clearLCD();
			setCursorPositionLCD(0,0);
			printLCD("secimler alindi");
	  		for(delay = 0; delay < 2000000; delay++);
	  		GPIO_PORTF_DATA_R &= ~(0b00100);
	  	}

		//button_sol = GPIO_PORTF_DATA_R & 0b10000;
	  	if (button_sol == 0) {
	  		GPIO_PORTF_DATA_R |= 0b00100; // 0x02
	  		s1=0;
	  		s2=0;
	  		s3=0;
	  		s4=0;
	  		s5=0;
	  		clearLCD();
	  		setCursorPositionLCD(0,0);
	  		printLCD("secimler silindi");
	  		tmp_otomat->k_100 = 0;
	  		tmp_otomat->k_50 = 0;
	  		tmp_otomat->k_25 = 0;
	  		setCursorPositionLCD(1,0);
	  		printLCD("paraniz iade edildi");
	  		for(delay = 0; delay < 3000000; delay++);
	  		GPIO_PORTF_DATA_R &= ~(0b00100);
	  		break;
	  	}

	  	if(endChoice){
	  		//random number uret simdilik direkt true
	  		// kasa ve stok durmunu seriale yazdır
	  		// buraları test et
	  		// if rand % 4 == 2 s == false
	  		int r = rand() % 4;
	  		if(r == 2){
	  			succes = 0;
	  		} else {
		  		succes = true;
	  		}
	  		//bunları serialden yazdırınca sil
	  		/*
			clearLCD();
			setCursorPositionLCD(0,0);
			itoa(otomat->urunler[0].urun_stok,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->urunler[1].urun_stok,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->urunler[2].urun_stok,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->urunler[3].urun_stok,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->urunler[4].urun_stok,buffer,10);
			printLCD(buffer);

			setCursorPositionLCD(1,0);
			itoa(otomat->k_100,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->k_50,buffer,10);
			printLCD(buffer);

			printLCD(":");
			itoa(otomat->k_25,buffer,10);
			printLCD(buffer);
			for(delay = 0; delay < 3000000; delay++);
			*/
			//para yetmedi
			if((tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 *50) + (tmp_otomat->k_25 *25) < s1*50 + s2*100 + s3*150 + s4*175 + s5*200){
				clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("bakiye yetersiz");
				setCursorPositionLCD(1,0);
				printLCD("yeniden sec");
				for(delay = 0; delay < 3000000; delay++);
		  		s1=0;
		  		s2=0;
		  		s3=0;
		  		s4=0;
		  		s5=0;
		  		endChoice = false;
		  		continue;
	  		}

			// stok yok
	  		if(s1 > otomat->urunler[0].urun_stok || s2 > otomat->urunler[1].urun_stok || s3 > otomat->urunler[2].urun_stok || s4 > otomat->urunler[3].urun_stok || s5 > otomat->urunler[4].urun_stok){
				clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("stok yetersiz");
				setCursorPositionLCD(1,0);
				printLCD("yeniden sec");
		  		s1=0;
		  		s2=0;
		  		s3=0;
		  		s4=0;
		  		s5=0;
		  		endChoice = false;
		  		for(delay = 0; delay < 3000000; delay++);
		  		continue;
	  		}

	  		if(succes){
	  			GPIO_PORTF_DATA_R |= 0b1000;
	  			// degisiklikleri commit et ledin rengi yanlıs
	  			clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("islem basarili!");
				for(delay = 0; delay < 2000000; delay++);
				GPIO_PORTF_DATA_R &= ~(0b1000);

	  			int para_ustu = (tmp_otomat->k_100 * 100) + (tmp_otomat->k_50 * 50) + (tmp_otomat->k_25 * 25) - (s1*50 + s2*100 + s3*150 + s4*175 + s5*200);
	  			otomat->k_100 += tmp_otomat->k_100;
	  			otomat->k_50 += tmp_otomat->k_50;
	  			otomat->k_25 += tmp_otomat->k_25;

	  			otomat->urunler[0].urun_stok -= s1;
	  			otomat->urunler[1].urun_stok -= s2;
	  			otomat->urunler[2].urun_stok -= s3;
	  			otomat->urunler[3].urun_stok -= s4;
	  			otomat->urunler[4].urun_stok -= s5;

	  			// para ustu hesapla ve kasadan dus
	  			//para ustunun yetersiz oldugunu hesapla

				clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("para ustu:");
				itoa(para_ustu,buffer,10);
				printLCD(buffer);
				setCursorPositionLCD(1,0);
				if(para_ustu > (otomat->k_100 * 100) + (otomat->k_50 * 50) + (otomat->k_25 * 25)){
					printLCD("Kasada para yok");
				}
				for(delay = 0; delay < 3000000; delay++);

				int p_100=0,p_50=0,p_25=0;
				while(otomat->k_100 > 0 && para_ustu >= 100){
					para_ustu -= 100;
					p_100++;
				}
				while(otomat->k_50 > 0 && para_ustu >= 50){
					para_ustu -= 50;
					p_50++;
				}
				while(otomat->k_25 > 0 && para_ustu >= 25){
					para_ustu -= 25;
					p_25++;
				}
				clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("para ustu:");
				itoa(para_ustu,buffer,10);
				printLCD(buffer);
				setCursorPositionLCD(1,0);

				printLCD("1t:");
				itoa(p_100,buffer,10);
				printLCD(buffer);

				printLCD(" 50k:");
				itoa(p_50,buffer,10);
				printLCD(buffer);

				printLCD(" 25k:");
				itoa(p_25,buffer,10);
				printLCD(buffer);

				//yesil ledi yak
				for(delay = 0; delay < 5000000; delay++);

				otomat->k_100 -= p_100;
	  			otomat->k_50 -= p_50;
	  			otomat->k_25 -= p_25;

	  			tmp_otomat->k_100 = 0;
	  			tmp_otomat->k_50 = 0;
	  			tmp_otomat->k_25 = 0;
	  			break;
	  		} else {
	  			//0b0010 kırmızı
	  			//0b0100 mavi
	  			//0b1000 yesil
				//kırmızı ledi yak
	  			GPIO_PORTF_DATA_R |= 0b0010;
	  			clearLCD();
				setCursorPositionLCD(0,0);
				printLCD("para sikisti!");
				for(delay = 0; delay < 3000000; delay++);
				GPIO_PORTF_DATA_R &= ~(0b0010);
	  			// parayı geri ver
				tmp_otomat->k_100 = 0;
				tmp_otomat->k_50 = 0;
				tmp_otomat->k_25 = 0;
				break;

	  		}
	  	}

	  	//while 1
	}
}


otomat_t otomat;
otomat_t tmp_otomat;
char *text = "20,20,10\n1,su,30,50 Kurus\n2,cay,20,1 TL\n3,kahve,15,1.5 TL\n4,cikolata,50,1.75 TL\n5,biskuvi,100,2 TL\0";

void serialOutput(){
	int i;
}

void initText(){
	// parse text into line
	// sadece stoku init et init et
	// kurus ve tl farkını ayır
	char line[6][32];
	int i = 0;
	while(text[i] != '\0'){
		UARTprintf("%c",text[i]);
		i++;
	}
	i=0;
	UARTprintf("\n-------------\n");

	int lineCount = 0;
	int k = 0;
	int j;
	while(lineCount < 6){
		for(j = 0;text[k] != '\n';j++){
			line[lineCount][j] = text[k++];
		}
		line[lineCount][j] = '\0';
		lineCount++;
		k++;
		j=0;
	}
	//int kk=0;

	UARTprintf("=%s=\n=%s=\n=%s=\n=%s=\n=%s=\n=%s=\n",line[0],line[1],line[2],line[3],line[4],line[5]);
	//UARTprintf("%d,%d",sizeof(line[0]),sizeof(line[1]));

	int lineSize=0;
	for(i = 1; i < 6; i++){
		char token[16];
		int ccounter = 0;
		for(j = 0;line[i][j] != '\0';j++ ){
			UARTprintf("%c",line[i][j]);
			if(line[i][j] == ","){
				ccounter++;
				if(ccounter == 2){
					//BURADA KALDIM TOKENIZE BITIR SOZDE PARSE BITIR
				}
			}

		}

	}

}


int main(void) {
	//baslangicta kasa ve stok durumunu hızlıca yaz
	//gosterimi guzellestir
	//0b0010 kırmızı
	//0b0100 mavi
	//0b1000 yesil

	init_UARTstdio();
	//serialOutput();
    initLCD();
    initText();

    otomat.k_100 = 10;
    otomat.k_50 = 10;
    otomat.k_25 = 10;

    otomat.urunler[0].urun_id = 1;
    otomat.urunler[0].urun_fiyat = 50;
    otomat.urunler[0].urun_stok = 10;

    otomat.urunler[1].urun_id = 2;
    otomat.urunler[1].urun_fiyat = 100;
    otomat.urunler[1].urun_stok = 10;

    otomat.urunler[2].urun_id = 3;
    otomat.urunler[2].urun_fiyat = 150;
    otomat.urunler[2].urun_stok = 10;

    otomat.urunler[3].urun_id = 4;
    otomat.urunler[3].urun_fiyat = 170;
    otomat.urunler[3].urun_stok = 10;

    otomat.urunler[4].urun_id = 5;
    otomat.urunler[4].urun_fiyat = 200;
    otomat.urunler[4].urun_stok = 10;

	while(1) {
		//SysCtlDelay(5000000);
		getCash(&tmp_otomat);
		getChoice(&tmp_otomat,&otomat);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////

void pulseLCD()
{
	// Go Low -> High -> Low
	GPIOPinWrite(CMD_PORT_BASE, EN, 0);
	GPIOPinWrite(CMD_PORT_BASE, EN, EN);
	GPIOPinWrite(CMD_PORT_BASE, EN, 0);
}

//
// Set the RS to LOW
// Indicating incoming command
//

void setCmd() {
	GPIOPinWrite(CMD_PORT_BASE, RS,0);
}

//
// Set the RS to HIGH
// Indicating incoming data
//

void setData() {
	GPIOPinWrite(CMD_PORT_BASE, RS,RS);
}

//
// Send data byte in 4 bit mode
// High nibble(yarım bayt - dörtlu) is sent first
//

void sendByte(char byteToSend, int isData)
{
	if (isData)
		setData();
	else
		setCmd();
	SysCtlDelay(400);
	GPIOPinWrite(DATA_PORT_BASE, ALLDATAPINS, byteToSend >>4);
	pulseLCD();
	GPIOPinWrite(DATA_PORT_BASE, ALLDATAPINS, byteToSend);
	pulseLCD();
}

//
// For 16 columns, 2 rows LCD
// Select row / column for next character output
// Initial values are 0,0
//

void setCursorPositionLCD(char row, char col)
{
	char address;

	if (row == 0)
		address = 0;
	else if (row==1)
		address = 0x40;
	else if (row==2)
		address = 0x14;
	else if (row==3)
		address = 0x54;
	else
		address = 0;

	address |= col;

	sendByte(0x80 | address, FALSE);
}

//
// Clear the LCD
// and return to home position (0,0)
//

void clearLCD(void)
{
	sendByte(0x01, FALSE); // Clear screen
	sendByte(0x02, FALSE); // Back to home
	SysCtlDelay(30000);
}

//
// Return to home position (0,0)
//

void homeLCD(void) {
	sendByte(0x02, FALSE);
	SysCtlDelay(30000);
}


//
// Make block cursor visible
//

void setBlockCursorLCD(void) {
	sendByte(0x0F, FALSE);
}

//
// Make line cursor visible
//

void setLineCursorLCD(void) {
	sendByte(0x0E, FALSE);
}

//
// Display cursor on screen
//

void cursorOnLCD(void) {
	sendByte(0x0E, FALSE);
}

//
// Hide cursor from screen
//

void cursorOffLCD(void) {
	sendByte(0x0C, FALSE);
}

//
// Turn off LCD
//

void displayOffLCD(void) {
	sendByte(0x08, FALSE);
}

//
// Turn on LCD
//

void displayOnLCD(void) {
	sendByte(0x0C, FALSE);
}

//
// Initialize the LCD
// Performs the following functions:
// Activates selected ports
// Designates ports as outputs
// Pulls all output pins to low
// Waits for LCD warmup
// Sets LCD for 4 pin communication
// Sets two lines display
// Hides the cursor
// Sets insert mode
// Clears the screen
//

void initLCD(void)
{
	//
	// set the MSP pin configurations
	// and bring them to low
	//
	SysCtlPeripheralEnable(DATA_PERIPH);
	SysCtlPeripheralEnable(CMD_PERIPH);
	GPIOPinTypeGPIOOutput(DATA_PORT_BASE,  ALLDATAPINS);
	GPIOPinTypeGPIOOutput(CMD_PORT_BASE, ALLCONTROLPINS);
	GPIOPinWrite(DATA_PORT_BASE, ALLDATAPINS ,0);
	GPIOPinWrite(CMD_PORT_BASE, ALLCONTROLPINS ,0);

	//
	// wait for the LCM to warm up and reach
	// active regions. Remember MSPs can power
	// up much faster than the LCM.
	//

	SysCtlDelay(10000);

	//
	// initialize the LCM module
	// Set 4-bit input
	//

	setCmd();
	SysCtlDelay(15000);
	GPIOPinWrite(DATA_PORT_BASE, ALLDATAPINS, 0b0010);
	pulseLCD();
	GPIOPinWrite(DATA_PORT_BASE, ALLDATAPINS, 0b0010);
	pulseLCD();
	sendByte(0x28,FALSE);  // Set two lines
	cursorOffLCD();       // Cursor invisible
	sendByte(0x06, FALSE); // Set insert mode
	clearLCD();
}

//
// Print the text
// on the screen
//

void printLCD(char *text)
{
	char *c;
	c = text;

	while ((c != 0) && (*c != 0))
	{
		sendByte(*c, TRUE);
		c++;
	}
}
