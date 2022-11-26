// IO ports definition for ESP32
#define A1_port 2
#define A2_port 1
#define A3_port 0
#define MUX04 27
#define MUX03 14
#define MUX02 12
#define MUX01 13
#define AN01 34

#define I2CADDR 0x25      // I2C address for Keypad
#define LCDADDR 0x27      // I2C address for Liquid Crystal Display
Preferences preferences;  // Saving EEPROM object

TaskHandle_t Task2, Task3;
ThingESP32 Symbiothing("nasierras", "SymbionteUN", "UNAL_co");

const byte ROWS = 4; //four rows
const byte COLS = 4; //four columns
char hexaKeys[ROWS][COLS] = {
  {'1','4','7','*'},
  {'2','5','8','0'},
  {'3','6','9','#'},
  {'A','B','C','D'}
};
byte rowPins[ROWS] = {3, 2, 1, 0}; //connect to the row pinouts of the keypad
byte colPins[COLS] = {7, 6, 5, 4}; //connect to the column pinouts of the keypad

// -------------------------------------------------------------------------- //
// Analog Inputs (International Systems)
// - analogInputs[0,2] = [Amps Phase 01, Amps Phase 02, Amps Phase 03]
// - analogInputs[3,4] = [Suction Line Temp, Discharge Line Temp]
// - analogInputs[5,7] = [Suction Pressure, Discharge Pressure, Oil Pressure]
// - analogInputs[8] = [CO2 particles per million]
// - analogInputs[9] = [Circuit Temperature]
// - analogInputs[10] = [Acceleration Magnitude]
// - analogInputs[11] = [Power Average]
// - analogInputs[12] = [Power Factor - cos(phi)]
// - analogInputs[13,15] = [Volts Phase 01, Volts Phase 02, Volts Phase 03]
// - analogInputs[16,17] = [Superheat, previous Superheat]
volatile float analogInputs[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

// Coefficents for linear regresion for voltage
float slopesAC_V[3] = {0.388, 0.375, 0.435};
float interceptsAC_V[3] = {-28.467, -19.739, -40.340};
// Coefficents for linear regresion for current
float slopesAC_A[3] = {0.0046, 0.0046, 0.0046};
float interceptsAC_A[3] = {-0.7122, -0.7122, -0.7122};
// Pressure sensor coefficients
// - ai_P[3] = [Oil_coeff, SCP_coeff, SSP_coeff]
float a2_P[3] = {0.005483, 0.01234, 0.005483};
float a1_P[3] = {-6.647, -14.95, -6.647};
float a0_P[3] = {2017, 4539, 2017};
// Temperature coefficients
// - ai_P[3] = [Oil_coeff, SCP_coeff, SSP_coeff]
float a3_T[3] = {8.06E-08, 0, 4.82E-08};
float a2_T[3] = {-0.0001293, 0, -0.0003924};
float a1_T[3] = {0.1057, 0.02923, 1.109};
float a0_T[3] = {-39.74, -21.69, -1016};

LiquidCrystal_I2C LCD_20x4 = LiquidCrystal_I2C(LCDADDR, 20, 4);
Keypad_I2C Keypad_4x4 = Keypad_I2C( makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS, I2CADDR); 
Adafruit_MPU6050 mpu;
Adafruit_ADS1115 ads_Amps;
sensors_event_t a, g, temp;

int keyInp = 0;
int menu_level = 1;
int tempValue = 0;
char selected_option = ' ';
char selected_option2 = ' ';
int screenCounter = 1;
int flagInitialization = 0;

unsigned long t1 = 0;
unsigned long t2 = 0;

float pressures_LCD [] = {0, 0};
float temps_LCD [] = {0, 0};
float powerComp = 0;
float SH_s []= {0};
float SH_LCD []= {0};

int status = 1;

// -------------------------------------------------------------------------- //
// Configuration of the properties of the compressor
// compressorProperties[0-2] = [HP, Compression_Technology, Temperature_Units]
// compressorProperties[3-5] = [Voltage, Phases, Frequency]
// compressorProperties[6-8] = [Refrigerant, Temperature_Application, Pressure_Units]
// compressorProperties[9-11] = [Voltage_Protection, Current_Protection, Temperature_Protection]
volatile float compressorProperties[12];

// -------------------------------------------------------------------------- //
// Dictionaries for variables
const char *dictCompType[] = {"NotSelect","Semihermetic","Scroll","Piston hermetic"};
const char *dictRefrigerant[] = {"NotS","R22  ","R410A","R134a","R404A","R507A","NaN ","R448A","R449A","R290 ","R407A","R407C","NaN ","R407F","R422D","R422A","R438A","R513A","NaN ","CO2-SC","CO2-TC","CO2-Pa"};
const char *dictApp[] = {"NA","HT","MT","LT"};
const char *dictUnitsP[] = {"NaN ","bAr ","psig"};
const char *dictUnitsT[] = {"X","C","F"};
const char *dictUnitsT_dif[] = {"X","K","F"};
const char *dictStatus[] = {"OFF", "ON "};
const char *dictVolts[] = {"NaN", "110-120V", "208-240V", "420-480V"};
const char *dictPhase[] = {"NaN", "1Ph", "3Ph"};
const char *dictFreq[] = {"NaN", "50Hz", "60Hz"};

// -------------------------------------------------------------------------- //// Failure Count per condition
// ENV = Envelope Error(s)
// READ = Reading or Probe Operation Error(s)
// SH = Superheat Error(s)
// VOLT = Voltage Error(s)
// AMP = Current Error(s)
// LEAK = Leakage and Vibration Error(s)
//   FailureCount[0] = ENV0 - Normal Operation
//   FailureCount[1] = ENV1 - Low Evaporation Press and High Condensation Press
//   FailureCount[2] = ENV2 - High Condensation Press
//   FailureCount[3] = ENV3 - High Evaporation Press and High Condensation Press
//   FailureCount[4] = ENV4 - High Evaporation Press
//   FailureCount[5] = ENV5 - High Evaporation Press and Low Condensation Press
//   FailureCount[6] = ENV6 - Low Condensation Press
//   FailureCount[7] = ENV7 - Low Evaporation Press and Low Condensation Press
//   FailureCount[8] = ENV8 - Low Evaporation Press
//   FailureCount[10] = READ0 - Normal Sensor Operation
//   FailureCount[11] = READ1 - Temperature Probe Error
//   FailureCount[12] = READ2 - Pressure Probe Error
//   FailureCount[13] = READ3 - CT Error
//   FailureCount[14] = READ4 - Voltage Probe Error
//   FailureCount[15] = READ5 - CO2 Probe Error
//   FailureCount[16] = READ6 - Accelerometer Error
//   FailureCount[17] = READ7 - Wifi Error
//   FailureCount[18] = READ8 - SD Error
//   FailureCount[20] = SH0 - Normal Superheat
//   FailureCount[21] = SH1 - High Superheat
//   FailureCount[22] = SH2 - Low Superheat
//   FailureCount[30] = VOLT0 - Normal Voltage
//   FailureCount[31] = VOLT1 - Missing Phase
//   FailureCount[32] = VOLT2 - Voltage Umbalance
//   FailureCount[40] = AMP0 - Normal Amps
//   FailureCount[41] = AMP1 - High Amps Draw
//   FailureCount[42] = AMP2 - Amps Umbalance
//   FailureCount[50] = TEMP0 - Normal Operation
//   FailureCount[51] = TEMP1 - Low Suction Temp and High Discharge Temp
//   FailureCount[52] = TEMP2 - High Discharge Temp
//   FailureCount[53] = TEMP3 - High Suction Temp and High Discharge Temp
//   FailureCount[54] = TEMP4 - High Suction Temp
//   FailureCount[55] = TEMP5 - High Suction Temp and Low Discharge Temp
//   FailureCount[56] = TEMP6 - Low Discharge Temp
//   FailureCount[57] = TEMP7 - Low Suction Temp and Low Discharge Temp
//   FailureCount[58] = TEMP8 - Low Suction Temp
//   FailureCount[60] = LEAK0 - Normal Leakage Module
//   FailureCount[61] = LEAK1 - Low Vibration with CO2 increase
//   FailureCount[62] = LEAK2 - High Vibration without CO2 increase
//   FailureCount[63] = LEAK3 - High Vibration with CO2 increase
//   FailureCount[NumbersNotListed] = FFU - For Future Use
volatile float FailureCount[70] = {
  0, 2, 1, 3, 1, 2, 1, 1, 0, 0, 
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 
  0, 6, 2, 0, 0, 0, 0, 0, 0, 0, 
  0, 1, 8, 0, 0, 0, 0,0 , 0, 0, 
  0, 15, 4, 0, 0, 0, 0, 0, 0, 0, 
  0, 2, 1, 3, 1, 2, 1, 1, 0, 0, 
  0, 1, 4, 1, 0, 0, 0, 0, 0, 0
  };

// Pressure Envelope Limits
// SSP -> Saturated Suction Pressure
// SCP -> Saturated Condensing Pressure
// LL -> Low Limits
// HL -> High Limit
// pressEnvXT [] = {
//	{R22 SSP-LL, R22 SSP-HL, R22 SCP-LL, R22 SCP-HL},
//	{R410A SSP-LL, R410A SSP-HL, R410A SCP-LL, R410A SCP-HL},
//	{R134A SSP-LL, R134A SSP-HL, R134A SCP-LL, R134A SCP-HL},
//	{R404A SSP-LL, R404A SSP-HL, R404A SCP-LL, R404A SCP-HL},
//	{R507A SSP-LL, R507A SSP-HL, R507A SCP-LL, R507A SCP-HL},
//	{R448A SSP-LL, R448A SSP-HL, R448A SCP-LL, R448A SCP-HL},
//	{R449A SSP-LL, R449A SSP-HL, R449A SCP-LL, R449A SCP-HL},
//	{R290 SSP-LL,  R290 SSP-HL,  R290 SCP-LL,  R290 SCP-HL},
//	{R407A SSP-LL, R407A SSP-HL, R407A SCP-LL, R407A SCP-HL},
//	{R407C SSP-LL, R407C SSP-HL, R407C SCP-LL, R407C SCP-HL},
//	{R407F SSP-LL, R407F SSP-HL, R407F SCP-LL, R407F SCP-HL},
//	{R422D SSP-LL, R422D SSP-HL, R422D SCP-LL, R422D SCP-HL},
//	{R422A SSP-LL, R422A SSP-HL, R422A SCP-LL, R422A SCP-HL},
//	{R438A SSP-LL, R438A SSP-HL, R438A SCP-LL, R438A SCP-HL},
//	{R513A SSP-LL, R513A SSP-HL, R513A SCP-LL, R513A SCP-HL},
//	{R744-XC SSP-LL, R744-XC SSP-HL, R744-XC SCP-LL, R744-XC SCP-HL},
// };

static float pressEnvLT[][4]= {
	{1.052, 2.962, 5.841, 24.275},
	{1.749, 4.800, 9.332, 38.344},
	{0.512, 1.639, 3.497, 16.818},
	{1.310, 3.610, 7.019, 28.711},
	{1.387, 3.773, 7.284, 29.467},
	{1.010, 2.996, 6.093, 27.165},
	{1.013, 2.993, 6.074, 26.991},
	{1.111, 2.916, 5.511, 21.167},
	{0.945, 2.866, 5.904, 26.873},
	{0.857, 2.632, 5.469, 25.287},
	{0.996, 3.008, 6.184, 28.070},
	{0.934, 2.795, 5.694, 25.119},
	{1.230, 3.492, 6.890, 28.814},
	{0.837, 2.555, 5.282, 24.108},
	{0.615, 1.865, 3.850, 17.431},
	{6.823, 22.908, 19.696, 50.871}
};

static float pressEnvMT[][4]= {
	{2.962, 6.809, 5.841, 24.275},
	{4.800, 10.848, 9.332, 38.344},
	{1.639, 4.146, 3.497, 16.818},
	{3.610, 8.157, 7.019, 28.711},
	{3.773, 8.453, 7.284, 29.467},
	{2.996, 7.151, 6.093, 27.165},
	{2.993, 7.126, 6.074, 26.991},
	{2.916, 6.366, 5.511, 21.167},
	{2.866, 6.948, 5.904, 26.873},
	{2.632, 6.449, 5.469, 25.287},
	{3.008, 7.275, 6.184, 28.070},
	{2.795, 6.682, 5.694, 25.119},
	{3.492, 8.031, 6.890, 28.814},
	{2.555, 6.220, 5.282, 24.108},
	{1.865, 4.532, 3.850, 17.431},
	{20.00, 35.000, 40.000, 90.000}
};
static float pressEnvHT[][4]= {	
  {6.809, 9.100, 5.841, 24.275},	
  {10.848, 14.43, 9.332, 38.344},	
  {4.146, 5.717, 3.497, 16.818},	
  {8.157, 10.844, 7.019, 28.711},
	{8.453, 11.209, 7.284, 29.467},
  {7.151, 9.681, 6.093, 27.165},	
  {7.126, 9.64, 6.074, 26.991},	
  {6.366, 8.365, 5.511, 21.167},	
  {6.948, 9.452, 5.904, 26.873},
	{6.449, 8.803, 5.469, 25.287},	
  {7.275, 9.889, 6.184, 28.07},
	{6.682, 9.037, 5.694, 25.119},	
  {8.031, 10.73, 6.89, 28.814},
	{6.220, 8.469, 5.282, 24.108},
	{4.532, 6.168, 3.85, 17.431},
	{20.00, 35.000, 40.000, 90.000}
};

// Saturated Suction Temperature Coefficents (3rd order)
// SST_coeff[][] = {
//	{R22 a3, R22 a2, R22 a1, R22 a0},
//	{R410A a3, R410A a2, R410A a1, R410A a0},
//	{R134A a3, R134A a2, R134A a1, R134A a0},
//	{R404A a3, R404A a2, R404A a1, R404A a0},
//	{R507A a3, R507A a2, R507A a1, R507A a0},
//	{R448A a3, R448A a2, R448A a1, R448A a0},
//	{R449A a3, R449A a2, R449A a1, R449A a0},
//	{R290 a3,  R290 a2,  R290 a1,  R290 a0},
//	{R407A a3, R407A a2, R407A a1, R407A a0},
//	{R407C a3, R407C a2, R407C a1, R407C a0},
//	{R407F a3, R407F a2, R407F a1, R407F a0},
//	{R422D a3, R422D a2, R422D a1, R422D a0},
//	{R422A a3, R422A a2, R422A a1, R422A a0},
//	{R438A a3, R438A a2, R438A a1, R438A a0},
//	{R513A a3, R513A a2, R513A a1, R513A a0},
//	{R744-SC a3, R744-SC a2, R744-SC a1, R744-SC a0},
//	{R744-PC a3, R744-PC a2, R744-PC a1, R744-PC a0},
//	{R744-TC a3, R744-TC a2, R744-TC a1, R744-TC a0}
// }

static float SST_coeff[][4] = {
	{0.42860, -6.1881, 34.637, -72.871},
	{0.39810, -5.7432, 32.016, -80.994},
	{0.44100, -6.3636, 35.544, -58.971},
	{0.41380, -5.9793, 33.558, -76.530},
	{0.41530, -6.0002, 33.649, -77.885},
	{0.41150, -5.9408, 33.187, -70.657},
	{0.41280, -5.9594, 33.293, -70.811},
	{0.44960, -6.4998, 36.613, -76.007},
	{0.40910, -5.9042, 32.953, -69.090},
	{0.41330, -5.9629, 33.258, -67.407},
	{0.40690, -5.8710, 32.751, -69.971},
	{0.41280, -5.9594, 33.293, -70.811},
	{0.40530, -5.8576, 32.908, -74.491},
	{0.41580, -6.0028, 33.550, -67.190},
	{0.44400, -6.4184, 35.963, -62.760},
	{0.00090, -0.0914, 4.2087, -74.218},
	{0.00005, -0.0134, 1.8209, 49.0430},
	{0.00005, -0.0134, 1.8209, 49.0430}
};

// Heat Specific Ratio at 1 bar @ 20ºC
static float eta[] = {	1.185, 	1.175, 	1.119, 	1.118, 	1.117,	1.140, 	1.139, 	136, 	1.138, 	1.144, 	1.152, 	1.139,	1.105,	1.121,	1.107,	1.294,	1.294
};



// ------------------------------------------------------------------------  //
// ----------- S Y M B I O N T E ---- F U N C T I O N S -------------------  //
// -------------------------------start------------------------------------  //
// ------------------------------------------------------------------------  //

// Query from WhatsApp and Twilio
String HandleResponse(String query){
  String * queryOutput;
  // ------------  Help or unknown.
  String helpOrInvalid = "";
  helpOrInvalid.concat("*Your query is invalid. Commands are:*\n");
  helpOrInvalid.concat("\t- *Properties*  for nominal characteristics\n");
  helpOrInvalid.concat("\t- *Envelope* for suction line pressure and temperature, discharge line pressure and temperature, and superheat\n");
  helpOrInvalid.concat("\t- *Electrical* for voltages, currents, average power and power factor \n");
  // ------------- Nominal properties
  String nominalProperties = "";
  nominalProperties.concat("Properties: \n\t - Compressor Type: ");
  nominalProperties.concat(dictCompType[int(compressorProperties[1])]);
  nominalProperties.concat("\n\t - Nominal Power: ");
  nominalProperties.concat(compressorProperties[0]);
  nominalProperties.concat(" HP\n\t - Refrigerant: ");
  nominalProperties.concat(dictRefrigerant[int(compressorProperties[6])]);
  nominalProperties.concat("\n\t - Application: ");
  nominalProperties.concat(dictApp[int(compressorProperties[7])]);
  // ------------- Envelope properties
  String envelopeProperties = "";
  envelopeProperties.concat("Envelope: \n\t - Suction/Discharge Pressures: \n\t ");
  envelopeProperties.concat(pressures_LCD[1]);
  envelopeProperties.concat(" ");
  envelopeProperties.concat(dictUnitsP[int(compressorProperties[8])]);
  envelopeProperties.concat("/ ");
  envelopeProperties.concat(pressures_LCD[0]);
  envelopeProperties.concat(" ");
  envelopeProperties.concat(dictUnitsP[int(compressorProperties[8])]);
  envelopeProperties.concat("\n\t - Suction/Discharge Temps: \n\t ");
  envelopeProperties.concat(temps_LCD[0]);
  envelopeProperties.concat(" ");
  envelopeProperties.concat(dictUnitsT[int(compressorProperties[2])]);
  envelopeProperties.concat("/ ");
  envelopeProperties.concat(temps_LCD[1]);
  envelopeProperties.concat(" ");
  envelopeProperties.concat(dictUnitsT[int(compressorProperties[2])]);
  envelopeProperties.concat("\n\t - Superheat: ");
  envelopeProperties.concat(SH_LCD[0]);
  envelopeProperties.concat(" ");
  envelopeProperties.concat(dictUnitsT_dif[int(compressorProperties[2])]);

// ------------- Electrical properties
  String electricalProperties = "";
  electricalProperties.concat("Electrical: \n\t - Nominal Properties: \n\t\t");
  electricalProperties.concat(dictVolts[int(compressorProperties[3])]);
  electricalProperties.concat("/ ");
  electricalProperties.concat(dictPhase[int(compressorProperties[4])]);
  electricalProperties.concat("/ ");
  electricalProperties.concat(dictFreq[int(compressorProperties[5])]);
  electricalProperties.concat("\n\t\t");
  electricalProperties.concat("- Volts: ");
  electricalProperties.concat(analogInputs[13]);
  electricalProperties.concat(" V/ ");
  electricalProperties.concat(analogInputs[14]);
  electricalProperties.concat(" V/ ");
  electricalProperties.concat(analogInputs[15]);
  electricalProperties.concat(" V");
  electricalProperties.concat("\n\t\t");
  electricalProperties.concat("- Amps: ");
  electricalProperties.concat(analogInputs[0]);
  electricalProperties.concat(" A/ ");
  electricalProperties.concat(analogInputs[1]);
  electricalProperties.concat(" A/ ");
  electricalProperties.concat(analogInputs[2]);
  electricalProperties.concat(" A");
  electricalProperties.concat("\n\t\t");
  electricalProperties.concat("- Power average: ");
  electricalProperties.concat(analogInputs[11]);
  electricalProperties.concat(" kW");
  electricalProperties.concat("\n\t\t");

  electricalProperties.concat("- Power factor: ");
  electricalProperties.concat(analogInputs[12]);
  // Query return
  if (query == "properties") {
    queryOutput = &nominalProperties;
  }
  else if(query == "envelope"){
    queryOutput = &envelopeProperties;
  }
  else if(query == "electrical"){
    queryOutput = &electricalProperties;
  }
  else{
    queryOutput = &helpOrInvalid;
  }
  return (*queryOutput);
}

// Converting temperatures from IP to SI and viceversa
float temps2show(int tempIndex, float temp_SI){
  float tempOutput;
  switch (tempIndex){
    case 0:
      tempOutput = 0;
      break;
    case 1:
      tempOutput = temp_SI;
      break;
    case 2:
      tempOutput = ((9/5)*temp_SI) + 32;
      break;
  }
  return tempOutput;
}

// Converting temperature diference from IP to SI and viceversa
float tempsDiff2show(int tempDiffIndex, float tempDiff_SI){
  float tempDiffOutput;
  switch (tempDiffIndex){
    case 0:
      tempDiffOutput = 0;
      break;
    case 1:
      tempDiffOutput = tempDiff_SI;
      break;
    case 2:
      tempDiffOutput = 1.8*tempDiff_SI;
      break;
  }
  return tempDiffOutput;
}

// Converting pressures from IP to SI and viceversa
float pressures2show(int pressureIndex, float pressure_SI){
  float pressureOutput;
  switch (pressureIndex){
    case 0:
      pressureOutput = 0;
      break;
    case 1:
      pressureOutput = pressure_SI;
      break;
    case 2:
      pressureOutput = pressure_SI * 14.5;
      break;
  }
  return pressureOutput;
}

// Function to change states (Protections)
float toggleState(float state){
  float toggle = 0;
  if(state == 0){
    toggle = 1;
  }
  return toggle;
}

// Read values from the EEPROM
void initValues(){
	preferences.begin("Symbionte", false);
  compressorProperties[0] = preferences.getFloat("param0", false);
  compressorProperties[1] = preferences.getFloat("param1", false);
  compressorProperties[2] = preferences.getFloat("param2", false);
  compressorProperties[3] = preferences.getFloat("param3", false);
  compressorProperties[4] = preferences.getFloat("param4", false);
  compressorProperties[5] = preferences.getFloat("param5", false);
  compressorProperties[6] = preferences.getFloat("param6", false);
  compressorProperties[7] = preferences.getFloat("param7", false);
  compressorProperties[8] = preferences.getFloat("param8", false);
  compressorProperties[9] = preferences.getFloat("param9", false);
  compressorProperties[10] = preferences.getFloat("param10", false);
  compressorProperties[11] = preferences.getFloat("param11", false);
}

// Intro message (Welcome) 
void LCD_Start(){
  LCD_20x4.clear();
  LCD_20x4.home();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("** Symbionte App **");
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(" HVAC-R IoT Systems");
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print("    Diagnosis &");
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print("  Troubleshooting");
}
void LCD_Mech_NominalHP(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Nominal HP");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("HP (near): ");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp >= '0') && (keyInp <= '9')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[0] = float(tempValue);
  preferences.putFloat("param0", compressorProperties[0]);
}
void LCD_Mech_CompTech(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Compressor Tech:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1.Semihermetic");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2.Scroll");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3.Piston hermetic");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '4')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[1] = float(tempValue);
  preferences.putFloat("param1", compressorProperties[1]);
}
void LCD_Mech_UnitsT(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Units for temp: ");
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(" 1. Metric (C)");
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print(" 2. Imperial (F)");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '4')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[2] = float(tempValue);
  preferences.putFloat("param2", compressorProperties[2]);
}
void LCD_Mechanical(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.home();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Mechanical Menu");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1. Nominal HP");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2. Compressor Tech");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3. Units Temp");
  if(menu_level == 2){
    keyInp = 0;
    while(keyInp == '\0'){
      keyInp = Keypad_4x4.getKey();
      if (keyInp == '1' || keyInp == '2' || keyInp == '3'){
        selected_option = keyInp;
        menu_level = 3;
      }
    }
  }
  if(selected_option != ' '){
    switch(selected_option){
      case '1':
        LCD_Mech_NominalHP();
        break;
      case '2':
        LCD_Mech_CompTech();
        break;
      case '3':
        LCD_Mech_UnitsT();
        break;
      default:
        LCD_20x4.clear();
        LCD_20x4.setCursor(0,0);
        LCD_20x4.print("Not Valid!");
        delay(1000);
        LCD_Mechanical();
        break;  
    }
  }
}
void LCD_Elect_Voltage(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Voltage Level:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1. 110-120V");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2. 208-240V");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3. 420-480V");
  LCD_20x4.setCursor(17,0);
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '4')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[3] = float(tempValue);
  preferences.putFloat("param3", compressorProperties[3]);
}
void LCD_Elect_Phases(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Phase Number:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1. Single Phase");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2. Three Phases");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '3')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[4] = float(tempValue);
  preferences.putFloat("param4", compressorProperties[4]);
}
void LCD_Elect_Freq(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Frequency:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1. 50Hz");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2. 60Hz");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '3')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[5] = float(tempValue);
  preferences.putFloat("param5", compressorProperties[5]);
}
void LCD_Electrical(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.home();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Electrical Menu");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1.Voltage Level");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2.Phase Number");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3.Nominal Freq");
  if(menu_level == 2){
    keyInp = 0;
    while(keyInp == '\0'){
      keyInp = Keypad_4x4.getKey();
      if (keyInp == '1' || keyInp == '2' || keyInp == '3'){
        selected_option = keyInp;
        menu_level = 3;
      }
    }
  }
  if(selected_option != ' '){
    switch(selected_option){
      case '1':
        LCD_Elect_Voltage();
        break;
      case '2':
        LCD_Elect_Phases();
        break;
      case '3':
        LCD_Elect_Freq();
        break;
      default:
        LCD_20x4.clear();
        LCD_20x4.setCursor(0,0);
        LCD_20x4.print("Not Valid!");
        delay(1000);
        LCD_Electrical();
        break;  
    }
  }
}
void LCD_RefApp_Application_Screen04(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Refrigerant:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("19.R744-CO2 SubCrt");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("20.R744-CO2 Parall");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("21.R744-CO2 TransC");
  LCD_20x4.setCursor(17,0);
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp >= '0') && (keyInp <= '9')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[6] = float(tempValue);
  preferences.putFloat("param6", compressorProperties[6]);
}
void LCD_RefApp_Application_Screen03(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Refrigerant:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("13.R407F");
  LCD_20x4.setCursor(11,1);
  LCD_20x4.print("16.R438A");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("14.R422D");
  LCD_20x4.setCursor(11,2);
  LCD_20x4.print("17.R513A");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("15.R422A");
  LCD_20x4.setCursor(11,3);
  LCD_20x4.print("18.more");
  LCD_20x4.setCursor(17,0);
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp >= '0') && (keyInp <= '9')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  if(tempValue == 18){
    LCD_RefApp_Application_Screen04();
  }
  else{
    menu_level = 1;
    compressorProperties[6] = float(tempValue);
    preferences.putFloat("param6", compressorProperties[6]);
  }
}
void LCD_RefApp_Application_Screen02(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Refrigerant:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("7.R448A");
  LCD_20x4.setCursor(11,1);
  LCD_20x4.print("10.R407A");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("8.R449A");
  LCD_20x4.setCursor(11,2);
  LCD_20x4.print("11.R407C");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("9.R290");
  LCD_20x4.setCursor(11,3);
  LCD_20x4.print("12.more");
  LCD_20x4.setCursor(17,0);
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp >= '0') && (keyInp <= '9')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  if(tempValue == 12){
    LCD_RefApp_Application_Screen03();
  }
  else{
    menu_level = 1;
    compressorProperties[6] = float(tempValue);
    preferences.putFloat("param6", compressorProperties[6]);
  }
}
void LCD_RefApp_Application_Screen01(){
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Refrigerant:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1.R22");
  LCD_20x4.setCursor(11,1);
  LCD_20x4.print("4.R404A");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2.R410A");
  LCD_20x4.setCursor(11,2);
  LCD_20x4.print("5.R507A");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3.R134a");
  LCD_20x4.setCursor(11,3);
  LCD_20x4.print("6.more");
}
void LCD_RefApp_Application(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("App Envelope:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1.High Temp A/C");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2.Medium Temp");
  LCD_20x4.setCursor(1,3);
  LCD_20x4.print("3.Low/UltraLow Temp");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '4')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[7] = float(tempValue);
  preferences.putFloat("param7", compressorProperties[7]);
}
void LCD_RefApp_UnitsP(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Pressure Units:");
  LCD_20x4.setCursor(1,1);
  LCD_20x4.print("1.Metric (bAr)");
  LCD_20x4.setCursor(1,2);
  LCD_20x4.print("2.Imperial (psig)");
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '3')){
        tempValue = (tempValue*10) + keyInp - 48;
        LCD_20x4.setCursor(17,0);
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  menu_level = 1;
  compressorProperties[8] = float(tempValue);
  preferences.putFloat("param8", compressorProperties[8]);
}
void LCD_RefApp_Refrigerant(){
  tempValue = 0;
  keyInp = 0;
  LCD_RefApp_Application_Screen01();
  LCD_20x4.setCursor(17,0);
  while(keyInp != '#'){
    keyInp = Keypad_4x4.getKey();
    if(keyInp != NO_KEY){
      if((keyInp > '0') && (keyInp < '7')){
        tempValue = keyInp - 48;
        LCD_20x4.print(keyInp - 48);
      }
    }
  }
  if(tempValue == 6){
    LCD_RefApp_Application_Screen02();
  }
  else{
    menu_level = 1;
    compressorProperties[6] = float(tempValue);
    preferences.putFloat("param6", compressorProperties[6]);
  }
}
void LCD_RefrigerantAndApp(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Refrigerant & App");
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(" 1. Refrigerant");
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print(" 2. Application");
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print(" 3. Unit for Press");
  if(menu_level == 2){
    keyInp = 0;
    while(keyInp == '\0'){
      keyInp = Keypad_4x4.getKey();
      if (keyInp == '1' || keyInp == '2' || keyInp == '3'){
        selected_option = keyInp;
        menu_level = 3;
      }
    }
  }
  if(selected_option != ' '){
    switch(selected_option){
      case '1':
        LCD_RefApp_Refrigerant();
        break;
      case '2':
        LCD_RefApp_Application();
        break;
      case '3':
        LCD_RefApp_UnitsP();
        break;
      default:
        LCD_20x4.clear();
        LCD_20x4.setCursor(0,0);
        LCD_20x4.print("Not Valid!");
        delay(1000);
        LCD_RefrigerantAndApp();
        break;  
    }
  }
}
void LCD_Protections(){
  tempValue = 0;
  keyInp = 0;
  LCD_20x4.clear();
  LCD_20x4.home();
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print("Protections");
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(" 1. Voltage (");
  LCD_20x4.print(dictStatus[int(compressorProperties[9])]);  
  LCD_20x4.print(")");
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print(" 2. Amps    (");
  LCD_20x4.print(dictStatus[int(compressorProperties[10])]);  
  LCD_20x4.print(")");
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print(" 3. Temps   (");
  LCD_20x4.print(dictStatus[int(compressorProperties[11])]);  
  LCD_20x4.print(")");
  LCD_20x4.setCursor(0, 17);
  if(menu_level == 2){
    keyInp = 0;
    while(keyInp == '\0'){
      keyInp = Keypad_4x4.getKey();
      if (keyInp == '1' || keyInp == '2' || keyInp == '3' || keyInp == '*'){
        selected_option = keyInp;
      }
    }
  }
  if(selected_option != ' '){
    switch(selected_option){
      case '1':
        compressorProperties[9] = toggleState(float(compressorProperties[9]));
        preferences.putFloat("param9", compressorProperties[9]);
        LCD_20x4.setCursor(13, 1);
        LCD_20x4.print(dictStatus[int(compressorProperties[9])]);
        delay(750);
        break;
      case '2':
        compressorProperties[10] = toggleState(float(compressorProperties[10]));
        preferences.putFloat("param10", compressorProperties[10]);
        LCD_20x4.setCursor(13, 2);
        LCD_20x4.print(dictStatus[int(compressorProperties[10])]);
        delay(750);
        break;
      case '3':
        compressorProperties[11] = toggleState(float(compressorProperties[11]));
        preferences.putFloat("param11", compressorProperties[11]);
        LCD_20x4.setCursor(13, 3);
        LCD_20x4.print(dictStatus[int(compressorProperties[11])]);
        delay(750);
        break;
      case '*':
        menu_level = 1;
        break;  
      default:
        LCD_20x4.clear();
        LCD_20x4.setCursor(0,0);
        LCD_20x4.print("Not Valid!");
        delay(1000);
        LCD_Mechanical();
        break;  
    }
  }
}

//Mechanical Screen
// - Compressor Type
// - Refrigerant
// - Nominal HP
// - Temperature Application
// - Nominal Pressures (Suction & Discharge)
void LCD_root1(){
  LCD_20x4.clear();
  //Compressor Type
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print(dictCompType[int(compressorProperties[1])]);
  //Refrigerant
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(dictRefrigerant[int(compressorProperties[6])]);
  //Nominal Power
  LCD_20x4.setCursor(8,1);
  LCD_20x4.print(compressorProperties[0],1);
  LCD_20x4.print(" HP"); 
  //Application Envelope
  LCD_20x4.setCursor(17,1);
  LCD_20x4.print(dictApp[int(compressorProperties[7])]);
  //Pressures
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print("SSP = ");
  LCD_20x4.print(pressures_LCD[1], 1);
  LCD_20x4.print(" ");
  LCD_20x4.print(dictUnitsP[int(compressorProperties[8])]);
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print("SCP = ");
  LCD_20x4.print(pressures_LCD[0], 1);
  LCD_20x4.print(" ");
  LCD_20x4.print(dictUnitsP[int(compressorProperties[8])]);
}

//Mechanical Screen 02
// - Compressor Type
// - Refrigerant
// - Nominal HP
// - Temperature Application
// - Nominal Temperatures (Suction & Discharge)
void LCD_root2(){
  LCD_20x4.clear();
  //Compressor Type
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print(dictCompType[int(compressorProperties[1])]);
  //Refrigerant
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(dictRefrigerant[int(compressorProperties[6])]);
  //Nominal Power
  LCD_20x4.setCursor(8,1);
  LCD_20x4.print(compressorProperties[0],1);
  LCD_20x4.print(" HP"); 
  //Application Envelope
  LCD_20x4.setCursor(17,1);
  LCD_20x4.print(dictApp[int(compressorProperties[7])]);
  //Temps
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print("SLT =  ");
  LCD_20x4.print(temps_LCD[0], 1);
  LCD_20x4.print(" ");
  LCD_20x4.print((char)223);
  LCD_20x4.print(dictUnitsT[int(compressorProperties[2])]);
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print("DLT =  ");
  LCD_20x4.print(temps_LCD[1], 1);
  LCD_20x4.print(" ");
  LCD_20x4.print((char)223);
  LCD_20x4.print(dictUnitsT[int(compressorProperties[2])]);
}

//Electrical Screen
// - Nominal Voltage/ Phases/ Frequency
// - Voltages
// - Currents
// - Average Power Comsumption / Average Power Factor
void LCD_root3(){
  LCD_20x4.clear();
  //Nominal Electrical Properties
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print(dictVolts[int(compressorProperties[3])]);
  LCD_20x4.print("/ ");
  LCD_20x4.print(dictPhase[int(compressorProperties[4])]);
  LCD_20x4.print("/ ");
  LCD_20x4.print(dictFreq[int(compressorProperties[5])]);
  //Voltages
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print("V = ");
  LCD_20x4.print(analogInputs[13],0);
  LCD_20x4.print("V ");
  LCD_20x4.print(analogInputs[14],0);
  LCD_20x4.print("V ");
  LCD_20x4.print(analogInputs[15],0);
  LCD_20x4.print("V ");
  //Amps
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print("A = ");
  LCD_20x4.print(analogInputs[0],0);
  LCD_20x4.print("A ");
  LCD_20x4.print(analogInputs[1],0);
  LCD_20x4.print("A ");
  LCD_20x4.print(analogInputs[2],0);
  LCD_20x4.print("A ");
  //Power
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print("P = ");
  LCD_20x4.print(analogInputs[11], 1);
  LCD_20x4.print("kW PF= ");
  LCD_20x4.print(analogInputs[12],2);
}

//Mechanical Screen
// - Compressor Type
// - Refrigerant
// - Nominal HP
// - Temperature Application
// - Nominal Pressures (Suction & Discharge)
void LCD_root4(){
  LCD_20x4.clear();
  //Compressor Type
  LCD_20x4.setCursor(0,0);
  LCD_20x4.print(dictCompType[int(compressorProperties[1])]);
  //Refrigerant
  LCD_20x4.setCursor(0,1);
  LCD_20x4.print(dictRefrigerant[int(compressorProperties[6])]);
  //Nominal Power
  LCD_20x4.setCursor(8,1);
  LCD_20x4.print(compressorProperties[0],1);
  LCD_20x4.print(" HP"); 
  //Application Envelope
  LCD_20x4.setCursor(17,1);
  LCD_20x4.print(dictApp[int(compressorProperties[7])]);
  //Temps & Status
  LCD_20x4.setCursor(0,2);
  LCD_20x4.print("SH =  ");
  LCD_20x4.print(SH_LCD[0], 1);
  LCD_20x4.print(" ");
  LCD_20x4.print(dictUnitsT_dif[int(compressorProperties[2])]);
  LCD_20x4.setCursor(0,3);
  LCD_20x4.print("Status =  ");
  LCD_20x4.print(dictStatus[status]);
}

void LCD_root(){
  temps_LCD[0] = temps2show(int(compressorProperties[2]), analogInputs[3]);
  temps_LCD[1] = temps2show(int(compressorProperties[2]), analogInputs[4]);
  SH_LCD[0] = tempsDiff2show(int(compressorProperties[2]), SH_s[0]);
  pressures_LCD[0] = pressures2show(int(compressorProperties[8]), analogInputs[5]);
  pressures_LCD[1] = pressures2show(int(compressorProperties[8]),  analogInputs[6]);
  t2 = millis();
  if((t2-t1) >= 3000){
    switch (screenCounter){
      case 1:
        LCD_root1();
        break;
      case 2:
        LCD_root2();
        break;
      case 3:
        LCD_root3();
        break;
      case 4:
        LCD_root4();
        break;
  }
    screenCounter++;
    t1=t2;
  }
  if(screenCounter>4){
    screenCounter = 1;
  }
}

float getCO2ppm(int analogPin){
  MQ135 gasSensor = MQ135(analogPin);
  float Y = 0;
  float alpha = 0.05;
  float ppm = Y;
  int counter = 0;
  int samples = 100;
  while(counter <= samples){
    float Y = gasSensor.getPPM();
    ppm = (alpha*Y) + ((1-alpha)*ppm);
    counter++;
  }
  return ppm;
}

float getAC_voltage(int AnalogPin, float slopeV, float interceptV){
  int maxVal = 0;
  int minVal = 4096;
  int range = 0;
  int rangeCummulative = 0;
  int samplesAC = 500;
  int sampleStatistics = 100;
  float Voltage = 0;
  int rawSamples[500];              // Same index than samplesAC
  for(int j=0; j<sampleStatistics; j++){
    for(int i=0; i<samplesAC; i++){
      rawSamples[i] = analogRead(AnalogPin);
      if(rawSamples[i] < minVal){
        minVal = rawSamples[i];
      }
      if(rawSamples[i] > maxVal){
        maxVal = rawSamples[i];
      }
    }
    range = maxVal - minVal;
    rangeCummulative = rangeCummulative + range;
    maxVal = 0;
    minVal = 4096;
  }
  Voltage = slopeV*float(rangeCummulative/sampleStatistics) + interceptV;
  return Voltage;
}

float getAC_current(int AI_pinPort, float slopeA, float interceptA){
  int maxVal=-65535;
  int minVal=65535;
  int range = 0;
  int rangeCummulative = 0;
  int samplesAC = 20;
  int sampleStatistics = 10;
  int rawSamples[20];
  float Amps;
  for(int j=0; j<sampleStatistics; j++){
    for(int i=0; i<samplesAC; i++){
      rawSamples[i] = ads_Amps.readADC_SingleEnded(AI_pinPort);
      if(rawSamples[i] < minVal){
        minVal = rawSamples[i];
      }
      if(rawSamples[i] > maxVal){
        maxVal = rawSamples[i];
      }
    }
    range = maxVal - minVal;
    rangeCummulative = rangeCummulative + range;
    maxVal = -65535;
    minVal = 65535;
  }
  Amps = slopeA*float(rangeCummulative/sampleStatistics) + interceptA;
  return Amps;
}

float getTemperature(int analogPin){
  float Y = 0;
  float alpha = 0.0025;
  float S = Y;
  float S_cummul = 0;
  int counter = 0;
  int samples = 20000;
  float temperature;
  while(counter < samples){
    Y = (float)analogRead(AN01);
    S = (alpha*Y)+((1-alpha)*S);
    S_cummul = S_cummul + S;
    counter++;
  }
  S_cummul = S_cummul/samples;
  if(S_cummul <= 840){
    temperature = a3_T[0]*pow(S_cummul,3) + a2_T[0]*pow(S_cummul,2) + a1_T[0]*S_cummul + a0_T[0];
  }
  if((S_cummul > 840) && (S_cummul < 2460)){
    temperature = a3_T[1]*pow(S_cummul,3) + a2_T[1]*pow(S_cummul,2) + a1_T[1]*S_cummul + a0_T[1];
  }
  if(S_cummul >= 2460){
    temperature = a3_T[2]*pow(S_cummul,3) + a2_T[2]*pow(S_cummul,2) + a1_T[2]*S_cummul + a0_T[2];
  }
  //temperature = S_cummul;
  return temperature;
}

float getPressure(int AnalogPin, float a2, float a1, float a0, float bias){
  float Y = 0;
  float alpha = 0.07;
  float S = Y;
  float S_cummul = 0;
  int counter = 0;
  int samples = 1000;
  float pressure;
  float slopePressure;
  while(counter < samples){
    Y = (float)analogRead(AN01);
    S = (alpha*Y)+((1-alpha)*S);
    S_cummul = S_cummul + S;
    counter++;
  }
  S_cummul = S_cummul/samples;
  //pressure = S_cummul;
  pressure = a2*pow(S_cummul, 2) + a1*S_cummul + a0 + bias;
  return pressure;
}

float getModeStatistics(float arraySamples[], int samples){
  float rangeAmplitude = arraySamples[samples-1] - arraySamples[0];
  int bins = 10;
  float range[bins+1];
  float relativeFrequences[bins];
  for(int i = 0; i <= bins; i++){
    range[i] = arraySamples[0] + i*rangeAmplitude;
  }
  // Filling relative frequences
  for(int j = 0; j < samples; j++){
    if((arraySamples[j]>=range[0]) && (arraySamples[j]<range[1])){
      relativeFrequences[0]++; 
    }
    if((arraySamples[j]>=range[1]) && (arraySamples[j]<range[2])){
      relativeFrequences[1]++; 
    }
    if((arraySamples[j]>=range[2]) && (arraySamples[j]<range[3])){
      relativeFrequences[2]++; 
    }
    if((arraySamples[j]>=range[3]) && (arraySamples[j]<range[4])){
      relativeFrequences[3]++; 
    }
    if((arraySamples[j]>=range[4]) && (arraySamples[j]<range[5])){
      relativeFrequences[4]++; 
    }
    if((arraySamples[j]>=range[5]) && (arraySamples[j]<range[6])){
      relativeFrequences[5]++; 
    }
    if((arraySamples[j]>=range[6]) && (arraySamples[j]<range[7])){
      relativeFrequences[6]++; 
    }
    if((arraySamples[j]>=range[7]) && (arraySamples[j]<range[8])){
      relativeFrequences[7]++; 
    }
    if((arraySamples[j]>=range[8]) && (arraySamples[j]<range[9])){
      relativeFrequences[8]++; 
    }
    if((arraySamples[j]>=range[9]) && (arraySamples[j]<=range[10])){
      relativeFrequences[9]++; 
    }
  }
  // Finding maximum
  int index = 0;
  float maxFreq = 0;
  for(int k = 0; k < bins; k++){
    if(relativeFrequences[k] > maxFreq){
      maxFreq = relativeFrequences[k];
      index = k;
    }
  }
  if(index == 0){
    index = 1;
  }
  if(index == bins){
    index = bins - 1;
  }
  float Li = range[index];
  float fi = relativeFrequences[index];
  float fi_bef = relativeFrequences[index-1];
  float fi_aft = relativeFrequences[index+1];
  float mode =  Li + ((fi-fi_bef)/((fi-fi_bef)+(fi-fi_aft)))*rangeAmplitude; 
  return mode;
}

float getVibrationMode(){
  float mode;
  int counter = 0;
  int samples = 100;
  float vibrationMagnitude[samples];
  // Getting samples
  while(counter<samples){
    mpu.getEvent(&a, &g, &temp);
    vibrationMagnitude[counter] = sqrt(pow(a.acceleration.x,2) + pow(a.acceleration.y,2) + pow(a.acceleration.y,2));
    counter++;
  }
  // Sorting array for statistics
  for(int i=0; i<(samples-1); i++){
    for(int j=0; j<(samples-(i+1)); j++){
      if(vibrationMagnitude[j] > vibrationMagnitude[j+1]) {
        float t = vibrationMagnitude[j];
        vibrationMagnitude[j] = vibrationMagnitude[j+1];
        vibrationMagnitude[j+1] = t;
      }
    }
  }
  mode = getModeStatistics(vibrationMagnitude, samples);
  //mode = vibrationMagnitude[samples-1];
  return mode;
}

float getCircuitTemp(){
  mpu.getEvent(&a, &g, &temp);
  return temp.temperature;
}

float FilterPressure(float Pin){
  float Pout = 0;
  if(Pin<0){ 
    Pout = 0;
  }
  if(Pin>35){ 
    Pout = 35;
  }
  return Pout;
}

float FilterVoltage(float Vin){
  float Vout = 0;
  if(Vin<80){ 
    Vout = 0;
  }
  return Vout;
}

float FilterCurrent(float Ain){
  float Aout = 0;
  if(Ain<0){ 
    Aout = 0;
  }
  return Aout;
}

int Ref2Index(float refrigerant){
  int refrigerantIndex = int(refrigerant);
  int Ref2Index=0;
	// Know the row in which is the refrigerant limits
	switch (refrigerantIndex){
		// R22
    case 1:
			Ref2Index=0;
			break;
		// R410A
    case 2:
		  Ref2Index=1;
			break;
    // R134a
    case 3:
			Ref2Index=2;
			break;
    // R404A
    case 4:
			Ref2Index=3;
			break;
    // R507A
    case 5:
			Ref2Index=4;
			break;
    // R448A
    case 6:
			Ref2Index=5;
			break;
    // R449A
    case 8:
			Ref2Index=6;
			break;
    // R290
    case 9:
			Ref2Index=7;
			break;
    // R407A
    case 10:
			Ref2Index=8;
			break;
    // R407C
    case 11:
			Ref2Index=9;
			break;
    // R407F
    case 13:
			Ref2Index=10;
			break;
    // R422D
    case 14:
			Ref2Index=11;
			break;
    // R422A
    case 15:
			Ref2Index=12;
			break;
    // R438A
    case 16:
			Ref2Index=13;
			break;
    // R513A
    case 17:
			Ref2Index=14;
			break;
    // CO2 - SC
    case 19:
			Ref2Index=15;
			break;
    // CO2 - TC
    case 20:
			Ref2Index=16;
			break;
    // CO2 - PC
    case 21:
			Ref2Index=17;
			break;

	}
  return Ref2Index;
}

int Env2Index(float envType){
	int envTypeIndex = int(envType);
  int Env2Index = 0;
  // Know the row in which are the refrigerant limits
	switch(envTypeIndex){
		// LT
    case 1:
			Env2Index = 3;
			break;
    // MT  
		case 2:
			Env2Index = 2;
			break;
    // HT
		case 3:
			Env2Index = 1;
			break;
	}
  return Env2Index;
}

void MUX_inputs(int index) {
  switch (index) {
    case 0:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, LOW);
      break;
    case 1:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, HIGH);
      break;
    case 2:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, LOW);
      break;
    case 3:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, HIGH);
      break;
    case 4:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, LOW);
      break;
    case 5:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, HIGH);
      break;
    case 6:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, LOW);
      break;
    case 7:
      digitalWrite(MUX01, LOW);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, HIGH);
      break;
    case 8:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, LOW);
      break;
    case 9:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, HIGH);
      break;
    case 10:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, LOW);
      break;
    case 11:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, LOW);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, HIGH);
      break;
    case 12:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, LOW);
      break;
    case 13:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, LOW);
      digitalWrite(MUX04, HIGH);
      break;
    case 14:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, LOW);
      break;
    case 15:
      digitalWrite(MUX01, HIGH);
      digitalWrite(MUX02, HIGH);
      digitalWrite(MUX03, HIGH);
      digitalWrite(MUX04, HIGH);
      break;
  }
}

float SH_calc(float P1, float T1, int indexA){
	float SH_calc = 0;
	float SST = SST_coeff[indexA][0]*pow(P1,3) + SST_coeff[indexA][1]*pow(P1,2) + SST_coeff[indexA][2]*P1 + SST_coeff[indexA][3];
	SH_calc = (T1 - SST);
	return SH_calc;
}

float DLT_estimated(float P1, float P2, float T1, int indexA){
	float DLT_estimated = 0;
	DLT_estimated = T1*pow(((P2-1.01)/(P1-1.01)), eta[indexA]);
	return DLT_estimated;
}

float envelopeProtection(float P1, float P2, int envType, int indexA){
	const float *pressureLimits;
	float  pressureError[2] = {0,0};
  float pressureErrorindex = 0;
	// Pointer to specific vector application
	switch (envType) {
		case 1:
			pressureLimits = &pressEnvLT[0][0];
			break;
		case 2:
			pressureLimits = &pressEnvMT[0][0];
			break;
		case 3:
			pressureLimits = &pressEnvHT[0][0];
			break;
	}
	// Filling error according to the zone in which is located the operating point
	if(P1 < *(pressureLimits + 4 * indexA + 0)){
		pressureError[0] = -1;
	}
	if(P1 > *(pressureLimits + 4 * indexA + 1)){
		pressureError[0] = 1;
	}
	if(P2 < *(pressureLimits + 4 * indexA + 2)){
		pressureError[1] = -1;
	}
	if(P2 > *(pressureLimits + 4 * indexA + 3)){
		pressureError[1] = 1;
	}
	// Returning error value
	if((pressureError[0] == 0) && (pressureError[1] == 0)){
		// Normal Operation
		pressureErrorindex = 10;
	}
	if((pressureError[0] == -1) && (pressureError[1] == 1)){
		// Low Evaporation Press and High Condensation Press
		pressureErrorindex = 11;
	}
	if((pressureError[0] == 0) && (pressureError[1] == 1)){
		// High Condensation Press
		pressureErrorindex = 12;
	}
	if((pressureError[0] == 1) && (pressureError[1] == 1)){
		// High Evaporation Press and High Condensation Press
		pressureErrorindex = 13;
	}
	if((pressureError[0] == 1) && (pressureError[1] == 0)){
		// High Evaporation Press
		pressureErrorindex = 14;
	}
	if((pressureError[0] == 1) && (pressureError[1] == -1)){
		// High Evaporation Press and Low Condensation Press
		pressureErrorindex = 15;
	}
	if((pressureError[0] == 0) && (pressureError[1] == -1)){
		// Low Condensation Press
		pressureErrorindex = 16;
	}
	if((pressureError[0] == -1) && (pressureError[1] == -1)){
		// Low Evaporation Press and Low Condensation Press
		pressureErrorindex = 17;
	}
	if((pressureError[0] == -1) && (pressureError[1] == 0)){
		// Low Evaporation Press
		pressureErrorindex = 18;
	}
  return pressureErrorindex;
}

float tempProtection(float SLT, float DLT, float DLT_estimated, int indexA, int envType){
	float SLT_min, SLT_max;
	float DLT_max = 100;
	float tempError[2] = {0,0};
  float tempErrorIndex = 0;
	switch (envType){
		case 1:
			SLT_min = -40;
			SLT_max = -15;
			break;
		case 2:
			SLT_min = -15;
			SLT_max = 10;
			break;
		case 3:
			SLT_min = 10;
			SLT_max = 25;
			break;
	}
	// Filling error according to the zone in which is located the operative point
	if(SLT < SLT_min){
		tempError[0] = -1;
	}
	if(SLT > SLT_max){
		tempError[0] = 1;
	}
	if(DLT < DLT_estimated){
		tempError[1] = -1;
	}
	if(DLT > DLT_max){
		tempError[1] = 1;
	}
	// Returning error value
	if((tempError[0] == 0) && (tempError[1] == 0)){
		// Normal Operation
		tempErrorIndex = 60;
	}
	if((tempError[0] == -1) && (tempError[1] == 1)){
		// Low Suction Temp and High Discharge Temp
		tempErrorIndex = 61;
	}
	if((tempError[0] == 0) && (tempError[1] == 1)){
		// High Discharge Temp
		tempErrorIndex = 62;
	}
	if((tempError[0] == 1) && (tempError[1] == 1)){
		// High Suction Temp and High Discharge Temp
		tempErrorIndex = 63;
	}
	if((tempError[0] == 1) && (tempError[1] == 0)){
		// High Suction Temp
		tempErrorIndex = 64;
	}
	if((tempError[0] == 1) && (tempError[1] == -1)){
		// High Suction Temp and Low Discharge Temp
		tempErrorIndex = 65;
	}
	if((tempError[0] == 0) && (tempError[1] == -1)){
		// Low Discharge Temp
		tempErrorIndex = 66;
	}
	if((tempError[0] == -1) && (tempError[1] == -1)){
		// Low Suction Temp and Low Discharge Temp
		tempErrorIndex = 67;
	}
	if((tempError[0] == -1) && (tempError[1] == 0)){
		// Low Suction Temp
		tempErrorIndex = 68;
	}
  return tempErrorIndex;
}

float SHProtection(float SH_actual, float SH_prev, float DLT_estimated, float DLT_real, int indexA, int status){
	float SH_min = 11;
	float SH_max = 21;
	float SH_Error = 0;
  float SH_ErrorIndex = 0;
	if(indexA == 15){
		SH_min = 20; // For CO2-SC
		SH_max = 30;
	}
	// Filling error according to the zone in which is located the operating point
	if(status){
		if((SH_actual < SH_min) && (DLT_real <= DLT_estimated)){
			if(SH_actual <= SH_prev){
				SH_ErrorIndex = 31;
			}
		}
		if((SH_actual > SH_max) && (DLT_real >= DLT_estimated)){
			if(SH_actual >= SH_prev){
				SH_ErrorIndex = 32;
			}
		}
	}
	else{
		SH_ErrorIndex = 30;
	}
  return SH_ErrorIndex;
}

float propertyUmbalance(float X1, float X2, float X3, float phaseNumber, float propertyLimit, float baseError){
	float X_avrg = (X1 + X2 + X3)/phaseNumber;
	float X_epsilon[3];
	X_epsilon[0] = abs(X_avrg - X1);
	X_epsilon[1] = abs(X_avrg - X2);
	X_epsilon[2] = abs(X_avrg - X3);
	if(X_epsilon[0] <= X_epsilon[1]){
		float temp = X_epsilon[0];
		X_epsilon[0] = X_epsilon[1];
		X_epsilon[1] = temp;
	}
	if(X_epsilon[0] <= X_epsilon[2]){
		float temp = X_epsilon[0];
		X_epsilon[0] = X_epsilon[2];
		X_epsilon[2] = temp;
	}
	if((X_epsilon[0]/X_avrg) >= propertyLimit){
		return baseError + 2;
	}
	else{
		return baseError;
	}
}

float missingPhase(float V1, float V2, float V3, float phaseNumber){
	int counter = 0;
	if(V1 > 0){
		counter++;
	}
	if(V2 > 0){
		counter++;
	}
	if(V3 > 0){
		counter++;
	}
	if(counter != phaseNumber){
		return 41;
	}
	else{
		return 40;
	}
}

float highAmps(float A1, float A2, float A3, float phaseNumber, float HP, float freq, float nominalVoltage){
	float A_theor = (1.2499*HP + 1.9847)*(440/nominalVoltage)*(3/phaseNumber)*(freq/60);
	float A_avrg = (A1 + A2 + A3)/phaseNumber;
	if(A_avrg > 1.25*A_theor){
		return 51;
	}
	else{
		return 50;
	}
}

float leakingProtection(float vib, float CO2_ppm){
	float vibLimit = 13.5; 		// Magnitude of acceleration in the second axis w gravity.
	float CO2_ppmLimit = 450; 	// CO2 ppm atmosphere (nominal)
	if((CO2_ppm <= 1.15*CO2_ppmLimit) && (vib >= 1.15*vibLimit)){
		return 71;
	}
	if((CO2_ppm >= 1.15*CO2_ppmLimit) && (vib >= 1.15*vibLimit)){
		return 72;
	}
	if((CO2_ppm >= 1.15*CO2_ppmLimit) && (vib <= 1.15*vibLimit)){
		return 73;
	}
	else{
		return 70;
	}
}

int toggleOutput(float out){
	int toggleOutput;
	if(out == 0){
		toggleOutput = 1;
	}
	else{
		toggleOutput = 0;
	}	
	return toggleOutput;
}

void savingErrors(float error[]){
}

void menuStatesMachine(){
  if(menu_level == 1){
    //SignalInputReadings();
    LCD_root();
    Symbiothing.Handle();
    keyInp = 0;
    if(keyInp == '\0'){
      keyInp = Keypad_4x4.getKey();
      if (keyInp == 'A' || keyInp == 'B' || keyInp == 'C' || keyInp == 'D' ){
        selected_option = keyInp;
        menu_level = 2;
      }
    }
  }
  if(selected_option != ' '){
    switch(selected_option){
      case 'A':
        LCD_Mechanical();
        break;
      case 'B':
        LCD_Electrical();
        break;
      case 'C':
        LCD_RefrigerantAndApp();
        break;
      case 'D':
        LCD_Protections();
        break;  
      default:
        menu_level = 1;
        break; 
    }
  }
}


float getPowerAverage(float V1, float V2, float V3, float A1, float A2, float A3, float Phases){
  float V_avrg, A_avrg, Pow;
  V_avrg = (V1 + V2 + V3)/Phases;
  A_avrg = (A1 + A2 + A3)/Phases;
  Pow = V_avrg * A_avrg;
  return Pow;
}

void SignalInputReadings_Simulate(){
  for(int ind=0; ind<=15; ind++){
    // Amps
    if((ind>=0) && (ind<=2)){
      analogInputs[ind] = 1.03*float(random(9, 14));
    }
    // Temperatures
    if((ind==3)){
      analogInputs[ind] = 1.03*float(random(2, 5));
    }
    if((ind==4)){
      analogInputs[ind] = 1.03*float(random(40, 70));
    }
    // Pressures
    if((ind==6)){
      analogInputs[ind] = 1.03*float(random(4, 6));
    }
    if((ind==5)){
      analogInputs[ind] = 1.03*float(random(9, 15));
    }
    if((ind==7)){
      analogInputs[ind] = 0;
    }
    // CO2 ppm
    if(ind==8){
      analogInputs[ind] = 1.03*float(random(380, 460));
    }
    // Circuit Temperature
    if(ind==9){
      analogInputs[ind] = 1.03*float(random(22, 28));
    }
    // Acceleration Magnitude
    if(ind==10){
      analogInputs[ind] = 1.03*float(random(9, 16));
    }
    // Power Average
    if(ind==11){
      analogInputs[ind] = getPowerAverage(analogInputs[13], analogInputs[14], analogInputs[15], analogInputs[0], analogInputs[1], analogInputs[2], 3);
      analogInputs[ind] = analogInputs[ind] / 1000;
    }
    // Power Factor
    if(ind==12){
      analogInputs[ind] = 0.86;
    }
    // Voltages
    if((ind>=13) && (ind<=15)){
      analogInputs[ind] = 1.03*float(random(212, 224));
    }
    // Superheat
    if(ind==16){
      analogInputs[ind+1] = analogInputs[ind];
      analogInputs[ind] = SH_calc(analogInputs[5], analogInputs[3], Ref2Index(compressorProperties[6]));
    }
  }	
}

void SignalInputReadings(){
  for(int ind=0; ind<=15; ind++){
    // Amps
    if((ind>=0) && (ind<=2)){
      analogInputs[ind] = getAC_current(A1_port, slopesAC_A[ind], interceptsAC_A[ind]);
      analogInputs[ind] = FilterCurrent(analogInputs[ind]);
    }
    // Temperatures
    if((ind>=3) && (ind<=4)){
      MUX_inputs(ind);
      analogInputs[ind] = getTemperature(AN01);
    }
    // Pressures
    if((ind>=5) && (ind<=7)){
      MUX_inputs(ind);
      analogInputs[ind] = getPressure(AN01, a2_P[ind-6], a1_P[ind-6], a0_P[ind-6], 0);
      analogInputs[ind] = FilterPressure(analogInputs[ind]);
    }
    // CO2 ppm
    if(ind==8){
      MUX_inputs(ind);
      analogInputs[ind] = getCO2ppm(AN01);
    }
    // Circuit Temperature
    if(ind==9){
      analogInputs[ind] = getCircuitTemp();
    }
    // Acceleration Magnitude
    if(ind==10){
      analogInputs[ind] = getVibrationMode();
    }
    // Power Average
    if(ind==11){
      analogInputs[ind] = getPowerAverage(analogInputs[13], analogInputs[14], analogInputs[15], analogInputs[0], analogInputs[1], analogInputs[2], compressorProperties[4]);
    }
    // Power Factor
    if(ind==12){
      //analogInputs[ind] = getPowerFactor();
      analogInputs[ind] = 0.86;

    }
    // Voltages
    if((ind>=13) && (ind<=15)){
      MUX_inputs(ind);
      analogInputs[ind] = getAC_voltage(AN01, slopesAC_V[ind-13], interceptsAC_V[ind-13]);
      analogInputs[ind] = FilterVoltage(analogInputs[ind]);
    }
    // Superheat
    if(ind==16){
      analogInputs[ind+1] = analogInputs[ind];
      analogInputs[ind] = SH_calc(analogInputs[5], analogInputs[3], Ref2Index(compressorProperties[6]));
    }
  }	
}

void readingPlusSending(){
    int simulator = 1;
    if (simulator == 1){
      SignalInputReadings_Simulate();
    }
    else{
      SignalInputReadings();
    }
    getSignals(
      analogInputs[0], 
      analogInputs[1], 
      analogInputs[2], 
      analogInputs[3], 
      analogInputs[4],
      analogInputs[5], 
      analogInputs[6],
      analogInputs[7], 
      analogInputs[8], 
      analogInputs[9], 
      analogInputs[10], 
      analogInputs[11], 
      analogInputs[12], 
      analogInputs[13], 
      analogInputs[14], 
      analogInputs[15], 
      analogInputs[16]
    );
    getFailures(
      FailureCount[0],
      FailureCount[1], 
      FailureCount[2], 
      FailureCount[3], 
      FailureCount[4], 
      FailureCount[5], 
      FailureCount[6], 
      FailureCount[7], 
      FailureCount[8], 
      FailureCount[10],
      FailureCount[11], 
      FailureCount[12], 
      FailureCount[13], 
      FailureCount[14], 
      FailureCount[15], 
      FailureCount[16], 
      FailureCount[17],
      FailureCount[18], 
      FailureCount[20],
      FailureCount[21], 
      FailureCount[22],
      FailureCount[30], 
      FailureCount[31], 
      FailureCount[32],
      FailureCount[40], 
      FailureCount[41], 
      FailureCount[42],
      FailureCount[50],
      FailureCount[51], 
      FailureCount[52], 
      FailureCount[53], 
      FailureCount[54], 
      FailureCount[55], 
      FailureCount[56], 
      FailureCount[57],
      FailureCount[58], 
      FailureCount[60],
      FailureCount[61], 
      FailureCount[62],
      FailureCount[63]
    );
    Serial.println("\t\t\t\t\t\t\t\t Signals and Failures Readed");
}

void loop2(void *parameter){
  while(1){
    menuStatesMachine();
  };
}

// ------------------------------------------------------------------------  //
// ----------- S Y M B I O N T E ---- F U N C T I O N S -------------------  //
// --------------------------------end-------------------------------------  //
// ------------------------------------------------------------------------  //
