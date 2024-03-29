
/*
 * SWISS Code
 */
#pragma SPARK_NO_PREPROCESSOR // disable spark preprocssor to avoid issues with callbacks

#include "application.h"
#include "LoggerController.h"
#include "LoggerDisplay.h"
#include "RelayLoggerComponent.h"
#include "ValcoValveLoggerComponent.h"
#include "SchedulerLoggerComponent.h"

// controller state (default)
LoggerControllerState* controller_state = new LoggerControllerState(
  /* locked */                    false,
  /* tz */                        -6,
  /* sd_logging */                true,
  /* state_logging */             false,
  /* data_logging */              false, // turn data logging off by default
  /* data_logging_period */       24*60*60, // in seconds
  /* data_logging_type */         LOG_BY_TIME,
  /* data_reading_period_min */   200, // in ms
  /* data_reading_period */       60*60*1000  // in ms
);

// controller
LoggerController* controller = new LoggerController(
  /* version */           "swiss 0.3",
  /* reset pin */         A0,
  /* pointer to state */  controller_state,
  /* enable sd */         true
);


// display (20x4 with 2 pages to visualize all the schedulers)
LoggerDisplay* lcd = new LoggerDisplay(
  /* pointer to controller */ controller, 
  /* lcd cols */              16,//20, 
  /* lcd rows */              2,//4, 
  /* lcd pages */             2
);

// relays: power, bypass, 1, 2, 3
RelayLoggerComponent* rpower = new RelayLoggerComponent(
  /* component name */        "power", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D2,
  /* normally open/closed */  RELAY_NORMALLY_OPEN
);
RelayLoggerComponent* rbypass = new RelayLoggerComponent(
  /* component name */        "bypass", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D4,
  /* normally open/closed */  RELAY_NORMALLY_CLOSED
);
RelayLoggerComponent* r25cm = new RelayLoggerComponent(
  /* component name */        "25cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D5,
  /* normally open/closed */  RELAY_NORMALLY_CLOSED
);
RelayLoggerComponent* r50cm = new RelayLoggerComponent(
  /* component name */        "50cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D6,
  /* normally open/closed */  RELAY_NORMALLY_CLOSED
);
RelayLoggerComponent* r75cm = new RelayLoggerComponent(
  /* component name */        "75cm", 
  /* pointer to controller */ controller,
  /* state on/off */          false,
  /* relay pin */             D7,
  /* normally open/closed */  RELAY_NORMALLY_CLOSED
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
const SchedulerEvent schedule[] = {
   {10,   SECONDS,      EVENT_START,      "start", "let's do this!"}, //enough time to power up mfc
   {10,   SECONDS,      EVENT_CLEAN,      "flush internal", "flush internal lines"}, 
   {15,   MINUTES,      EVENT_CLEAN25CM,  "flush probe", "flush the 25 cm probe" },
   {10,   MINUTES,      EVENT_START_25CM, "flush flask", "sample at 25cm depth"},
   {45,   MINUTES,      EVENT_END_25CM,   "END! 25cm", "finished sampling 25"},
   {1,    MINUTES,      EVENT_CLEAN50CM,  "flush probe", "flush the 50 cm probe" },
   {10,   MINUTES,      EVENT_START_50CM, "flush flask", "sample at 50cm"},
   {45,   MINUTES,      EVENT_END_50CM,   "END! 50cm"},
   {1,    MINUTES,      EVENT_CLEAN75CM,  "flush probe", "flush the 75 cm probe" },
   {10,   MINUTES,      EVENT_START_75CM,  "flush flask", "sample at 75 cm"},
   {45,   MINUTES,      EVENT_END_75CM,   "END! 75 cm"},
   {1,    MINUTES,      EVENT_END_CLEAN,  "final clean"},
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
        valco->changeDirection(VALVE_DIR_CW);
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
        valco->changeDirection(VALVE_DIR_CC);
        valco->changePosition(valco_pos_25cm);
      } else if (event == EVENT_END_25CM) {
        // move valco to starting position
        valco->changeDirection(VALVE_DIR_CW);
        valco->changePosition(1);
        // turn relay off
        r25cm->changeRelay(false);
        // resume state saving so once this event finishes it will resume here in case of power out
        ctrl->resumeStateSaving();
      } else if(event == EVENT_CLEAN50CM){
        // pause state saving so it doesn't resume at a partial sampling point
        ctrl->pauseStateSaving(); 
        //double check the valco is HM
        valco->changePosition(1);
        //open the 50 cm probe
        r50cm->changeRelay(true);
      } else if (event == EVENT_START_50CM) {
        // move valco sampling position
        valco->changeDirection(VALVE_DIR_CC);
        valco->changePosition(valco_pos_50cm);
      } else if (event == EVENT_END_50CM) {
        // move valco to starting position
        valco->changeDirection(VALVE_DIR_CW);
        valco->changePosition(1);
        // turn relay off
        r50cm->changeRelay(false);
        // resume state saving so once this event finishes it will resume here in case of power out
        ctrl->resumeStateSaving();
      } else if (event == EVENT_CLEAN75CM){
        // pause state saving so it doesn't resume at a partial sampling point
        ctrl->pauseStateSaving(); 
        //double check the valco is HM
        valco->changeDirection(VALVE_DIR_CW);
        valco->changePosition(1);
        //open the 75 cm probe
        r75cm->changeRelay(true);
      } else if (event == EVENT_START_75CM) {
        // move valco sampling position
        valco->changeDirection(VALVE_DIR_CC);
        valco->changePosition(valco_pos_75cm);
      } else if (event == EVENT_END_75CM) {
        // move valco to starting position
        valco->changeDirection(VALVE_DIR_CW);
        valco->changePosition(1);
        // turn relay off
        r75cm->changeRelay(false);
        // resume state saving so once this event finishes it will resume here in case of power out
        ctrl->resumeStateSaving();
      } else if (event == EVENT_END_CLEAN){
        // pause state saving so it doesn't resume at a partial sampling point
        ctrl->pauseStateSaving(); 
        // move valco to starting position
        valco->changeDirection(VALVE_DIR_CW);
        valco->changePosition(1);
        //open bypass valve
        rbypass->changeRelay(true);
      } else if (event == EVENT_END) {
        //close the bypass valve
        rbypass->changeRelay(false);
        // turn power back off
        rpower->changeRelay(false);
        // resume state saving so once this event finishes it will resume here in case of power out
        ctrl->resumeStateSaving();
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
long lcd_update = 0;

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
  //valco->debug(); // debug serila communication

  // display
  controller->setDisplay(lcd);

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
  
  // allow start-up to complete properly
	delay(1000);
  lcd_update = millis();

}

void loop() {
  
  controller->update();
  // update once a second for scheduling timers
  if (millis() - lcd_update > 1000) {
    lcd_update = millis();
    lcd_update_callback();
  }
  
}
