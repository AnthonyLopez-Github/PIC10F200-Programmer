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
- Fix issue with setting configuration bits (Incorrect bits getting set)
*/

#define MEMSIZE 384  // 1 word is 12 bits and the flash can store 256 words. [384 Bytes]
#define WORDSIZE 12
#define DEFAULT_CONFIG_WORD 0x7F18
#define MCLRE (1 << 7)
#define CP_NOT (1 << 6)
#define WDTE (1 << 5)

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
const uint16_t configurationWord = DEFAULT_CONFIG_WORD | CP_NOT; // Set to: 0111111101011000. Meaning: GP3/MCLR functions as GP3, Code protection is off, Watchdog timer is disabled.
uint16_t osccalPrimaryCalibrationBits; // Storage for calibration bits in program memory
uint16_t osccalBackupCalibrationBits; // Storage for calibration bits in configuration memory

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

// NOTE: word will need to be formatted: 0xxxxxxxxxxxx0000, where the x's are the word. Lsb first.
void loadWord(uint16_t word) {
  // Send write command
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((LOAD_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }

  // LSb first
  for(int i = 0 ; i < 16 ; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((word >> (15 - i)) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }
}

void beginProgramming(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((BEGIN_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(3); // Requires a max of 2ms wait time before using end programming
  }
}

void endProgramming(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((END_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }
}

uint16_t readWord(void) {
  uint16_t wordBuffer;

  // Send read command
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((READ_COMMAND >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }

  // Wait for the second rising edge
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);

  // Change ICSPDAT to input
  pinMode(ICSPDAT, INPUT);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);

  wordBuffer = 0;
  // Read 12 bits onto the serial monitor, ignore the remaining two MSbs
  for(int i = 0 ; i < 12 ; i++) {
    digitalWrite(ICSPCLK, LOW);
    delay(1);
    if(digitalRead(ICSPDAT)) {
      wordBuffer |= (uint16_t) 0x1 << (i); // Set bits. MSb first
    }
    delay(1);
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
  }

  // Clock up to 16 times, change mode of ICSPDAT
  digitalWrite(ICSPCLK, LOW);
  delay(1);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);
  digitalWrite(ICSPCLK, HIGH); // 16th rising edge, GP0 goes into input mode
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);

  pinMode(ICSPDAT, OUTPUT); // Set ICSPDAT to output mode

  return wordBuffer;
}

void getCalibrationBits(void) {
  // Move to OSCCAL calibration bits in program memory
  for(int i = 0 ; i < 256 ; i++) {
    incrementAddress();
  }

  osccalPrimaryCalibrationBits = readWord();
  Serial.print(osccalPrimaryCalibrationBits, BIN);
  Serial.println();

  // Move to OSCCAL backup calibration bits in configuration memory
  for(int i = 0 ; i < 5 ; i++){
    incrementAddress();
  }

  osccalBackupCalibrationBits = readWord();
  Serial.print(osccalBackupCalibrationBits, BIN);
}

void setup() {
  Serial.begin(9600);

  // Clear serial buffer
  while(Serial.available()) {
    Serial.read();
  }

  pinMode(ICSPDAT, OUTPUT);
  pinMode(ICSPCLK, OUTPUT);

  digitalWrite(ICSPDAT, LOW);
  digitalWrite(ICSPCLK, LOW);

  // Wait for user to send a character to start the following commands
  while(!Serial.available()) {
  }

  loadWord(configurationWord);
  beginProgramming();
  endProgramming();

  Serial.print(readWord(), BIN);
  //getCalibrationBits();
}

void loop() {
  //digitalWrite(ICSPDAT, LOW);
  //digitalWrite(ICSPCLK, LOW);
}