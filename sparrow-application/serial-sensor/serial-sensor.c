// Copyright 2023 Cameron Bunce
// Licensed alike with the work from Blues that makes this all possible
// The mistakes contained herein are mine

#include "serial-sensor.h"

/*
| This Application is designed to provide serial extensions to 
| an offboard processor to speed application testing
| and to enable Edge Impulse models on higher-horsepower sister microcontrollers
| 
| EXPECT THIS TO BREAK - it is not finished
| EXPECT THIS TO CHANGE - implemtation is not final
| 
| Enabling this application will check to see if a device connected on the 
| Tx/Rx pins responds to a "Hello" message
| 
| Polling intervals "Poll" the sister microcontroller for the latest sensor reading
| Responses for which include, at an interval determined by the other device
| the assessment of the latest data through the Edge Impulse model
*/

// memory
#include <malloc.h>

// Blues headers
#include <framework.h>
#include <note.h>

// If TRUE we have a device attached that is doing the big number crunching
#define SERIAL_SENSOR_MODE          false

#define REQUESTED_TEMPLATE          42

// This would be better not set at compile time, 
// but rather set at initialization 
// or in the response from the sister device
// yielding "*#DS18B20.qo" or "*#edgeimpulse.qo" 
// as indicated in the response
#define APPLICATION_NOTEFILE        "*#serial.qo" 

typedef struct applicationContext {
    bool templateRegistered;
    bool done;
} applicationContext;

// Forwards
static bool registerNotefileTemplate (void);

// Scheduled App One-Time Init
bool serialsensorInit(void)
{
    APP_PRINTF("Serial Sensor: Initializing application \r\n");

    // Allocate and warm up application context
    applicationContext *ctx = (applicationContext *)malloc(sizeof(applicationContext));
    ctx->done = false;
    ctx->templateRegistered = false;

    // wrangle the serial conection and say hello
    //...
    // Register the app
    schedAppConfig config = {
        .name = "serial-sensor",
        .activationPeriodSecs = 60 * 100,
        .pollPeriodSecs = 15,
        .activateFn = serialsensorActivate,
        .interruptFn = serialsensorISR,
        .pollFn = serialsensorPoll,
        .responseFn = serialsensorResponse,
        .appContext = ctx,
    }

    // Regitration was successful if the application identifier
    // is greater than or equal to zero
    result = (0 <= schedRegisterApp(&config));

    return result;
}

// Interrupt handler, maybe
// void serial-sensorISR()

static bool registerNotefileTemplate(void)
{
    APP_PRINTF("Serial Sensor: template registration request");
    
    // Create the request
    J *req = NoteNewRequest("note.template");
    if (req == NUL) {
        return false;
    }
    
    // Create the body
    J *body = JCreateObject();
    if (body == NULL) {
        JDelete(req);
        return false;
    }
    // Fill-in request parameters.  Note that in order to minimize
    // the size of the over-the-air JSON we're using a special format
    // for the "file" parameter implemented by the gateway, in which
    // a "file" parameter beginning with * will have that character
    // substituted with the textified Sparrow node address.
    JAddStringToObject(req, "file", APPLICATION_NOTEFILE);

    // Add an ID to the request, which will be echo'ed
    // back in the response by the notecard itself.  This
    // helps us to identify the asynchronous response
    // without needing to have an additional state.
    JAddNumberToObject(req, "id", REQUESTID_TEMPLATE);

}

// Gateway response handler
void serialsensorResponse(int appID, J *rsp, void *appContext)
{
    APP_PRINTF("Serial Sensor: Entered application callback function: serialsensorResponse\r\n\tappId: %d", appID);  
    
    // See if there's an error
    char *err = JGetString(rsp, "err");
    if (err[0] != '\0') {
        APP_PRINTF("Serial Sensor: gateway returned error: %s\r\n", err);
        return;
    }

    // Identify specific response(s)
    switch (JGetInt(rsp, "id")) {
    case REQUESTID_TEMPLATE:
        ctx->templateRegistered = true;
        APP_PRINTF("Serial Sensor: SUCCESSFUL template registration\r\n");
        break;
    default:
        APP_PRINTF("Serial Sensor: received unexpected response\r\n");
    }

}