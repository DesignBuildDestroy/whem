#ifndef cbi
#define cbi(sfr,bit)(_SFR_BYTE(sfr)&=~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr,bit)(_SFR_BYTE(sfr)|=_BV(bit))
#endif


void setup() {
  sbi(ADCSRA,ADPS2);
  sbi(ADCSRA,ADPS1);
  cbi(ADCSRA,ADPS0);
 
  Serial2.begin(115200);
  analogReference(EXTERNAL);
  analogRead(0);  
  Serial2.println("STARTUP");
}

 int vPin = 1;
 int iPin = 3;
 double avgVa = 0;
 double avgIa = 0;
 double avgVb = 0;
 double avgIb = 0;
 double avgPa = 0;
 double avgPb = 0;
 double avgPFa = 0;
 double avgPFb = 0;
 double avgAPa = 0;
 double avgAPb = 0;
 int avgCount = 0;

//*************************8
//Each loop swaps the phase leg being captured, instead of capturing all 4 ADC values at a time which
//then has to trade off number of samples than can be captured due to memory limitations of the Mega
//forcing us to make the array sizes 1/4 of what they are here.
  
void loop() {
  const int SupplyVoltage = 3323;   //ADC External Reference 3.325

  double realPower=0,
      apparentPower=0,
      powerFactor=0,
      Vrms=0,
      Irms=0,
      instP=0,
      sumP=0;
     


    //Calibration coefficients
    //These need to be set in order to obtain accurate results
    //Hook both to same source Phase, Adjust VCALa,ICALa to get matching real world values
    //Then adjust VCALb, ICALb, to match output result of VRMS,IRMS of Phase A
    //Since each phase is run through independent of the other, this difference is not an issue
    //of phase calibration but of the individual components in the circuit that are slightly off value.
    //Therefore the calbiration values below will be slightly different from each other to produce the same result.
    //The values should not be massively different from each other though, if so there is probably a hardware issue(?)
    // (Expected Value/ Measured) * vcal
    double VCALa = 523.5888; // 510.328;
    double ICALa = 62.6254; //68;
    double VCALb = 511.4073; //510.328;
    double ICALb = 53.4710; //68;

    
    
    //--------------------------------------------------------------------------------------
    // Variable declaration for emon_calc procedure
    //--------------------------------------------------------------------------------------
    int sampleV=0;                        //sample_ holds the raw analog read value
    int sampleI=0;
  
    double lastfilteredVa=0;

    int samples = 950; //Number of samples to take, approx 18 sine cycles
    double Va[samples];
    double Ia[samples];
  
    double offsetV = 512;                          //Low-pass filter output
    double offsetI = 512;                          //Low-pass filter output
  
    double phaseShiftedVa =0;                             //Holds the calibrated phase shifted voltage.
  
    double sumV =0;
    double sumI = 0;
 
  
 

  //-------------------------------------------------------------------------------------------------------------------------
  // 1) Waits for the waveform to be close to 'zero' (mid-scale adc) part in sin curve.
  //-------------------------------------------------------------------------------------------------------------------------
  unsigned long start = millis();    //millis()-start makes sure it doesnt get stuck in the loop if there is an error.

  if (vPin == 0)
  {
    vPin = 1;
    iPin = 3;
    VCALa=VCALb;
    ICALa=ICALb;
  }
  else {
    vPin = 0;
    iPin = 2;
  }
  
  while(1)                                   //the while loop...
  {
    sampleV = analogRead(vPin);                    //using the voltage waveform
    sampleI = analogRead(vPin);
    if ((sampleV < (1024*0.55)) && (sampleV > (1024*0.45))){
      if(sampleI > sampleV) break;  //check its within range and on up swing
    }
    //if ((millis()-start)>2000) break;   //Timeout 
  }

  //-------------------------------------------------------------------------------------------------------------------------
  // 2) Main measurement loop
  //-------------------------------------------------------------------------------------------------------------------------
  start = millis();

  //Capture the samples
  for (int i=0; i<samples;++i)
  {
    sampleV = analogRead(vPin);     //Read raw phaseA voltage signal
    sampleI = analogRead(iPin);     //Read raw phaseA current signal
    Va[i]=sampleV;
    Ia[i]=sampleI;
   
  }

  //We started on an upswing so now start from the end of the array
  //and move backwards looking for the first 0 cross (or close to it)
  //we'll drop everything after that point so that we have whole half cycles
  for (int i=samples; i > 1;--i)
  {
    //Check its within range and if so stop here and set samples to new length 
    //Basically ignoring the remaining values for the rest of the calculations to follow
    if ((Va[i] < (1024*0.55)) && (Va[i] > (1024*0.45))){
      samples = i;
      break;  
    }
  }

  
  /*//DEBUGGING
   
   for(int i = 0; i < samples ; ++i){
   Serial2.println(Va[i]);
   delay(1);
  }
  delay(10000);
  */

  for(int i = 0; i < samples ; ++i){
    //Apply Low Pass filter as in EMONlib and add to array
    offsetV = offsetV + ((Va[i]-offsetV)/1024);
    Va[i] = Va[i] - offsetV;
    offsetI = offsetI + ((Ia[i]-offsetI)/1024);
    Ia[i] = Ia[i] - offsetI;   
  }
  //-----------------------------------------------------------------------------
  // Apply Calculations to data set
  //-----------------------------------------------------------------------------
    for(int i = 0; i < samples ; ++i){
      //Used for delay/phase compensation
      if(i ==0) 
        { 
          lastfilteredVa=0;
        }
      else 
        { 
          lastfilteredVa = Va[i-1];
        }  
      
      sumV += Va[i] * Va[i];   //Square voltage values

      sumI += Ia[i] * Ia[i];   //Square Current values

      //Phase Calibration
      phaseShiftedVa = lastfilteredVa + 1 * (Va[i] - lastfilteredVa);
 
      //Instantaneous power calc
      instP = phaseShiftedVa * Ia[i];          //Instantaneous Power
      sumP +=instP;                               //Sum

    }
    
  
  //-------------------------------------------------------------------------------------------------------------------------
  // 3) Post loop calculations
  //-------------------------------------------------------------------------------------------------------------------------
  //Calculation of the root of the mean of the voltage and current squared (rms)
  //Calibration coefficients applied.

  double V_RATIO = VCALa *((SupplyVoltage/1000.0) / (1024));
  Vrms = V_RATIO * sqrt(sumV / samples);
  double I_RATIO = ICALa *((SupplyVoltage/1000.0) / (1024));
  Irms = I_RATIO * sqrt(sumI / samples);
  
  
  //Calculation power values
  realPower = V_RATIO * I_RATIO * sumP / samples;  //This is what residential power company uses!!
  apparentPower = Vrms * Irms;
  powerFactor = realPower / apparentPower; 

  //Mass averaging 10 cycles each
  if (vPin ==0) {
    avgVa += Vrms;
    avgIa += Irms;
    avgPa += realPower;
    avgAPa += apparentPower;
    avgPFa += powerFactor;
  }
  else {
    avgVb += Vrms;
    avgIb += Irms;
    avgPb += realPower;
    avgAPb += apparentPower;
    avgPFb += powerFactor;
  }
  
  avgCount++;
  if (avgCount == 20){  //Have we reached the 10 cycles each for averaging?
    avgVa /= 10;
    avgVb /= 10;
    avgIa /= 10;
    avgIb /= 10;
    avgPa /= 10;
    avgPb /= 10;
    avgPFa /= 10;
    avgPFb /= 10;
    avgAPa /= 10;
    avgAPb /= 10;
    Serial2.print(avgVa);
    Serial2.print(" ");
    Serial2.print(avgIa);
    Serial2.print(" ");
    Serial2.print(avgPa,3);
    Serial2.print(" ");
    Serial2.print(avgAPa,3);
    Serial2.print(" ");
    Serial2.print(avgPFa);
    Serial2.print(" ");
    Serial2.print(avgVb);
    Serial2.print(" ");
    Serial2.print(avgIb);
    Serial2.print(" ");
    Serial2.print(avgPb,3);
    Serial2.print(" ");
    Serial2.print(avgAPa,3);
    Serial2.print(" ");
    Serial2.println(avgPFb);
    
    avgCount = 0;  //Reset accumulators
    avgVa = 0;
    avgVb = 0;
    avgIa = 0;
    avgIb = 0;
    avgPa = 0;
    avgPb = 0;
    avgPFa = 0;
    avgPFb = 0;
    avgAPa = 0;
    avgAPb = 0;
  }
  /*Serial2.print(avgCount);
  Serial2.print(Vrms);
  Serial2.print("  ");
  Serial2.println(Irms);

  if(vPin==1){Serial2.println();}
*/
  
  //Reset accumulators
  sumV = 0;
  sumI = 0;
  sumP = 0;
  I_RATIO=0;
  V_RATIO=0;
  Vrms=0;
  Irms =0;
  lastfilteredVa = 0;
  realPower = 0;
  powerFactor = 0;
  apparentPower = 0;

}
