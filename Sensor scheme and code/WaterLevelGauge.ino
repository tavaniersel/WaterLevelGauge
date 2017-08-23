SYSTEM_MODE(SEMI_AUTOMATIC);
 
SYSTEM_THREAD(ENABLED);

STARTUP(cellular_credentials_set("Internet", "", "", NULL));
 
// This #include statement was automatically added by the Particle IDE.
#include <math.h>
#define SERIALDEBUG TRUE 
 
const int trigPin = B0;
const int echoPin = A3;
const int buttonPin = A1;
const int ThermistorPin = A2;                       
 
// Constantes
const float R_balance = 9700.0;            // measured value of the 10kohm resistance of the balance resistor
const int nummerSamples = 5;               // 5 samples temperature
const float Thermistor_Nominal = 10000.0;  // resistance at 25 degrees C van de Thermistor  
const int TemperatureNominal = 25.0;       // temp. for nominal resistance (almost always 25 C) 
const int connectCloudTime = 60000;        // in milliseconds, time allowed to connect to Cloud once connected to GSM.
const int uploadNr = 5;                    // nr of measurements before everything is uploaded.
const int sleepTime = 120;                 // seconds of sleep between measurements.
 
// Variabels
int thermistorValue;                      // de op A4 gemeten waarde tussen 0- komt lineair overeen met een waarde tussen 0-5V           
float Resistance;                         // de gemeten weerstand op de Thermistor
float Temp_S;                             // is de temperatuur maar ook tussentijds een tijdelijke variabele voor de steinhart-hart formule
double Temp_Sd;
float duration;
float distance;
double distanced;
float level;
double leveld;
 
double distanceArray[uploadNr];                     //will contain last uploadNr distance measurements in cm.
double temperatureArray[uploadNr];                  //will contain last uploadNr temperature measurements in hundreds of degrees.
double levelArray[uploadNr];                        //will contain last uploadNr level in feet.
int switchState = 0;
int currentSampleCounter = 0;
int counter = 0;                   


// SET UP
void setup() {
    if (SERIALDEBUG) Serial.begin(9600);
    pinMode(trigPin, OUTPUT);
    pinMode(echoPin, INPUT);
    pinMode(buttonPin, INPUT_PULLDOWN);
    Serial1.begin(9600);
    delay(250);
    
    Particle.variable("distance", distanced);
    Particle.variable("temperature", Temp_Sd);
    Particle.variable("level", leveld);
}
 
// LOOP
void loop() {
    
// print on SD like level:distance:temperature:Sec from january 1 1970
    Serial1.print(level);
    Serial1.print(":"); 
    Serial1.print(distance);
    Serial1.print(":"); 
    Serial1.print(Temp_S);
    Serial1.print(":"); 
    Serial1.println(Time.now()); 
    delay(1000);
    
//    
    temperatuurMeten();
    afstand_meten();
    level_meten();
    
    if (SERIALDEBUG) print_info();
   
    currentSampleCounter=currentSampleCounter+1;
    
    if (currentSampleCounter == uploadNr) {
        if (SERIALDEBUG) Serial.println("publishing to Spark Cloud");        
        String tempString = "";
        String distString = "";
        String levelString = "";
        for (int i = 0; i<uploadNr;i++){
            tempString += ((String) temperatureArray[i] + ",");
            distString += ((String) distanceArray[i] + ",");
            levelString += ((String) levelArray[i] + ",");
        }
        
        publish2Cloud(tempString,distString,levelString);
    }
    System.sleep(buttonPin,RISING,sleepTime);
 }
 
 void afstand_meten() {
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(100);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  
//if t is true

 if (Temp_S < -10) {//when temp is below -10 degrees
  distance = (duration/2.0) / ((1/(20.0*sqrt(273.0+30.0)))*1000000.0/100);
  distanced = double(distance);
  distanceArray[currentSampleCounter] = (double) ( (distance ));             
  }
  else if (Temp_S > 60) { //when temp is above 60 degrees
  distance = (duration/2.0) / ((1/(20.0*sqrt(273.0+30.0)))*1000000.0/100);
  distanced = double(distance);
  distanceArray[currentSampleCounter] = (double) ( (distance ));             
  }
  
  else {//when between -10 and 60 degrees
  distance = (duration/2.0) / ((1/(20.0*sqrt(273.0+Temp_S)))*1000000.0/100);
  distanced = double(distance);
  distanceArray[currentSampleCounter] = (double) ( (distance ));             
  }  

} 

void level_meten() {
  level = 182.5 - (distance);        // reference point here 185.5 cm 
  leveld = double(level);
  levelArray[currentSampleCounter] = (double) ( ( level));                   
}

void temperatuurMeten(){
    thermistorValue = 0;                                // thermistorValue = 0
    for(int i=0;i<nummerSamples; i++){                  // 5 samples from the thermistor for index 0, 1, 2, 3 en 4
        thermistorValue += analogRead(ThermistorPin);   
        delay(10);                               
    }

    thermistorValue /= nummerSamples;                   // calculating average     
   
    Resistance=R_balance/((4095.0 / (float) thermistorValue) - 1.0);    // is the known resistance of the Thermistor 
    // temperature according to Steinhart-Hart equation degrees Celcius
    Temp_S = log(Resistance);                                   
    Temp_S = 1.0 / (0.006599983 + (-0.0003961780534 * Temp_S) + ( 0.000000515755081900  * Temp_S * Temp_S * Temp_S));
    Temp_S = Temp_S - 273.15;
    Temp_Sd=double(Temp_S);
    temperatureArray[currentSampleCounter] = (double) ( ( Temp_S));
}
 
// printing measurements on serial monitor
void print_info(){
    Serial.print("afstand is ");        
    Serial.print(distance,2);          
    Serial.println(" cm");     
    Serial.println("");
    Serial.print("Average Thermistor value: ");
    Serial.println(thermistorValue);
    Serial.print("Resistance: ");
    Serial.print(Resistance);
    Serial.println(" Ohm");
    Serial.print("Temperature by Steinhart-Hart formule ");
    Serial.print(Temp_S,2);
    Serial.println(" *C");
}
 
// publish data
void publish2Cloud(String tempString, String distString, String levelString){
 
    //connect, publish, disconnect
    Cellular.on();
    Particle.connect();
    waitFor(Particle.connected, connectCloudTime);
    if (Particle.connected()){
        Spark.publish("Temperature",tempString);         
        Spark.publish("Distance",distString);            
        Spark.publish("Waterlevel",levelString);        
        Particle.process();
        //wait a second to allow the data to be sent.
        delay(1000);
        currentSampleCounter = 0;
    }
    else{
        if (SERIALDEBUG) Serial.println("did not connect to Particle Cloud.");
    }
    Cellular.off();
    delay(1000);
}
