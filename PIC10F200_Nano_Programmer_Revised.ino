/*
The user memory space extends from (0x000-0x0FF) on the PIC10F200/204.
In Program/Verify mode, the program memory space extends from (0x000-0x1FF) for the PIC10F200/204.

During Read commands, in which the data is output from
the PIC Flash MCUs, the ICSPDAT pin transitions from
the high-impedance input state to the low-impedance
output state at the rising edge of the second data clock
(first clock edge after the Start cycle). The ICSPDAT pin
returns to the high-impedance state at the rising edge of
the 16th data clock (first edge of the Stop cycle).

The Configuration Word register is stored at the logical 
address location of 0xFFF within the hex file generated 
for the PIC10F200. It is the responsibility of the 
programming software to retrieve the Configuration Word 
register from the logical address within the hex file and 
translate the address to the proper physical location when 
programming.

ToDo:
- Add read functionality
- Get into a working state
- Redesign code (Make cleaner)
*/

#define MEMSIZE 384  // 1 word is 12 bits and the flash can store 256 words. [384 Bytes]
#define WORDSIZE 12

const unsigned int ICSPCLK = 2;
const unsigned int ICSPDAT = 4;

// Least significant bit first
const byte LOAD_COMMAND = 0b01000000;        // Load Data for Program Memory
const byte READ_COMMAND = 0b00100000;        // Read Data from Program Memory
const byte INCREMENT_COMMAND = 0b01100000;   // Increment Address
const byte BEGIN_COMMAND = 0b00010000;       // Begin Programming
const byte END_COMMAND = 0b01110000;         // End Programming
const byte BULK_ERASE_COMMAND = 0b10010000;  // Bulk Erase Program Memory

byte program[MEMSIZE];  // Array containing 384 bytes
byte configurationWord[2] = {0b11111110, 0b10110000}; // Set to: 111111101011. Meaning: GP3/MCLR functions as GP3, Code protection is off, Watchdog timer is disabled.
byte osccalCalibrationBits; // Store calibration 

void fillProgramArray(void) {
  static int iterator = 0;
  program[iterator] = Serial.read();
  iterator++;
}

// Location: 00FFh contains the internal clock oscillator calibration value.
void getCalibrationBits(void) {
  // Move to address 00FFh to read calibration bits
  //while() {
  //
  //}
}

void incrementAddress(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((INCREMENT_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }
}

void readWord(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(250);
    digitalWrite(ICSPDAT, ((READ_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(250);
    digitalWrite(ICSPCLK, LOW);
    delay(500);
  }

  // Wait for the second rising edge
  digitalWrite(ICSPCLK, HIGH);
  delay(500);
  digitalWrite(ICSPCLK, LOW);
  delay(500);

  // Change ICSPDAT to input
  pinMode(ICSPDAT, INPUT);
  digitalWrite(ICSPCLK, HIGH);
  delay(500);

  // Read 12 bits onto the serial monitor, ignore the remaining two MSbs
  for(int i = 0 ; i < 12 ; i++) {
    digitalWrite(ICSPCLK, LOW);
    delay(250);
    Serial.println(digitalRead(ICSPDAT));
    delay(250);
    digitalWrite(ICSPCLK, HIGH);
    delay(500);
  }

  //Bring clock low
  digitalWrite(ICSPCLK, LOW);
}

void setup() {
  Serial.begin(9600);
  while(Serial.available()) {
    Serial.read();
  }

  pinMode(ICSPDAT, OUTPUT);
  pinMode(ICSPCLK, OUTPUT);

  digitalWrite(ICSPDAT, LOW);
  digitalWrite(ICSPCLK, LOW);
  while(!Serial.available()) {
  }

  // Increment to OSCCAL calibration value in program memory
  for(int i = 0 ; i < 256 ; i++) {
    incrementAddress();
  }

  // Read data from current address
  readWord();
}

void loop() {
  //digitalWrite(ICSPDAT, LOW);
  //digitalWrite(ICSPCLK, LOW);
}
