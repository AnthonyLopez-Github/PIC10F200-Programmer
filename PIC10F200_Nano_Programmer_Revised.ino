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
- Fix writeProgram function
*/

#define MEMSIZE 256 // 1 word is 12 bits and the flash can store 256 words. [384 Bytes]
#define WORDSIZE 12
#define DEFAULT_CONFIG_WORD 0x0FE3
#define MCLRE (1 << 4)
#define CP_NOT (1 << 3)
#define WDTE (1 << 2)

const unsigned int ICSPCLK = 2;
const unsigned int ICSPDAT = 4;

// Most significant bit first
const byte LOAD_COMMAND = 0b0010;        // Load Data for Program Memory
const byte READ_COMMAND = 0b0100;        // Read Data from Program Memory
const byte INCREMENT_COMMAND = 0b0110;   // Increment Address
const byte BEGIN_COMMAND = 0b1000;       // Begin Programming
const byte END_COMMAND = 0b1110;         // End Programming
const byte BULK_ERASE_COMMAND = 0b1001;  // Bulk Erase Program Memory

byte program[MEMSIZE];  // Array containing 384 bytes
const uint16_t configurationWord = DEFAULT_CONFIG_WORD | CP_NOT | WDTE; // Set to: 111111101011. Meaning: GP3/MCLR functions as GP3, Code protection is off, Watchdog timer is disabled.
uint16_t osccalPrimaryCalibrationBits; // Storage for calibration bits in program memory
uint16_t osccalBackupCalibrationBits; // Storage for calibration bits in configuration memory

void incrementAddress(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((INCREMENT_COMMAND >> i) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }
}

// Load a word
void loadWord(uint16_t word) {
  // Send write command
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((LOAD_COMMAND >> i) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }

  // Skip first bit
  digitalWrite(ICSPDAT, LOW);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);

  // Load word
  for(int i = 0 ; i < 12 ; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((word >> i) & (uint16_t) 0x1) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }

  // Skip the remaining bits
  digitalWrite(ICSPDAT, LOW);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);
  digitalWrite(ICSPCLK, HIGH);
  delay(1);
  digitalWrite(ICSPCLK, LOW);
  delay(1);
}

void beginProgramming(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((BEGIN_COMMAND >> i) & 0b00000001) ? HIGH : LOW);
    delay(1);
    digitalWrite(ICSPCLK, LOW);
    delay(1);
  }
  delay(2); // Wait at most 2ms before using end programming
}

void endProgramming(void) {
  for(int i = 0 ; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(1);
    digitalWrite(ICSPDAT, ((END_COMMAND >> i) & 0b00000001) ? HIGH : LOW);
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
    digitalWrite(ICSPDAT, ((READ_COMMAND >> i) & 0b00000001) ? HIGH : LOW);
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
////////////////////////////////////////////////////////////////////////////////////////////////
int getBit(byte * data, int queryBit) {
  int byteIndex = queryBit / 8;
  int bitIndex = queryBit % 8;

  return data[byteIndex] & (0x1 << (7 - bitIndex));
}
////////////////////////////////////////////////////////////////////////////////////////////////
void storeCalibrationBits(void) {
  // Move to OSCCAL calibration bits in program memory
  for(int i = 0 ; i < 256 ; i++) {
    incrementAddress();
  }

  osccalPrimaryCalibrationBits = readWord(); // Store

  // Move to OSCCAL backup calibration bits in configuration memory
  for(int i = 0 ; i < 5 ; i++){
    incrementAddress();
  }

  osccalBackupCalibrationBits = readWord(); // Store
}

void writeCalibrationBits(void) {
  // Move to OSCCAL calibration bits in program memory
  for(int i = 0 ; i < 256 ; i++) {
    incrementAddress();
  }

  // Store primary calibration bits at current address
  loadWord(osccalPrimaryCalibrationBits);
  beginProgramming();
  endProgramming();

  // Move to OSCCAL backup calibration bits in configuration memory
  for(int i = 0 ; i < 5 ; i++){
    incrementAddress();
  }

  // Store backup calibration bits at current address
  loadWord(osccalBackupCalibrationBits);
  beginProgramming();
  endProgramming();
}

void writeConfigurationWord(void) {
  Serial.print("Writing configuration word...");
  loadWord(configurationWord);
  beginProgramming();
  endProgramming();

  // Report if written value does not match what was provided
  uint16_t valueWritten = readWord();
  if(valueWritten != configurationWord) {
    Serial.println("Failed to write configuration word: ");
    Serial.print("Provided: ");
    Serial.println(configurationWord, BIN);
    Serial.print("Read: ");
    Serial.println(valueWritten, BIN);
  } else {
    Serial.println("Done");
    Serial.print("Current configuration: ");
    Serial.println(valueWritten, BIN);
  }
}

int writeProgram(byte programArr[MEMSIZE]) {
  incrementAddress(); // Roll over to address 000 from configuration word

  uint16_t wordBuffer;
  int bitIndex = 0;
  Serial.print("Writing program to memory...");
  for(int i = 0 ; i < MEMSIZE - 1 ; i++) {
    wordBuffer = 0; // Reset buffer
    for(int j = 0 ; j < 12 ; j++) {
      wordBuffer |= getBit(programArr, bitIndex) << (11 - j);
      bitIndex ++;
    }
    loadWord(wordBuffer);
    beginProgramming();
    endProgramming();

    uint16_t valueWritten = readWord();
    if(valueWritten != wordBuffer) {
      Serial.println("Failed to write word: ");
      // Add address with error
      Serial.print("Provided: ");
      Serial.println(wordBuffer, BIN);
      Serial.print("Read: ");
      Serial.println(valueWritten, BIN);
      return 1;
    }
  }
  Serial.println("Done");
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

  writeConfigurationWord();

  /*
  // Write a bunch of NOPs
  writeProgram(program); 
  
  // Increment twice to rollover to 000
  incrementAddress();
  incrementAddress();

  // Dump memory
  for(int i = 0 ; i < 256 ; i++) {
    readWord();
    incrementAddress();
  }
  */
}

void loop() {
}