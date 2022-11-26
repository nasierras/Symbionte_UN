// Copyright (c) Microsoft Corporation. All rights reserved.
// SPDX-License-Identifier: MIT

#include <stdlib.h>
#include <stdarg.h>

#include <az_core.h>
#include <az_iot.h>

#include "AzureIoT.h"
#include "Azure_IoT_PnP_Template.h"

#include <az_precondition_internal.h>

float V1, V2, V3, A1, A2, A3, SLT, DLT, SymbT, SSP, SCP, OIL, POW, PF, VIB, CO2, SH;

float ENV0, ENV1, ENV2, ENV3, ENV4, ENV5, ENV6, ENV7, ENV8, READ0, READ1, READ2, READ3, READ4, READ5, READ6, READ7, READ8, SH0, SH1, SH2, VOLT0, VOLT1, VOLT2, AMP0, AMP1, AMP2, TEMP0, TEMP1, TEMP2, TEMP3, TEMP4, TEMP5, TEMP6, TEMP7, TEMP8, LEAK0, LEAK1, LEAK2, LEAK3;

/* --- Defines --- */
#define AZURE_PNP_MODEL_ID "dtmi:azureiot:devkit:freertos:Esp32AzureIotKit;1"

#define SAMPLE_DEVICE_INFORMATION_NAME                 "deviceInformation"
#define SAMPLE_MANUFACTURER_PROPERTY_NAME              "manufacturer"
#define SAMPLE_MODEL_PROPERTY_NAME                     "model"
#define SAMPLE_SOFTWARE_VERSION_PROPERTY_NAME          "swVersion"
#define SAMPLE_OS_NAME_PROPERTY_NAME                   "osName"
#define SAMPLE_PROCESSOR_ARCHITECTURE_PROPERTY_NAME    "processorArchitecture"
#define SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_NAME    "processorManufacturer"
#define SAMPLE_TOTAL_STORAGE_PROPERTY_NAME             "totalStorage"
#define SAMPLE_TOTAL_MEMORY_PROPERTY_NAME              "totalMemory"

#define SAMPLE_MANUFACTURER_PROPERTY_VALUE             "ESPRESSIF"
#define SAMPLE_MODEL_PROPERTY_VALUE                    "Symbionte ESP32AzureIoT"
#define SAMPLE_VERSION_PROPERTY_VALUE                  "1.0.0"
#define SAMPLE_OS_NAME_PROPERTY_VALUE                  "FreeRTOS"
#define SAMPLE_ARCHITECTURE_PROPERTY_VALUE             "ESP32 LOLIN32"
#define SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_VALUE   "ESPRESSIF"
// The next couple properties are in KiloBytes.
#define SAMPLE_TOTAL_STORAGE_PROPERTY_VALUE            4096
#define SAMPLE_TOTAL_MEMORY_PROPERTY_VALUE             8192

#define TELEMETRY_PROP_NAME_VOLTS1               "V1"
#define TELEMETRY_PROP_NAME_VOLTS2               "V2"
#define TELEMETRY_PROP_NAME_VOLTS3               "V3"
#define TELEMETRY_PROP_NAME_AMPS1                "A1"
#define TELEMETRY_PROP_NAME_AMPS2                "A2"
#define TELEMETRY_PROP_NAME_AMPS3                "A3"
#define TELEMETRY_PROP_NAME_SLT                  "SLT"
#define TELEMETRY_PROP_NAME_DLT                  "DLT"
#define TELEMETRY_PROP_NAME_CIRCTEMP             "SymbT"
#define TELEMETRY_PROP_NAME_SSP                  "SSP"
#define TELEMETRY_PROP_NAME_SCP                  "SCP"
#define TELEMETRY_PROP_NAME_OIL                  "OIL"
#define TELEMETRY_PROP_NAME_CO2                  "CO2"
#define TELEMETRY_PROP_NAME_POWER                "POW"
#define TELEMETRY_PROP_NAME_PF                   "PF"
#define TELEMETRY_PROP_NAME_VIB                  "VIB"
#define TELEMETRY_PROP_NAME_SH                   "SH"

#define TELEMETRY_PROP_NAME_ENV0                 "ENV0"
#define TELEMETRY_PROP_NAME_ENV1                 "ENV1"
#define TELEMETRY_PROP_NAME_ENV2                 "ENV2"
#define TELEMETRY_PROP_NAME_ENV3                 "ENV3"
#define TELEMETRY_PROP_NAME_ENV4                 "ENV4"
#define TELEMETRY_PROP_NAME_ENV5                 "ENV5"
#define TELEMETRY_PROP_NAME_ENV6                 "ENV6"
#define TELEMETRY_PROP_NAME_ENV7                 "ENV7"
#define TELEMETRY_PROP_NAME_ENV8                 "ENV8"
#define TELEMETRY_PROP_NAME_READ0                "READ0"
#define TELEMETRY_PROP_NAME_READ1                "READ1"
#define TELEMETRY_PROP_NAME_READ2                "READ2"
#define TELEMETRY_PROP_NAME_READ3                "READ3"
#define TELEMETRY_PROP_NAME_READ4                "READ4"
#define TELEMETRY_PROP_NAME_READ5                "READ5"
#define TELEMETRY_PROP_NAME_READ6                "READ6"
#define TELEMETRY_PROP_NAME_READ7                "READ7"
#define TELEMETRY_PROP_NAME_READ8                "READ8"
#define TELEMETRY_PROP_NAME_SH0                  "SH0"
#define TELEMETRY_PROP_NAME_SH1                  "SH1"
#define TELEMETRY_PROP_NAME_SH2                  "SH2"
#define TELEMETRY_PROP_NAME_VOLT0                "VOLT0"
#define TELEMETRY_PROP_NAME_VOLT1                "VOLT1"
#define TELEMETRY_PROP_NAME_VOLT2                "VOLT2"
#define TELEMETRY_PROP_NAME_AMP0                 "AMP0"
#define TELEMETRY_PROP_NAME_AMP1                 "AMP1"
#define TELEMETRY_PROP_NAME_AMP2                 "AMP2"
#define TELEMETRY_PROP_NAME_TEMP0                "TEMP0"
#define TELEMETRY_PROP_NAME_TEMP1                "TEMP1"
#define TELEMETRY_PROP_NAME_TEMP2                "TEMP2"
#define TELEMETRY_PROP_NAME_TEMP3                "TEMP3"
#define TELEMETRY_PROP_NAME_TEMP4                "TEMP4"
#define TELEMETRY_PROP_NAME_TEMP5                "TEMP5"
#define TELEMETRY_PROP_NAME_TEMP6                "TEMP6"
#define TELEMETRY_PROP_NAME_TEMP7                "TEMP7"
#define TELEMETRY_PROP_NAME_TEMP8                "TEMP8"
#define TELEMETRY_PROP_NAME_LEAK0                "LEAK0"
#define TELEMETRY_PROP_NAME_LEAK1                "LEAK1"
#define TELEMETRY_PROP_NAME_LEAK2                "LEAK2"
#define TELEMETRY_PROP_NAME_LEAK3                "LEAK3"

static az_span COMMAND_NAME_TOGGLE_LED_1 = AZ_SPAN_FROM_STR("ToggleLed1");
static az_span COMMAND_NAME_TOGGLE_LED_2 = AZ_SPAN_FROM_STR("ToggleLed2");
static az_span COMMAND_NAME_DISPLAY_TEXT = AZ_SPAN_FROM_STR("DisplayText");

#define COMMAND_RESPONSE_CODE_ACCEPTED                 202
#define COMMAND_RESPONSE_CODE_REJECTED                 404

#define WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS          "telemetryFrequencySecs"
#define WRITABLE_PROPERTY_RESPONSE_SUCCESS             "success"

#define DOUBLE_DECIMAL_PLACE_DIGITS 2

#define DOUBLE_DECIMAL_INTEGER 0

/* --- Function Checks and Returns --- */
#define RESULT_OK       0
#define RESULT_ERROR    __LINE__

#define EXIT_IF_TRUE(condition, retcode, message, ...)                              \
  do                                                                                \
  {                                                                                 \
    if (condition)                                                                  \
    {                                                                               \
      LogError(message, ##__VA_ARGS__ );                                            \
      return retcode;                                                               \
    }                                                                               \
  } while (0)

#define EXIT_IF_AZ_FAILED(azresult, retcode, message, ...)                                   \
  EXIT_IF_TRUE(az_result_failed(azresult), retcode, message, ##__VA_ARGS__ )

/* --- Data --- */
#define DATA_BUFFER_SIZE 1024
static uint8_t data_buffer[DATA_BUFFER_SIZE];
static uint32_t telemetry_send_count = 0;

static size_t telemetry_frequency_in_seconds = 10; // With default frequency of once in 10 seconds.
static time_t last_telemetry_send_time = INDEFINITE_TIME;

static bool led1_on = false;
static bool led2_on = false;

/* --- Function Prototypes --- */
/* Please find the function implementations at the bottom of this file */
static int generate_telemetry_payload(
  uint8_t* payload_buffer, size_t payload_buffer_size, size_t* payload_buffer_length);
static int generate_device_info_payload(
  az_iot_hub_client const* hub_client, uint8_t* payload_buffer,
  size_t payload_buffer_size, size_t* payload_buffer_length);
static int consume_properties_and_generate_response(
  azure_iot_t* azure_iot, az_span properties,
  uint8_t* buffer, size_t buffer_size, size_t* response_length);

/* --- Public Functions --- */
void azure_pnp_init()
{
}

void getSignals(float A1_r, float A2_r, float A3_r, float SLT_r, float DLT_r,float SCP_r, float SSP_r, float OIL_r, float CO2_r, float CircTemp_r, float VIB_r, float POW_r, float PF_r, float V1_r, float V2_r, float V3_r, float SH_r){
  A1 = A1_r;
  A2 = A2_r;
  A3 = A3_r;
  SLT = SLT_r;
  DLT = DLT_r;
  SCP = SCP_r;
  SSP = SSP_r;
  OIL = OIL_r;
  CO2 = CO2_r;
  SymbT = CircTemp_r;
  VIB = VIB_r;
  POW = POW_r;
  PF = PF_r;
  V1 = V1_r;
  V2 = V2_r;
  V3 = V3_r;
  SH = SH_r;
};

void getFailures(float ENV0_r, float ENV1_r, float ENV2_r, float ENV3_r, float ENV4_r, float ENV5_r, float ENV6_r, float ENV7_r, float ENV8_r, float READ0_r, float READ1_r, float READ2_r, float READ3_r, float READ4_r, float READ5_r, float READ6_r, float READ7_r, float READ8_r, float SH0_r, float SH1_r, float SH2_r, float VOLT0_r, float VOLT1_r, float VOLT2_r, float AMP0_r, float AMP1_r, float AMP2_r, float TEMP0_r, float TEMP1_r, float TEMP2_r, float TEMP3_r, float TEMP4_r, float TEMP5_r, float TEMP6_r, float TEMP7_r, float TEMP8_r, float LEAK0_r, float LEAK1_r, float LEAK2_r, float LEAK3_r){
  ENV0 = ENV0_r;
  ENV1 = ENV1_r;
  ENV2 = ENV2_r;
  ENV3 = ENV3_r;
  ENV4 = ENV4_r;
  ENV5 = ENV5_r;
  ENV6 = ENV6_r;
  ENV7 = ENV7_r;
  ENV8 = ENV8_r;
  READ0 = READ0_r;
  READ1 = READ1_r;
  READ2 = READ2_r;
  READ3 = READ3_r;
  READ4 = READ4_r;
  READ5 = READ5_r;
  READ6 = READ6_r;
  READ7 = READ7_r;
  READ8 = READ8_r;
  SH0 = SH0_r;
  SH1 = SH1_r;
  SH2 = SH2_r;
  VOLT0 = VOLT0_r;
  VOLT1 = VOLT1_r;
  VOLT2 = VOLT2_r;
  AMP0 = AMP0_r;
  AMP1 = AMP1_r;
  AMP2 = AMP2_r;
  TEMP0 = TEMP0_r;
  TEMP1 = TEMP1_r;
  TEMP2 = TEMP2_r;
  TEMP3 = TEMP3_r;
  TEMP4 = TEMP4_r;
  TEMP5 = TEMP5_r;
  TEMP6 = TEMP6_r;
  TEMP7 = TEMP7_r;
  TEMP8 = TEMP8_r;
  LEAK0 = LEAK0_r;
  LEAK1 = LEAK1_r;
  LEAK2 = LEAK2_r;
  LEAK3 = LEAK3_r;
};

const az_span azure_pnp_get_model_id()
{
  return AZ_SPAN_FROM_STR(AZURE_PNP_MODEL_ID);
}

void azure_pnp_set_telemetry_frequency(size_t frequency_in_seconds)
{
  telemetry_frequency_in_seconds = frequency_in_seconds;
  LogInfo("Telemetry frequency set to once every %d seconds.", telemetry_frequency_in_seconds);
}

/* Application-specific data section */

int azure_pnp_send_telemetry(azure_iot_t* azure_iot){
  _az_PRECONDITION_NOT_NULL(azure_iot);
  time_t now = time(NULL);
  if (now == INDEFINITE_TIME)
  {
    LogError("Failed getting current time for controlling telemetry.");
    return RESULT_ERROR;
  }
  else if (last_telemetry_send_time == INDEFINITE_TIME ||
           difftime(now, last_telemetry_send_time) >= telemetry_frequency_in_seconds)
  {
    size_t payload_size;

    last_telemetry_send_time = now;

    if (generate_telemetry_payload(data_buffer, DATA_BUFFER_SIZE, &payload_size) != RESULT_OK)
    {
      LogError("Failed generating telemetry payload.");
      return RESULT_ERROR;
    }

    if (azure_iot_send_telemetry(azure_iot, az_span_create(data_buffer, payload_size)) != 0)
    {
      LogError("Failed sending telemetry.");
      return RESULT_ERROR;
    }
  }

  return RESULT_OK;
}

//Send data to Azure
int azure_pnp_send_device_info(azure_iot_t* azure_iot, uint32_t request_id){
  _az_PRECONDITION_NOT_NULL(azure_iot);

  int result;
  size_t length;  
    
  result = generate_device_info_payload(&azure_iot->iot_hub_client, data_buffer, DATA_BUFFER_SIZE, &length);
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed generating telemetry payload.");

  result = azure_iot_send_properties_update(azure_iot, request_id, az_span_create(data_buffer, length));
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed sending reported properties update.");

  return RESULT_OK;
}


// Actions for the device
int azure_pnp_handle_command_request(azure_iot_t* azure_iot, command_request_t command){
  _az_PRECONDITION_NOT_NULL(azure_iot);

  uint16_t response_code;

  if (az_span_is_content_equal(command.command_name, COMMAND_NAME_TOGGLE_LED_1))
  {
    led1_on = !led1_on;
    LogInfo("LED 1 state: %s", (led1_on ? "ON" : "OFF"));
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else if (az_span_is_content_equal(command.command_name, COMMAND_NAME_TOGGLE_LED_2))
  {
    led2_on = !led2_on;
    LogInfo("LED 2 state: %s", (led2_on ? "ON" : "OFF"));
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else if (az_span_is_content_equal(command.command_name, COMMAND_NAME_DISPLAY_TEXT)){
    // The payload comes surrounded by quotes, so to remove them we offset the payload by 1 and its size by 2.
    LogInfo("OLED display: %.*s", az_span_size(command.payload) - 2, az_span_ptr(command.payload) + 1);
    response_code = COMMAND_RESPONSE_CODE_ACCEPTED;
  }
  else{
    LogError("Command not recognized (%.*s).", az_span_size(command.command_name), az_span_ptr(command.command_name));
    response_code = COMMAND_RESPONSE_CODE_REJECTED;
  }

  return azure_iot_send_command_response(azure_iot, command.request_id, response_code, AZ_SPAN_EMPTY);
}

int azure_pnp_handle_properties_update(azure_iot_t* azure_iot, az_span properties, uint32_t request_id){
  _az_PRECONDITION_NOT_NULL(azure_iot);
  _az_PRECONDITION_VALID_SPAN(properties, 1, false);

  int result;
  size_t length;

  result = consume_properties_and_generate_response(azure_iot, properties, data_buffer, DATA_BUFFER_SIZE, &length);
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed generating properties ack payload.");

  result = azure_iot_send_properties_update(azure_iot, request_id, az_span_create(data_buffer, length));
  EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "Failed sending reported properties update.");

  return RESULT_OK;
}

/* --- Internal Functions --- */

static int generate_telemetry_payload(uint8_t* payload_buffer, size_t payload_buffer_size, size_t* payload_buffer_length){
  az_json_writer jw;
  az_result rc;
  az_span payload_buffer_span = az_span_create(payload_buffer, payload_buffer_size);
  az_span json_span;
  
  // Acquiring the simulated data.
  PF = 0.86;

  // initializing
  rc = az_json_writer_init(&jw, payload_buffer_span, NULL);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed initializing json writer for telemetry.");
  rc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed setting telemetry json root.");

  // sending data (telemetry)
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTS1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage01 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, V1, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage01 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTS2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage02 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, V2, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage02 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLTS3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage03 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, V3, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage03 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMPS1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps01 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, A1, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps01 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMPS2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps02 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, A2, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps02 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMPS3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps03 property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, A3, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps03 property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SLT)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding suction_temp property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, SLT, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding suction_temp property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_DLT)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding discharge_temp property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, DLT, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding discharge_temp property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CIRCTEMP)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding circuit_temp property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, SymbT, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding circuit_temp property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SSP)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding suction_pressure property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, SSP, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding suction_pressure property value to telemetry payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SCP)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding discharge_pressure property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, SCP, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding discharge_pressure property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_OIL)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding oil_pressure property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, OIL, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding oil_pressure property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_CO2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding CO2_ppm property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, CO2, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding CO2_ppm property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_POWER)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding power property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, POW, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding power property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_PF)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding power_factor property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, PF, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding power_factor property value to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VIB)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding vibration property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, VIB, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding vibration property value to telemetry payload."); 

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SH)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat property name to telemetry payload.");  
  rc = az_json_writer_append_double(&jw, SH, DOUBLE_DECIMAL_PLACE_DIGITS);  
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat property value to telemetry payload."); 

  /// ----- Error writing --- ///
  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_10 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_10 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_11 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_11 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_12 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_12 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_13 to telemetry payload.");
  rc = az_json_writer_append_double(&jw, ENV3, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_13 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV4)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_14 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV4, DOUBLE_DECIMAL_INTEGER);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_14 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV5)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_15 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV5, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_15 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV6)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_16 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV6, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_16 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV7));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_17 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV7, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_17 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_ENV8)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_18 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, ENV8, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding envelope_error_18 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_20 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_20 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_21 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_21 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_22 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_22 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_23 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ3, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_23 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ4)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_24 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ4, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_24 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ5)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_25 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ5, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_25 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ6)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_26 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ6, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_26 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ7));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_27 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ7, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_27 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_READ8)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_28 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, READ8, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding sensor_error_28 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SH0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_30 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, SH0, DOUBLE_DECIMAL_INTEGER);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_30 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SH1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_31 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, SH1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_31 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_SH2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_32 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, SH2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding superheat_error_32 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLT0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_40 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, VOLT0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_40 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLT1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_41 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, VOLT1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_41 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_VOLT2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_42 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, VOLT2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding voltage_error_42 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMP0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_50 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, AMP0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_50 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMP1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_51 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, AMP1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_51 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_AMP2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_52 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, AMP2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding amps_error_52 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_60 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_60 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_61 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_61 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_62 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_62 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_63 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP3, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_63 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP4)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_64 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP4, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_64 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP5)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_65 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP5, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_65 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP6)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_66 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP6, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_66 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP7)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_67 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP7, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_67 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_TEMP8)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_68 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, TEMP8, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding temperature_error_68 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_LEAK0)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_70 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, LEAK0, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_70 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_LEAK1)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_71 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, LEAK1, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_71 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_LEAK2)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_72 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, LEAK2, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_72 to telemetry payload.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(TELEMETRY_PROP_NAME_LEAK3)); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_73 to telemetry payload."); 
  rc = az_json_writer_append_double(&jw, LEAK3, DOUBLE_DECIMAL_INTEGER); 
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding leakage_error_73 to telemetry payload.");
  
  // closing send
  rc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing telemetry json payload.");

  payload_buffer_span = az_json_writer_get_bytes_used_in_destination(&jw);

  if ((payload_buffer_size - az_span_size(payload_buffer_span)) < 1){
    LogError("Insufficient space for telemetry payload null terminator.");
    return RESULT_ERROR;
  }

  payload_buffer[az_span_size(payload_buffer_span)] = null_terminator;
  *payload_buffer_length = az_span_size(payload_buffer_span);
 
  return RESULT_OK;
}

static int generate_device_info_payload(az_iot_hub_client const* hub_client, uint8_t* payload_buffer, size_t payload_buffer_size, size_t* payload_buffer_length)
{
  az_json_writer jw;
  az_result rc;
  az_span payload_buffer_span = az_span_create(payload_buffer, payload_buffer_size);
  az_span json_span;

  rc = az_json_writer_init(&jw, payload_buffer_span, NULL);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed initializing json writer for telemetry.");

  rc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed setting telemetry json root.");
  
  rc = az_iot_hub_client_properties_writer_begin_component(
    hub_client, &jw, AZ_SPAN_FROM_STR(SAMPLE_DEVICE_INFORMATION_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed writting component name.");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_MANUFACTURER_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_MANUFACTURER_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_MANUFACTURER_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_MANUFACTURER_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_MODEL_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_MODEL_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_MODEL_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_MODEL_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_SOFTWARE_VERSION_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_SOFTWARE_VERSION_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_VERSION_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_VERSION_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_OS_NAME_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_OS_NAME_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_OS_NAME_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_OS_NAME_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_PROCESSOR_ARCHITECTURE_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_PROCESSOR_ARCHITECTURE_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_ARCHITECTURE_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_ARCHITECTURE_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_string(&jw, AZ_SPAN_FROM_STR(SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_VALUE));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_PROCESSOR_MANUFACTURER_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_TOTAL_STORAGE_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_TOTAL_STORAGE_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_double(&jw, SAMPLE_TOTAL_STORAGE_PROPERTY_VALUE, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_TOTAL_STORAGE_PROPERTY_VALUE to payload. ");

  rc = az_json_writer_append_property_name(&jw, AZ_SPAN_FROM_STR(SAMPLE_TOTAL_MEMORY_PROPERTY_NAME));
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_TOTAL_MEMORY_PROPERTY_NAME to payload.");
  rc = az_json_writer_append_double(&jw, SAMPLE_TOTAL_MEMORY_PROPERTY_VALUE, DOUBLE_DECIMAL_PLACE_DIGITS);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed adding SAMPLE_TOTAL_MEMORY_PROPERTY_VALUE to payload. ");

  rc = az_iot_hub_client_properties_writer_end_component(hub_client, &jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing component object.");

  rc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(rc, RESULT_ERROR, "Failed closing telemetry json payload.");

  payload_buffer_span = az_json_writer_get_bytes_used_in_destination(&jw);

  if ((payload_buffer_size - az_span_size(payload_buffer_span)) < 1)
  {
    LogError("Insufficient space for telemetry payload null terminator.");
    return RESULT_ERROR;
  }

  payload_buffer[az_span_size(payload_buffer_span)] = null_terminator;
  *payload_buffer_length = az_span_size(payload_buffer_span);
 
  return RESULT_OK;
}

static int generate_properties_update_response(
  azure_iot_t* azure_iot,
  az_span component_name, int32_t frequency, int32_t version,
  uint8_t* buffer, size_t buffer_size, size_t* response_length)
{
  az_result azrc;
  az_json_writer jw;
  az_span response = az_span_create(buffer, buffer_size);

  azrc = az_json_writer_init(&jw, response, NULL);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed initializing json writer for properties update response.");

  azrc = az_json_writer_append_begin_object(&jw);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed opening json in properties update response.");

  // This Azure PnP Template does not have a named component,
  // so az_iot_hub_client_properties_writer_begin_component is not needed.

  azrc = az_iot_hub_client_properties_writer_begin_response_status(
    &azure_iot->iot_hub_client,
    &jw,
    AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS),
    (int32_t)AZ_IOT_STATUS_OK,
    version,
    AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_RESPONSE_SUCCESS));
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed appending status to properties update response.");

  azrc = az_json_writer_append_int32(&jw, frequency);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed appending frequency value to properties update response.");

  azrc = az_iot_hub_client_properties_writer_end_response_status(&azure_iot->iot_hub_client, &jw);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed closing status section in properties update response.");

  // This Azure PnP Template does not have a named component,
  // so az_iot_hub_client_properties_writer_end_component is not needed.

  azrc = az_json_writer_append_end_object(&jw);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed closing json in properties update response.");

  *response_length = az_span_size(az_json_writer_get_bytes_used_in_destination(&jw));

  return RESULT_OK;
}

static int consume_properties_and_generate_response(
  azure_iot_t* azure_iot, az_span properties,
  uint8_t* buffer, size_t buffer_size, size_t* response_length)
{
  int result;
  az_json_reader jr;
  az_span component_name;
  int32_t version = 0;

  az_result azrc = az_json_reader_init(&jr, properties, NULL);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed initializing json reader for properties update.");

  const az_iot_hub_client_properties_message_type message_type =
    AZ_IOT_HUB_CLIENT_PROPERTIES_MESSAGE_TYPE_WRITABLE_UPDATED;

  azrc = az_iot_hub_client_properties_get_properties_version(
    &azure_iot->iot_hub_client, &jr, message_type, &version);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed writable properties version.");

  azrc = az_json_reader_init(&jr, properties, NULL);
  EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed re-initializing json reader for properties update.");

  while (az_result_succeeded(
    azrc = az_iot_hub_client_properties_get_next_component_property(
      &azure_iot->iot_hub_client, &jr, message_type,
      AZ_IOT_HUB_CLIENT_PROPERTY_WRITABLE, &component_name)))
  {
    if (az_json_token_is_text_equal(&jr.token, AZ_SPAN_FROM_STR(WRITABLE_PROPERTY_TELEMETRY_FREQ_SECS)))
    {
      int32_t value;
      azrc = az_json_reader_next_token(&jr);
      EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed getting writable properties next token.");

      azrc = az_json_token_get_int32(&jr.token, &value);
      EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed getting writable properties int32_t value.");

      azure_pnp_set_telemetry_frequency((size_t)value);

      result = generate_properties_update_response(
        azure_iot, component_name, value, version, buffer, buffer_size, response_length);
      EXIT_IF_TRUE(result != RESULT_OK, RESULT_ERROR, "generate_properties_update_response failed.");
    }
    else
    {
      LogError("Unexpected property received (%.*s).",
        az_span_size(jr.token.slice), az_span_ptr(jr.token.slice));
    }

    azrc = az_json_reader_next_token(&jr);
    EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed moving to next json token of writable properties.");

    azrc = az_json_reader_skip_children(&jr);
    EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed skipping children of writable properties.");

    azrc = az_json_reader_next_token(&jr);
    EXIT_IF_AZ_FAILED(azrc, RESULT_ERROR, "Failed moving to next json token of writable properties (again).");
  }

  return RESULT_OK;
}
