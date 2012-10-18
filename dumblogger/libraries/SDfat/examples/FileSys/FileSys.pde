/*
#include <FatStructs.h>
#include <Sd2Card.h>
#include <Sd2PinMap.h>
#include <SdFatmainpage.h>
#include <SdFatUtil.h>
#include <SdInfo.h>
*/

#include <SdFat.h>
#include <HardwareSPI.h>
#include <stdint.h>

HardwareSPI spi(1);
Sd2Card card(spi, USE_NSS_PIN, true);
SdVolume volume;
SdFile root;
SdFile file;

void setup() {
  SerialUSB.begin();
  SerialUSB.println("type any char to start");
  while (!SerialUSB.available());
  SerialUSB.println();

  // initialize the SD card
  if (!card.init())
    SerialUSB.println("card.init failed");
  else
    SerialUSB.println("card.init passed");

  delay(100);

  // initialize a FAT volume
  if (!volume.init(&card,1))
    SerialUSB.println("volume.init failed");
  else
    SerialUSB.println("volume.init passed");

  // open the root directory
  if (!root.openRoot(&volume))
    SerialUSB.println("openRoot failed");
  else
    SerialUSB.println("openRoot passed");

  // open a file
  if (file.open(&root, "Read.txt", O_READ))
  {
    SerialUSB.println("Opened Read.txt");
    int16_t c;
    while ((c = file.read()) > 0)
      SerialUSB.print((char)c);
    SerialUSB.println("");
  }
  else
  {
    SerialUSB.println("file.open failed");
  }
  SerialUSB.println();

  SerialUSB.println("Done");
  spi.end();
}

void loop() {
}