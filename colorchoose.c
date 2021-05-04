#include <wiringPi.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <softPwm.h>
#include <lcd.h>
#include <string.h>


#define RGBred    27
#define RGBgreen  29
#define RGBblue   28

#define ADCMAX 255
#define ADCMIN 0

#define     ADC_CS    25
#define     ADC_DIO   23
#define     ADC_CLK   24

#define CAMBUT    6 //active LOW
#define COLORBUT  26 //active LOW


//ouputs the RGB values to the LCD display 
//rVal, bVal, and gVal are ints in the range 0-255
//comingFrom is an integer representation of the function that called displayStats
    //this is used so that the display only has to update 3 characters instead of 32
//fd is the integer representation of the LCD display
void displayStats(int fd, int rVal, int gVal, int bVal, int comingFrom){
    char line1 [16];
    char line2 [16];

    char r[6];
    char g[6];
    char b[6];


    strcpy(line1, "R:       B:    ");
    strcpy(line2, "     G:        ");

    snprintf(r, 6, "%3d", rVal);
    snprintf(g, 6, "%3d", gVal);
    snprintf(b, 6, "%3d", bVal);

    lcdPosition(fd, 0, 0);
    lcdPuts(fd, line1);
    lcdPosition(fd, 0, 1);
    lcdPuts(fd, line2);

    
    switch (comingFrom){
        case 0:
            //coming from red adjustment
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (3+i), 0);
                lcdPutchar(fd, *(r + i));
            }
        case 1:
            //coming from green adjustment
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (8+i), 1);
                lcdPutchar(fd, *(g + i));
            }
        case 2:
            //coming from blue adjustment
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (12+i), 0);
                lcdPutchar(fd, *(b + i));
            }
        case 3:
            //coming from picture
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (3+i), 0);
                lcdPutchar(fd, *(r + i));
            }
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (8+i), 1);
                lcdPutchar(fd, *(g + i));
            }
            for(int i = 0; i < 3; i++){
                lcdPosition(fd, (12+i), 0);
                lcdPutchar(fd, *(b + i));
            }
            

    }

}

//gets the value of the color adjustment potentiometer
unsigned char get_ADC_Result(void)
{
	unsigned char i;
	unsigned char dat1=0, dat2=0;

	digitalWrite(ADC_CS, 0);

	digitalWrite(ADC_CLK,0);
	digitalWrite(ADC_DIO,1);	delayMicroseconds(2);
	digitalWrite(ADC_CLK,1);	delayMicroseconds(2);
	digitalWrite(ADC_CLK,0);

	digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
	digitalWrite(ADC_CLK,1);	delayMicroseconds(2);
	digitalWrite(ADC_CLK,0);

	digitalWrite(ADC_DIO,0);	delayMicroseconds(2);

	digitalWrite(ADC_CLK,1);	
	digitalWrite(ADC_DIO,1);    delayMicroseconds(2);
	digitalWrite(ADC_CLK,0);	
	digitalWrite(ADC_DIO,1);    delayMicroseconds(2);

	for(i=0;i<8;i++)
	{
		digitalWrite(ADC_CLK,1);	delayMicroseconds(2);
		digitalWrite(ADC_CLK,0);    delayMicroseconds(2);

		pinMode(ADC_DIO, INPUT);
		dat1=dat1<<1 | digitalRead(ADC_DIO);
	}

	for(i=0;i<8;i++)
	{
		dat2 = dat2 | ((unsigned char)(digitalRead(ADC_DIO))<<i);
		digitalWrite(ADC_CLK,1); 	delayMicroseconds(2);
		digitalWrite(ADC_CLK,0);    delayMicroseconds(2);
	}

	digitalWrite(ADC_CS,1);

	pinMode(ADC_DIO, OUTPUT);

	return(dat1==dat2) ? dat1 : 0;
}

//maps x from in_min-in_max range to out_min-outmax range (not really necessary, but it's here anyways)
int map(int x, int in_min, int in_max, int out_min, int out_max){  
	return (x -in_min) * (out_max - out_min) / (in_max - in_min) + out_min;  
}  

//writes the color to the RGB led
//r,g,b are in range 0 to 255
void writeColor(int r, int g, int b){
    softPwmWrite(RGBred, 255-r);
    softPwmWrite(RGBgreen, 255-g);
    softPwmWrite(RGBblue, 255-b);
}

//calls colordetect.py to take a picture, and save the color data for the   
    //dominant color in colordata.csv
//reads colordata.csv and stores values for R,G,B at the addresses that hold r,g, and b values
//returns COLORBUT+2 to signify that the next function called by main() should be adjustRed
//!CURRENT LIMITATION! cannot take a picture immediately after taking picture,
    //must adjust value for at leat 1 color before calling takePic again
int takePic(int fd, int* rVal, int* bVal, int* gVal){
    //run colordetect.py
        system("raspistill -o source.png -w 720 -h 480 -q 10");
        system("python colordetect.py");

    //read colordata.csv
        FILE *fp;
        char rbuf[4];
        char gbuf[4];
        char bbuf[4];

        int Rint = 0;
        int Gint = 0;
        int Bint = 0;

        fp = fopen("colordata.csv", "r");
        fscanf(fp, "%s", rbuf);
        fscanf(fp, "%s", gbuf);
        fscanf(fp, "%s", bbuf);

        Rint = atoi(&rbuf[0]);
        Gint = atoi (&gbuf[0]);
        Bint = atoi(&bbuf[0]);
        *rVal = Rint;
        *gVal = Gint;
        *bVal = Bint;

    writeColor(*rVal, *gVal, *bVal);
    displayStats(fd, *rVal, *gVal, *bVal, 3);
    while (digitalRead(COLORBUT) == HIGH){
        delay(10);
    }
    while (digitalRead(COLORBUT) == LOW){
        delay(10);
    }

    return(COLORBUT+2);
}

//calls get_ADC_result to read the value of the potentiometer and adjusts the rVal value to match it.
//Continues to run until CAMBUT or COLORBUT is pressed
//returns COLORBUT if colorbutton was pressed, or returns CAMBUT if cam button was pressed
int adjustRed(int fd, int* rVal, int* bVal, int* gVal){
    int nextState;
    while ((digitalRead(COLORBUT) != LOW) && (digitalRead(CAMBUT) != LOW)){
        *rVal = get_ADC_Result();
        displayStats(fd, *rVal, *bVal, *gVal, 0);
        writeColor(*rVal, *bVal, *gVal);
    }
    while ((digitalRead(COLORBUT) == LOW) || (digitalRead(CAMBUT) == LOW)){
        if(digitalRead(COLORBUT) == LOW){nextState = COLORBUT + 0;}
        if(digitalRead(CAMBUT) == LOW){nextState = CAMBUT;}
        delay(10);
    }
    return (nextState);


}

//calls get_ADC_result to read the value of the potentiometer and adjusts the gVal value to match it.
//Continues to run until CAMBUT or COLORBUT is pressed
//returns COLORBUT if colorbutton was pressed, or returns CAMBUT if cam button was pressed
int adjustGreen(int fd, int* rVal, int* bVal, int* gVal){
    int nextState;
    while ((digitalRead(COLORBUT) != LOW) && (digitalRead(CAMBUT) != LOW)){
        *gVal = get_ADC_Result();
        displayStats(fd, *rVal, *gVal, *bVal, 1);
        writeColor(*rVal, *bVal, *gVal);
    }
    while ((digitalRead(COLORBUT) == LOW) || (digitalRead(CAMBUT) == LOW)){
        if(digitalRead(COLORBUT) == LOW){nextState = COLORBUT + 1;}
        if(digitalRead(CAMBUT) == LOW){nextState = CAMBUT;}
        delay(10);
    }
    return (nextState);


}

//calls get_ADC_result to read the value of the potentiometer and adjusts the bVal value to match it.
//Continues to run until CAMBUT or COLORBUT is pressed
//returns COLORBUT if colorbutton was pressed, or returns CAMBUT if cam button was pressed
int adjustBlue(int fd, int* rVal, int* bVal, int* gVal){
    int nextState;
    while ((digitalRead(COLORBUT) != LOW) && (digitalRead(CAMBUT) != LOW)){
        *bVal = get_ADC_Result();
        displayStats(fd, *rVal, *gVal, *bVal, 2);
        writeColor(*rVal, *bVal, *gVal);
    }
    while ((digitalRead(COLORBUT) == LOW) || (digitalRead(CAMBUT) == LOW)){
        if(digitalRead(COLORBUT) == LOW){nextState = COLORBUT + 2;}
        if(digitalRead(CAMBUT) == LOW){nextState = CAMBUT;}
        delay(10);
    }
    return (nextState);


}
 
//set up calls wiringPiSetup(), creates the softPWM signals, 
    //initializes the RBGLED to white, and calls lcdinit()
int setup (){
    int fd = 0;
    if (wiringPiSetup() == -1){
        printf("wiringpi setup failing!\n");
        return(0);
    }
    else{
        for (int i = 27; i<30; i++){
            pinMode(CAMBUT, INPUT);
            pinMode(COLORBUT, INPUT);


            softPwmCreate(RGBred,  0, 255);  //create a soft pwm
	        softPwmCreate(RGBgreen,0, 255);  
	        softPwmCreate(RGBblue, 0, 255);  
            softPwmWrite(RGBred, 255);
            softPwmWrite(RGBgreen, 255);
            softPwmWrite(RGBblue, 255);

               //lcdInit(rows, columns, bits width/char, rs GPIO pin, E(strobe) GPIO pin, GPIO data pins for LCD )
            fd = lcdInit(2,16,4, 5,4,  0,1,2,3,0,0,0,0); //see /usr/local/include/lcd.h
            if(fd == -1){
                printf("lcd setup failed!\n");
                return(0);
            }
            else{
                return(fd);
            }
        }
        return (1);
    }

}


int main (){
    int nextState = -1;
    int r = 255;
    int g = 255;
    int b = 255;

    int lcd = setup();
    //if lcd == -1, setup failed
    if (lcd == -1){
        return -1;
    }

    else{
        
        lcdClear(lcd);                          //clear the LCD to initialize it as blank
        //start out by adjusting red, and the next state will be either adjusting green or taking a picture
        nextState = adjustRed(lcd, &r, &g, &b); 
        while(1){
            //nextState = COLORBUT means the program just returned from adjustRed()
            if(nextState == COLORBUT){
                nextState = adjustGreen(lcd, &r, &g, &b);
            }
            //nextState = COLORBUT+1 means the program just returned from adjustGreen()
            else if(nextState == (COLORBUT + 1)){
                nextState = adjustBlue(lcd, &r, &g, &b);
            }
            //nextState = COLORBUT+2 means the program just returned from adjustblue()
            else if(nextState == (COLORBUT + 2)){
                nextState = adjustRed(lcd, &r, &g, &b);
            }
            //nextState = CAMBUT means the program just returned from an adjustrgb() 
                //function and the camera button was pressed
            else if(nextState == CAMBUT){
                nextState = takePic(lcd, &r, &g, &b);
            }
            else{
                printf("could not determine the next state!\n");
                return (-1);
            }
        }
    }
    
    
}
