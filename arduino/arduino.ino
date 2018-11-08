/*
 * ProyectoFinal del curso de IoT.
 * Consistira en una appWeb que podrá enviar una configuracion a Arduino para que este muestree
 * temperatura o luminosidad además de diferentes tiempos de muestreo y salidas.
 * Considerar pasar los valores de voltaje a lumenes y temperatura en grados centigrados con las funciones del campus virtual.
 * 
 * Arduino estará comunicado a python mediante la libreria PySerial.
 * Python trasnformara estos mensajes en un json y los enviará por protocolo MQTT a la nube de Google.
 * En la nube los datos se almacenarán en gcloudDatastore.
 * Con una funcion de gcloudFunction trataremos dichos datos para ser visualizados en nuestra appWeb.
 * La appWeb se creará en ECLIPSE y se subirá a gcloudAppEngine.
 */
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

// Declaracion de variables globales y definiciones:
// Estados del sistema:
const int NOCONTROL = 0;
const int LUZCONTROL = 1;
const int TEMPCONTROL = 2;
const int ALLCONTROL = 3;

// PINES
const byte tempPin = A0;
const byte luzPin = A1;
const byte releluzPin = 2;
const byte reletempPin = 3;

// PERIODOS
const unsigned long anPer = 2000;
const unsigned long monitorPer = 5000;
const unsigned long sistemaPer = 5000;
const unsigned long relePer = 5000;

//UMBRALES
const float UMBRALLUZ = 400;
const float UMBRALTEMP = 740;
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

//Entrada Analogica Periodica
struct EntradaAnalogica{
  byte pin;
  float estado;
  unsigned long perms;
  unsigned long lastms;
};
void setup_EntradaAnalogica (struct EntradaAnalogica& e, byte pin, unsigned long perms, unsigned long currms) {
  e.pin = pin;
  e.perms = perms;
  e.lastms = currms - e.perms;
  pinMode(e.pin, INPUT);
  e.estado = analogRead(e.pin);
}

void loop_EntradaAnalogica (struct EntradaAnalogica& e, unsigned long currms) {
  if (currms - e.lastms >= e.perms) {
    e.lastms += e.perms;
    e.estado = analogRead(e.pin);
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

/*
 * Monitor Periodico: previo al sistema. Este va a enviar las dos entradas independientemente de que configuracion permanezca
 * por lo que para facilidad del codigo lo situamos antes en la escala de flujo.
 * 
 * El monitor ademas recibirá los umbrales de temperatura y lumninosidad que posteriormente le dara al Sistema() por lo que se deben incluir como variables.
 */
struct Monitor{
  unsigned long perms;
  unsigned long lastms;

  char mensaje;
  float umbralTemp;
  float umbralLuz;
};
void setup_Monitor (struct Monitor& m, unsigned long perms, unsigned long currms) {
  Serial.begin(9600);
  m.perms = perms;
  m.lastms = currms - m.perms;  
  m.umbralLuz = UMBRALLUZ;
  m.umbralTemp = UMBRALTEMP;
}

void loop_Monitor(struct Monitor& m, const struct EntradaAnalogica& eluz, const struct EntradaAnalogica& etemp, unsigned long currms) {
  if(currms - m.lastms >= m.perms) {
    //blink(3);
    m.lastms += m.perms;
    
    // RECIBE DATOS
    if (Serial.available()>0) {
      blink(4);
      m.mensaje = Serial.read();      
    }
        
    // ENVIA DATOS: temperatura y luminosidad.     
    Serial.println(eluz.estado);
    Serial.println(etemp.estado);
  }
  
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

// SISTEMA periodico
/*
 * Al sistema le entra un caracter leido por el puerto serial que determinará su estado.
 * También leerá los dos umbrales de temperatura y de luz.
 * Tambien entraran los estados analogicos de los sensores.
 * 
 * En funcion de los umbrales y los valores de los sensores se activarán o desactivaran las variables booleanas luzencender y tempemcender.
 */
struct Sistema {
  int estado;
  int estadoant;
  unsigned long cambioms;
  unsigned long perms;
  unsigned long lastms;

  boolean luzencender;
  boolean tempencender;
};
void setup_Sistema(struct Sistema& s, unsigned long perms, unsigned long currms){
  s.estado = NOCONTROL;
  s.perms = perms;
  s.lastms = currms - s.perms;
  s.luzencender = false;
  s.tempencender = false;
}

void loop_Sistema(struct Sistema& s, const struct EntradaAnalogica& eluz, const struct EntradaAnalogica& etemp, const struct Monitor& m, unsigned long currms){
  if(currms - s.lastms >= s.perms) {
    s.lastms += s.perms;
    s.estadoant = s.estado;

    if(eluz.estado < m.umbralLuz){
      s.luzencender = true;
    } else {
      s.luzencender = false;
    }
    if(etemp.estado < m.umbralTemp){
      s.tempencender = true;
    } else {
      s.tempencender = false;
    }

    // El estado solo va a cambiar dependiendo de un caracter que envia el monitor.
    switch (m.mensaje) {
      case 'a':
        s.estado = NOCONTROL;
      break;
      case 'b':
        s.estado = LUZCONTROL;
      break;
      case 'c':
        s.estado = TEMPCONTROL;
      break;
      case 'd':
        s.estado = ALLCONTROL;
      break;
    }
    //s.estado = m.mensaje; Terminariamos más rapido así si s.estado fuese un char.
    
    if(s.estadoant != s.estado) {
      s.cambioms = currms;
    }
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

//RELE Temperatura Periodica.
struct ReleTemp{
  byte pin;
  byte estado;
  unsigned long perms;
  unsigned long lastms;
  unsigned long cambioms;
};
void setup_ReleTemp(struct ReleTemp& r, byte pin, unsigned long perms, unsigned long currms) {
  r.pin = pin;
  r.perms = perms;
  r.lastms = currms - r.perms;
  pinMode(r.pin, OUTPUT);
  r.estado = 0;
  digitalWrite(r.pin, r.estado);
}

void loop_ReleTemp(struct ReleTemp& r, const struct Sistema& sis, unsigned long currms) {  
  if(currms - r.lastms >= r.perms) {
    r.lastms += r.perms;
    if((sis.estado == ALLCONTROL || sis.estado == TEMPCONTROL) && sis.tempencender == true) {
      r.estado = HIGH;
      r.cambioms = currms;
    } else {
      r.estado = LOW;
      r.cambioms = currms;
    }
  }
  if(r.cambioms == currms){
    digitalWrite(r.pin, r.estado);
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

//RELE Luz Periodica.
struct ReleLuz{
  byte pin;
  byte estado;
  unsigned long perms;
  unsigned long lastms;
  unsigned long cambioms;
};
void setup_ReleLuz(struct ReleLuz& r, byte pin, unsigned long perms, unsigned long currms) {
  r.pin = pin;
  r.perms = perms;
  r.lastms = currms - r.perms;
  
  pinMode(r.pin, OUTPUT);
  r.estado = 0;
  digitalWrite(r.pin, r.estado);
}

void loop_ReleLuz(struct ReleLuz& r, const struct Sistema& sis, unsigned long currms) {
  if(currms - r.lastms >= r.perms) {
    r.lastms += r.perms;
    if((sis.estado == ALLCONTROL || sis.estado == LUZCONTROL) && sis.luzencender == true) {
      r.estado = HIGH;
      r.cambioms = currms;
    } else {
      r.estado = LOW;
      r.cambioms = currms;
    }
  }
  if(r.cambioms == currms){
    digitalWrite(r.pin, r.estado);
  }
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

struct EntradaAnalogica sensorT, sensorL;
struct Monitor monitor;
struct Sistema sistema;
struct ReleLuz releluz;
struct ReleTemp reletemp;
//-----------------------------------------------------------------------------------------------------------------------------------------------------------

void setup() {
  // put your setup code here, to run once:
  pinMode(13, OUTPUT);
  unsigned long currms = millis();
  //setup_EntradaAnalogica(sensorT, tempPin, anPer, currms);
  setup_EntradaAnalogica(sensorL, luzPin, anPer, currms);
  setup_EntradaAnalogica(sensorT, tempPin, anPer, currms);
  setup_Monitor(monitor, monitorPer, currms);
  setup_Sistema(sistema, sistemaPer, currms);
  setup_ReleLuz(releluz, releluzPin, relePer, currms);
  setup_ReleTemp(reletemp, reletempPin, relePer, currms);
}

void loop() {
  // put your main code here, to run repeatedly:
  unsigned long currms = millis();
  //loop_EntradaAnalogica(sensorT, currms);
  loop_EntradaAnalogica(sensorL, currms);
  loop_EntradaAnalogica(sensorT, currms);
  loop_Monitor(monitor, sensorL, sensorT, currms);
  loop_Sistema(sistema, sensorL, sensorT, monitor, currms);
  loop_ReleLuz(releluz, sistema, currms);
  loop_ReleTemp(reletemp, sistema, currms);
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------------------------------------------------------------





// funciones para depurar usando leds
void blink(unsigned int numberTimes) {
  blink(numberTimes, 100);
}

void blink(unsigned int numberTimes, int delayT) {
  for ( int i = 0; i < numberTimes; i++) {
    digitalWrite(13, HIGH);   // turn the LED on (HIGH is the voltage level)
    delay(delayT);              // wait for a second
    digitalWrite(13, LOW);    // turn the LED off by making the voltage LOW
    delay(delayT);
  }
}
