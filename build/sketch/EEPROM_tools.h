/* 
 * 
 * ESP8296 EEPROM tools
 * 
 * Vladislav Kalugin (c) 2020
 * 
 */


// Returns CRC checksum of the EEPROM
unsigned long eeprom_crc(void) {

  const unsigned long crc_table[16] = {
    0x00000000, 0x1db71064, 0x3b6e20c8, 0x26d930ac,
    0x76dc4190, 0x6b6b51f4, 0x4db26158, 0x5005713c,
    0xedb88320, 0xf00f9344, 0xd6d6a3e8, 0xcb61b38c,
    0x9b64c2b0, 0x86d3d2d4, 0xa00ae278, 0xbdbdf21c
  };

  unsigned long crc = ~0L;

  for (int index = 4; index < EEPROM.length()  ; ++index) {
    crc = crc_table[(crc ^ EEPROM[index]) & 0x0f] ^ (crc >> 4);
    crc = crc_table[(crc ^ (EEPROM[index] >> 4)) & 0x0f] ^ (crc >> 4);
    crc = ~crc;
  }
  return crc;
}

// Reads long from EEPROM
long EEPROMReadlong(long address) {
  long four = EEPROM.read(address);
  long three = EEPROM.read(address + 1);
  long two = EEPROM.read(address + 2);
  long one = EEPROM.read(address + 3);
 
  return ((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
}

// Writes long to the EEPROM
void EEPROMWritelong(int address, long value) {
  byte four = (value & 0xFF);
  byte three = ((value >> 8) & 0xFF);
  byte two = ((value >> 16) & 0xFF);
  byte one = ((value >> 24) & 0xFF);
 
  EEPROM.write(address, four);
  EEPROM.write(address + 1, three);
  EEPROM.write(address + 2, two);
  EEPROM.write(address + 3, one);
}

// Write string to the EEPROM
void EEPROMWriteString(int address, String str) {
  // Get the length of our string
  byte len = str.length();
  // Write string length
  EEPROM.write(address, len);
  for (int i = 0; i < len; i++) {
    EEPROM.write(address + 1 + i, str[i]);
  }
  
  // Commit EEPROM changes
  EEPROM.commit();
}

// Read string from the EEPROM
String EEPROMReadString(int address) {
  // Read string address
  int len = EEPROM.read(address);
  // Create an array to store data 
  char data[len + 1];

  for (int i = 0; i < len; i++) {
    data[i] = EEPROM.read(address + 1 + i);
  }

  // Add the terminator
  data[len] = '\0';

  return String(data);
}

// Update EEPROM checksum
void eeprom_crc_update() {
  // Update checksum
  EEPROMWritelong(0, eeprom_crc());
  // Commit changes
  EEPROM.commit();
}

// Clears the EEPROM
void EEPROMClear() {
  for (int i = 0; i < EEPROM.length(); i++) {
    EEPROM.write(i, 0); 
  }
  EEPROM.commit();
}
