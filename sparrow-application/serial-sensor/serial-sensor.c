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

//ST Headers
#include <main.h>

// Blues headers
#include <framework.h>
#include <note.h>

// If TRUE we have a device attached that is doing the big number crunching
#define SERIAL_SENSOR_MODE          false

#define REQUESTID_TEMPLATE          42

// This would be better not set at compile time,
// but rather set at initialization
// or in the response from the sister device
// yielding "*#DS18B20.qo" or "*#edgeimpulse.qo"
// as indicated in the response
#define APPLICATION_NOTEFILE        "*#serial.qo"

typedef struct applicationContext {
    /// @brief collection of things we need to remember as an application
    bool templateRegistered;
    bool done;
} applicationContext;
UART_HandleTypeDef huart2;


// Forwards
static void addSerialNote(applicationContext * ctx, bool immediate);
static const char * serialStateName(int state);
static bool registerNotefileTemplate (void);

// Application Activation (on wake)
bool serialsensorActivate(int appID, void *appContext)
{
  APP_PRINTF("Serial Sensor: Application Callback entered: serialsensorActivate\r\n\tappId: %d\r\n", appID);
  // Load context
  applicationContext *ctx = appContext;
  // reset state
  ctx->done = false;

  return true;
}

// Scheduled App One-Time Init
bool serialsensorInit(void)
{
    APP_PRINTF("Serial Sensor: Initializing application \r\n");
    bool result = false;

    // Allocate and warm up application context
    applicationContext *ctx = (applicationContext *)malloc(sizeof(applicationContext));
    ctx->done = false;
    ctx->templateRegistered = false;

    // wrangle the serial conection and say hello
    //...
    MX_USART1_Init();
    MX_USART1_UART_Transmit("Hello\n", 6, 100);

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
    };

    // Regitration was successful if the application identifier
    // is greater than or equal to zero
    result = (0 <= schedRegisterApp(&config));

    return result;
}

// Interrupt handler, maybe
void serialsensorISR (int appID, uint16_t pins, void *appContext)
{
  UNUSED(appContext);
  // ensure app is running
  if(!schedIsActive(appID)) {
    schedActivateNowFromISR(appID, true, 1);
  }

}

static bool registerNotefileTemplate(void)
{
    APP_PRINTF("Serial Sensor: template registration request");

    // Create the request
    J *req = NoteNewRequest("note.template");
    if (req == NULL) {
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

    // Create the body
    J *body = JAddObjectToObject(req, "body");
    if(body == NULL){
        JDelete(req);
        return false;
    }

    // Fill in the body template with message from sister device
    // this needs to be determined at init
    // Ideally this should pack a lot of detail into the template, but lets
    // just get it working
    JAddStringToObject(body, "Message", TSTRINGV);

    // Send request to the gateway
    noteSendToGatewayAsync(req, true);

}

// Gateway response handler
void serialsensorResponse(int appID, J *rsp, void *appContext)
{
    APP_PRINTF("Serial Sensor: Entered application callback function: serialsensorResponse\r\n\tappId: %d", appID);

    applicationContext *ctx = appContext;

    // See if there's an error
    char *err = JGetString(rsp, "err");
    if (err[0] != '\0') {
        APP_PRINTF("Serial Sensor: gateway returned error: %s\r\n", err);
        return;
    }

    // Identify specific response(s)
    // We probably need to log the appid to map templates to sensor types
    switch (JGetInt(rsp, "id")) {
    case REQUESTID_TEMPLATE:
        ctx->templateRegistered = true;
        APP_PRINTF("Serial Sensor: SUCCESSFUL template registration\r\n");
        break;
    default:
        APP_PRINTF("Serial Sensor: received unexpected response\r\n");
    }

}

static void addSerialNote(applicationContext * ctx, bool immediate)
{
    APP_PRINTF("Serial Sensor: generating note\r\n");

    // Create the request
    J *req = NoteNewRequest("note.add");
    if (req == NULL){
        return;
    }

    // If Immediate, sync now
    if(immediate){
        JAddBoolToObject(req, "sync", true);
    }
    HAL_UART_Receive_DMA(&huart1, );


    // Set the target notefile
    // Ideally this would take a sensor parameter from the sister device
    JAddStringToObject(req, "file", APPLICATION_NOTEFILE);
    //JAddStringToObject(req, "file", ~serialstruct.sensor);

    // Create the Body
    J *body = JAddObjectToObject(req, "body");
    if (body == NULL){
        JDelete(req);
        return;
    }

    // Fill in the body
    JAddStringToObject(body, );
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  /* Prevent unused argument(s) compilation warning */
  UNUSED(huart);
  // Add Rx msgs to application context and ding the bell


}