# HT6P20B_Arduino_Decoder

  Arduino decoder HT6P20B (using 2M2 resistor in osc)
  - This code (Arduino) uses timer1 and external interrupt pin.
  - It measures the "pilot period" time (see HT6P20B datasheet for details) and compares if it is within the defined range,
  - If the "pilot period" is within the defined range, it switches to "code period" part (see HT6P20B datasheet for details)
    and takes time measurements at "high level" ("C++" language).
  - If the "code period" is within the previously measured range, add bit 0 or 1 in the _data variable,
    if the "code period" is not within the previously measured range, reset the variables and restart,
  - After receiving all 28 bits, the final 4 bits ("anti-code period", see HT6P20B datasheet for details)
    are tested to verify that the code was received correctly.
  - If received correctly it sets the antcode variable to 1, and the _data2 variable receives the content of the _data variable.
  
 (Note: The RF receiver (for example 433MHz) needs to be connected to the external interrupt pin)
 
 (Note: For HT6P20A and HT6P20D the code may need to be modified)
