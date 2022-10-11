/**
 * This class is the basis for logger components that provide control over some peripheral but don't read data.
 */

#pragma once
#include "LoggerComponent.h"
#include "LoggerController.h"
#include "LoggerDisplay.h"

/* component */
class ControllerLoggerComponent : public LoggerComponent
{

  public:

    /*** constructors ***/
    // these types of logger components don't usually have global time offsets --> set to false
    // these types of logger components usually manage their own data clearing --> set to false
    ControllerLoggerComponent (const char *id, LoggerController *ctrl) : LoggerComponent(id, ctrl, false, false) {}

};
