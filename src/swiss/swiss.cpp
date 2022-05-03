
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
#define EVENT_START_25CM  4
#define EVENT_END_25CM    5
#define EVENT_START_50CM  6
#define EVENT_END_50CM    7
#define EVENT_START_75CM  8
#define EVENT_END_75CM    9
const SchedulerEvent schedule[] = {
   {0,   MILLISECONDS, EVENT_START,      "start", "description"},
   {10,  SECONDS,      EVENT_CLEAN,      "clean", "what goes on here?"},
   {0.1, MINUTES,      EVENT_START_25CM, "start 25cm", "bla blub?"},
   {5,   SECONDS,      EVENT_END_25CM,   "end 25cm"},
   {2,   SECONDS,      EVENT_END,        "complete"}
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
      } else if (event == EVENT_CLEAN) {

      } else if (event == EVENT_START_25CM) {
        // turn relay on
        r25cm->changeRelay(true);
        // move valco to sampling  position
        valco->changePosition(valco_pos_25cm);
      } else if (event == EVENT_END_25CM) {
        // move valco to starting position
        valco->changePosition(1);
        // turn relay off
        r25cm->changeRelay(false);
        
      } else if (event == EVENT_START_50CM) {

      } else if (event == EVENT_END_25CM) {

      } else if (event == EVENT_START_75CM) {

      } else if (event == EVENT_END_75CM) {

      } else if (event == EVENT_END) {
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
