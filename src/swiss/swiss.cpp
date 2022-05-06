
/*
 * SWISS Code
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "RelayLoggerComponent.h"
#include "ValcoValveLoggerComponent.h"
#include "SchedulerLoggerComponent.h"

// display (20x4 with 2 pages to visualize all the schedulers)
LoggerDisplay* lcd = new LoggerDisplay(20, 4, 2);

// controller state
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* tz */                        -6,
  /* sd_logging */                false,
  /* state_logging */             true,
  /* data_logging */              false,
  /* data_logging_period */       24*60*60, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   200, // in ms
  /* data_reading_period */       60*60*1000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "swiss 0.1",
  /* reset pin */         A5,
  /* lcd screen */        lcd,
  /* pointer to state */  controller_state
);

// relays: power, bypass, 1, 2, 3
RelayLoggerComponent* rpower = new RelayLoggerComponent(
  /* component name */        "power", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D3,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);
RelayLoggerComponent* rbypass = new RelayLoggerComponent(
  /* component name */        "bypass", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D4,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);
RelayLoggerComponent* r25cm = new RelayLoggerComponent(
  /* component name */        "25cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D5,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);
RelayLoggerComponent* r50cm = new RelayLoggerComponent(
  /* component name */        "50cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D6,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);
RelayLoggerComponent* r75cm = new RelayLoggerComponent(
  /* component name */        "75cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D7,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);

// valvo valve (RS232 port)
ValcoValveLoggerComponent* valco = new ValcoValveLoggerComponent(
  /* component name */        "valco", 
  /* pointer to controller */ controller,
  /* max positiosn */         16
);

// schedule
#define EVENT_START       1
#define EVENT_END         2
#define EVENT_CLEAN       3
#define EVENT_CLEAN25CM   4
#define EVENT_START_25CM  5
#define EVENT_END_25CM    6
#define EVENT_CLEAN50CM   7
#define EVENT_START_50CM  8
#define EVENT_END_50CM    9
#define EVENT_CLEAN75CM   10
#define EVENT_START_75CM  11
#define EVENT_END_75CM    12
#define EVENT_END_CLEAN   13
#define EVENT_END         14

const SchedulerEvent schedule[] = {
   {30,   SECONDS,      EVENT_START,      "start", "let's do this!"}, //enough time to power up mfc
   {5,    MINUTES,      EVENT_CLEAN,      "clean", "flush internal lines"}, 
   {10,   MINUTES,      EVENT_CLEAN25CM,  "clean25", "flush the 25 cm probe" },
   {45,   MINUTES,      EVENT_START_25CM, "start 25cm", "sample at 25cm depth"},
   {15,   SECONDS,      EVENT_END_25CM,   "end 25cm", "finished sampling 25"},
   {10,   MINUTES,      EVENT_CLEAN50CM,  "clean50", "flush the 50 cm probe" },
   {45,   MINUTES,      EVENT_START_50CM, "start 50cm", "sample at 50cm"},
   {15,   SECONDS,      EVENT_END_50CM,   "end 50cm"},
   {10,   MINUTES,      EVENT_CLEAN75CM,  "clean75", "flush the 75 cm probe" },
   {45    MINUTES,      EVENT_START_75CM, "start 75 cm", "sample at 75 cm"},
   {15,   SECONDS       EVENT_END_75CM,   "end 75 cm"},
   {5,    MINUTES,      EVENT_END_CLEAN,  "flush internal lines"},
   {1,    MINUTES,      EVENT_END,        "complete"}
};

const SchedulerEvent* schedule_pointer = schedule;
const int schedule_events_number = sizeof(schedule)/sizeof(schedule[0]);

// scheduler class managing thee events
class SwissScheduler : public SchedulerLoggerComponent {

  uint8_t valco_pos_25cm;
  uint8_t valco_pos_50cm;
  uint8_t valco_pos_75cm;

  public:

    SwissScheduler (const char *id, uint8_t valco_pos_25cm, uint8_t valco_pos_50cm, uint8_t valco_pos_75cm) : 
      SchedulerLoggerComponent(id, controller, SCHEDULER_DT_PATTERN_YMD_24HM, schedule_pointer, schedule_events_number), valco_pos_25cm(valco_pos_25cm), valco_pos_50cm(valco_pos_50cm), valco_pos_75cm(valco_pos_75cm) {}

    virtual void runEvent(uint8_t event) {
      if(event == EVENT_START) {
        // make sure evertything is on/off as needed
        rpower->changeRelay(true);
        rbypass->changeRelay(false);
        r25cm->changeRelay(false);
        r50cm->changeRelay(false);
        r75cm->changeRelay(false);
        valco->changePosition(1);
        // pause state saving after this to always resume at this point if power is out
        ctrl->pauseStateSaving(); 

      } else if (event == EVENT_CLEAN) {
        
        //the valco is at position 1,so just open the bypass relay to start flushing internal lines
        rbypass ->changeRelay(true);


      } else if(event == EVENT_CLEAN25CM){
        // close the bypass loop
        rbypass->changeRelay(false);  
        // turn relay on
        r25cm->changeRelay(true);
      } else if (event == EVENT_START_25CM) {
        // move valco to sampling  position
        valco->changePosition(valco_pos_25cm);
      } else if (event == EVENT_END_25CM) {
        // move valco to starting position
        valco->changePosition(1);
        // turn relay off
        r25cm->changeRelay(false);
        


      } else if(event == EVENT_CLEAN50CM){
        //double check the valco is HM
        valco->changePosition(1);
        //open the 50 cm probe
        r50cm->changeRelay(true);
      } else if (event == EVENT_START_50CM) {
        // resume state saving at this point and save that we are at this breakpoint
        ctrl->resumeStateSaving();
        saveState();
        valco->changePosition(valco_pos_50cm);
        ctrl->pauseStateSaving();
      } else if (event == EVENT_END_50CM) {
        // move valco to starting position
        valco->changePosition(1);
        // turn relay off
        r50cm->changeRelay(false);



      } else if (event == EVENT_CLEAN75CM){
        //double check the valco is HM
        valco->changePosition(1);
        //open the 75 cm probe
        r75cm->changeRelay(true);
      } else if (event == EVENT_START_75CM) {
        // resume state saving at this point and save that we are at this breakpoint
        ctrl->resumeStateSaving();
        saveState();
        valco->changePosition(valco_pos_75cm);
        ctrl->pauseStateSaving();
      } else if (event == EVENT_END_75CM) {
        // move valco to starting position
        valco->changePosition(1);
        // turn relay off
        r75cm->changeRelay(false);


      } else if (event == EVENT_END_CLEAN){
        // move valco to starting position
        valco->changePosition(1);
        //open bypass valve
        rbypass->changeRelay(true);
      }


      } else if (event == EVENT_END) {
        //clse the bypass valve
        rbypass->changeRelay(true);

        //save where we're at
        ctrl->resumeStateSaving();
        saveState();
        // turn power back off
        rpower->changeRelay(false);
      }
    }
    
};

// schedulers: 1, 2, 3, 4, 5
SwissScheduler* scheduler1 = new SwissScheduler("s1", 2, 3, 4);
SwissScheduler* scheduler2 = new SwissScheduler("s2", 5, 6, 7);
SwissScheduler* scheduler3 = new SwissScheduler("s3", 8, 9, 10);
SwissScheduler* scheduler4 = new SwissScheduler("s4", 11, 12, 13);
SwissScheduler* scheduler5 = new SwissScheduler("s5", 14, 15, 16);

// lcd update callback function
void lcd_update_callback() {
  lcd->resetBuffer();
  if (lcd->getCurrentPage() == 1) {
    scheduler1->getSchedulerStatus(lcd->buffer, 21);
    lcd->printLineFromBuffer(2);
    scheduler2->getSchedulerStatus(lcd->buffer, 21);
    lcd->printLineFromBuffer(3);
    scheduler3->getSchedulerStatus(lcd->buffer, 21);
    lcd->printLineFromBuffer(4);
  } else if (lcd->getCurrentPage() == 2) {
    scheduler4->getSchedulerStatus(lcd->buffer, 21);
    lcd->printLineFromBuffer(2);
    scheduler5->getSchedulerStatus(lcd->buffer, 21);
    lcd->printLineFromBuffer(3);
    lcd->printLine(4, "");
  }
}

// manual wifi management
SYSTEM_THREAD(ENABLED);
SYSTEM_MODE(MANUAL);

void setup() {

  // turn communications module on
  #if PLATFORM_ID == PLATFORM_PHOTON || PLATFORM_ID == PLATFORM_ARGON
  WiFi.on();
  #elif PLATFORM_ID == PLATFORM_BORON
  Cellular.on();
  #endif

  // serial
  Serial.begin(9600);
  delay(1000);

  // debugging flags
  //controller->forceReset();
  //controller->debugDisplay();
  //controller->debugData();
  //controller->debugState();
  //controller->debugCloud();
  //controller->debugWebhooks();

  // add components
  controller->addComponent(valco);
  controller->addComponent(rpower);
  controller->addComponent(rbypass);
  controller->addComponent(r25cm);
  controller->addComponent(r50cm);
  controller->addComponent(r75cm);
  
  // add schedulers
  controller->addComponent(scheduler1);
  controller->addComponent(scheduler2);
  controller->addComponent(scheduler3);
  controller->addComponent(scheduler4);
  controller->addComponent(scheduler5);

  // controller
  controller->init();
}


long lcd_update = 0;

void loop() {
  controller->update();
  // update once a second for scheduling timers
  if (millis() - lcd_update > 1000) {
    lcd_update = millis();
    lcd_update_callback();
  }
}
