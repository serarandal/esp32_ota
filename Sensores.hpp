#include "Seeed_BME280.h"
#include <Wire.h>
#include <MQUnifiedsensor.h>
#include <DHT.h>

// Definimos el pin digital donde se conecta el sensor
#define DHTPIN 2
// Dependiendo del tipo de sensor
#define DHTTYPE DHT11
// Inicializamos el sensor DHT11
#define placa "ESP-32"
#define Voltage_Resolution 3.3
#define pin 34
#define type "MQ-135"
#define ADC_Bit_Resolution 12
#define RatioMQ135CleanAir 3.6 

DHT dht(DHTPIN, DHTTYPE);
BME280 bme280;
double CO2 = (0);  
MQUnifiedsensor MQ135(placa, Voltage_Resolution, ADC_Bit_Resolution, pin, type);

void init_Sensores()
{
	dht.begin();
  if (!bme280.init()) {
        Serial.println("Device error!");
    }
  MQ135.setRegressionMethod(1); //_PPM =  a*ratio^b   
  MQ135.setA(110.47); 
  MQ135.setB(-2.862); 
  // Configurate the ecuation values to get NH4 concentration    
  MQ135.init();    
  Serial.print("Calibrating please wait.");   
  float calcR0 = 0;   
  for(int i = 1; i<=10; i ++)   {     
    MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin     
    calcR0 += MQ135.calibrate(RatioMQ135CleanAir);    
    Serial.print(".");   
    }   
  MQ135.setR0(calcR0/10);   
  Serial.println("  done!.");      
  if(isinf(calcR0)) { Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply"); while(1);}   
  if(calcR0 == 0){Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply"); while(1);}   
  /*****************************  MQ CAlibration **************************/                   
  MQ135.serialDebug(false); 
}

String get_Data()
{

  float pressure;
 String Data;
  // Leemos la humedad relativa
  float h = dht.readHumidity();
  // Leemos la temperatura en grados centígrados (por defecto)
  float t = dht.readTemperature();
  // Leemos la temperatura en grados Fahrenheit
  //float f = dht.readTemperature(true);
  Data = Data + "DHT_Humidity:"+String(h)+"DHT_Temperature"+String(t);
 
 
  // Calcular el índice de calor en Fahrenheit
  //float hif = dht.computeHeatIndex(f, h);
  // Calcular el índice de calor en grados centígrados
  //float hic = dht.computeHeatIndex(t, h, false);
  
  //get and print temperatures
  Data = Data + "BME_Temperature"+bme280.getTemperature();//Celsius

  //get and print atmospheric pressure data
  pressure = bme280.getPressure();
  Data = Data + "BME_Pressure"+bme280.getPressure();//Pascales
  
  //get and print altitude data
  Data = Data + "BME_Altitude"+bme280.calcAltitude(pressure);//meters
  

  //get and print humidity data
  Data = Data + "BME_Humidity"+ bme280.getHumidity() + "%";//%

 
  MQ135.update(); // Update data, the arduino will be read the voltage on the analog pin   
  CO2 = MQ135.readSensor(); // Sensor will read CO2 concentration using the model and a and b values setted before or in the setup   
  Data = Data + "CO2" + CO2;
  Serial.println(Data);
	return Data ;
}

float getDhtHumidity()
{
	return dht.readHumidity();
}


float getDhtTemperature()
{
	return dht.readTemperature();
}

float getBmeTemperature()
{
	return bme280.getTemperature();
}

float getBmePressure()
{
	return bme280.getPressure();
}

float getBmeAltitude()
{
  float pressure = 0;
	pressure = bme280.getPressure();
	return bme280.calcAltitude(pressure);
}

float getBmeHumidity()
{
	return bme280.getHumidity();
}

float getCO2()
{
	MQ135.update();    
  return MQ135.readSensor();
}
