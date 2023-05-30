/*
The user memory space extends from (0x000-0x0FF) on the PIC10F200/204.
In Program/Verify mode, the program memory space extends from (0x000-0x1FF) for the PIC10F200/204.

ToDo:
- Add read functionality
- Get into a working state
- Redesign code (Make cleaner)
*/

#define MEMSIZE 384  // 1 word is 12 bits and the flash can store 256 words. [384]
#define WORDSIZE 12

unsigned int dlay = 1;

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

void writeLow(void) {
  digitalWrite(ICSPCLK, HIGH);
  delay(dlay);
  digitalWrite(ICSPDAT, LOW);
  delay(dlay);
  digitalWrite(ICSPCLK, LOW);
  delay(dlay);
}

void sendCommand(byte command) {
  for (int i = 0; i < 6; i++) {
    digitalWrite(ICSPCLK, HIGH);
    delay(dlay);
    digitalWrite(ICSPDAT, ((command >> (2 + (5 - i))) & 0b00000001) ? HIGH : LOW);
    delay(dlay);
    digitalWrite(ICSPCLK, LOW);
    delay(dlay);
  }
}

int getBit(byte * data, int queryBit) {
  int byteIndex = queryBit / 8;
  int bitIndex = queryBit % 8;

  return data[byteIndex] & (0x1 << (7 - bitIndex));
}

void putBit(byte * data, int queryBit, int bit) {
  int byteIndex = queryBit / 8;
  int bitIndex = queryBit % 8;

  byte mask = 0x1 << (7 - bitIndex);
  data[byteIndex] &= ~mask;
  data[byteIndex] |= bit & mask;
}

/// Gets a bit from a PIC instruction
/// @param data data to edit
/// @param queryInstruction desired instruction index
/// @param queryBit desired bit in the selected instruction (0 is the MSB)
/// @returns 1 for HIGH bit, 0 for LOW bit
int getPicBit(byte * data, int queryInstruction, int queryBit) {
  return getBit(data, queryInstruction * 12 + queryBit);
}

/// Puts a bit in a PIC instruction
/// @param data data to edit
/// @param queryInstruction desired instruction index
/// @param queryBit desired bit in the selected instruction (0 is the MSB)
/// @param bit 1 for HIGH bit, 0 for LOW bit
void putPicBit(byte * data, int queryInstruction, int queryBit, int bit) {
  putBit(data, queryInstruction * 12 + queryBit, bit);
}


// Write instructions
void writeInstruction(int index) {
  int instructionIndex = index;
  int start = instructionIndex * 12;

  sendCommand(LOAD_COMMAND);
  //Serial.print("L ");

  writeLow(); // Start bit
  for (int i = 0; i < 12; i++) {
    int b = i + start;
    int programIndex = b / 8;
    int bitOffset = b % 8;

    int bit = (program[programIndex] >> bitOffset) & 0x1;

    digitalWrite(ICSPCLK, HIGH);
    delay(dlay);
    digitalWrite(ICSPDAT, bit ? HIGH : LOW);
    delay(dlay);
    digitalWrite(ICSPCLK, LOW);
    delay(dlay);
    //Serial.print(bit ? '1' : '0');
  }
  // Ignored bits
  writeLow();
  writeLow();
  // Stop bit
  writeLow();

  //Serial.println(" ");  
}

void writeFlash() {
  for(int i = 0 ; i < 256 ; i++) {
    writeInstruction(i);
  }
}

void readFlash(void) {
  // Code to read data
}

void fillProgramArr(void) {
  static int iterator = 0;
  program[iterator] = Serial.read();
  // Serial.print(program[iterator], BIN);
  // Serial.print(" — ");
  // Serial.println(iterator);
  iterator++;

  if(iterator >= MEMSIZE) {
    iterator = 0;
    writeFlash();
    while(1);  // Hit reset button to write next program.
  }
}

byte readBuffer[MEMSIZE];

void readMemory(void) {
  for(int i = 0 ; i < MEMSIZE ; i++) {
    // Send read command
    sendCommand(READ_COMMAND);
    pinMode(ICSPDAT, INPUT);

    // Skip the start bit
    digitalWrite(ICSPCLK, HIGH);
    delay(dlay);
    digitalWrite(ICSPCLK, LOW);
    delay(dlay);

    // Read and store data
    for(int j = 0 ; j < 12 ; j++) {
      digitalWrite(ICSPCLK, HIGH);
      delay(dlay);
      if((// Stopped here)) {

      }
      delay(dlay);
      digitalWrite(ICSPCLK, LOW);
    }

    // Increment PC
    sendCommand(INCREMENT_COMMAND);
  }
}

void setup() {
  Serial.begin(9600);
  while(Serial.available()) {
    Serial.read();
  }

  pinMode(ICSPDAT, OUTPUT);
  pinMode(ICSPCLK, OUTPUT);
}

void loop() {
  digitalWrite(ICSPDAT, LOW);
  digitalWrite(ICSPCLK, LOW);

  if(Serial.available() > 0) {
    fillProgramArr();
  }
}