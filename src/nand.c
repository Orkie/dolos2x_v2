#include <stdio.h>
#include <string.h>
#include <dolos2x.h>
#include <gp2xregs.h>

#define CMD_READ 0x0
#define CMD_ERASE_SETUP 0x60
#define CMD_ERASE_CONFIRM 0xD0
#define CMD_WRITE_SETUP 0x80
#define CMD_WRITE_CONFIRM 0x10

#define BLOCK_SZ 512

static FILE* nandFp;

static int addrCounter = 0;
static uint32_t addrBuffer = 0x0;
static uint32_t addr = 0x0;
static uint8_t command = 0x0;
static bool commandRunning = false;
static uint8_t dataBuffer[BLOCK_SZ];
static int dataCounter = 0;
static uint16_t rMEMNANDCTRLW = 0x0;

static void padFileTo(FILE* fp, long sz) {
  fseek(fp, 0, SEEK_END);
  long fileSize = ftell(fp);
  long paddingNeeded = sz - fileSize;
  for(long i = 0 ; i < paddingNeeded ; i++) {
    fputc(0xff, fp);
  }
  fseek(fp, 0, SEEK_SET);
}

static void startCommand() {
  //  printf("Starting NAND command 0x%x with address 0x%x\n", command, addrBuffer);

  addr = addrBuffer;
  addrBuffer = 0x0;

  uint32_t startAddress = BLOCK_SZ * addr;
  
  switch(command) {
  case CMD_READ:
    commandRunning = true;
    dataCounter = 0;
    memset(dataBuffer, 0x0, BLOCK_SZ);
    int bytesRead;
    if(fseek(nandFp, addr, SEEK_SET) != 0 || (bytesRead = fread(dataBuffer, 1, BLOCK_SZ, nandFp)) != 512) {
      #ifdef DEBUG
      printf("Could not read full NAND block 0x%x, read %d bytes\n", addr, bytesRead);
      #endif
      return;
    }
    rMEMNANDCTRLW |= 0x8080;
    break;
  case CMD_ERASE_CONFIRM:
    memset(dataBuffer, 0xff, BLOCK_SZ);
    #ifdef DEBUG
    printf("Erasing block %d, addr 0x%x\n", addr, startAddress);
    #endif
    padFileTo(nandFp, startAddress + BLOCK_SZ);
    fseek(nandFp, startAddress, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SZ, nandFp);
    fflush(nandFp);
    rMEMNANDCTRLW |= 0x8080;
    break;
  case CMD_WRITE_SETUP:
    commandRunning = true;
    dataCounter = 0;
    memset(dataBuffer, 0x0, BLOCK_SZ);
    addrBuffer = addr;
    break;
  case CMD_WRITE_CONFIRM:
    #ifdef DEBUG
    printf("Writing to: 0x%x\n", addr);
    #endif
    padFileTo(nandFp, startAddress + BLOCK_SZ);
    fseek(nandFp, addr, SEEK_SET);
    fwrite(dataBuffer, 1, BLOCK_SZ, nandFp);
    fflush(nandFp);
    rMEMNANDCTRLW |= 0x8080;
    break;
  default:
    fprintf(stderr, "Unknown NAND command 0x%x\n", command);
  }
}

static void MEMNANDCTRLW_write(uint32_t addr, int bytes, void* value) {
  rMEMNANDCTRLW = CLEARBITS(rMEMNANDCTRLW, *((uint16_t*)value));
}

static void MEMNANDCTRLW_read(uint32_t addr, int bytes, void* ret) {
  *((uint16_t*)ret) = rMEMNANDCTRLW;
}

static void MEMNANDTIMEW_write(uint32_t addr, int bytes, void* value) {}

static void NFCMD_write(uint32_t addr, int bytes, void* value) {
  command = bytes == 1
    ? *((uint8_t*)value)
    : (*((uint16_t*)value) & 0xF);
  if(command == CMD_ERASE_CONFIRM) {
    startCommand();
  } else if(command == CMD_WRITE_CONFIRM) {
    startCommand();
  }
}

static void NFADDR_write(uint32_t addr, int bytes, void* value) {
  uint8_t written = bytes == 1
    ? *((uint8_t*)value)
    : (*((uint16_t*)value) & 0xF);

  if(addrCounter == 0) {
    addrBuffer |= (written & 0xFF);
    addrCounter++;
  } else if(addrCounter == 1) {
    addrBuffer |= ((written & 0xFF) << (command == CMD_ERASE_SETUP ? 8 : 9));
    addrCounter++;
  } else if(addrCounter == 2) {
    addrBuffer |= ((written & 0xFF) << (command == CMD_ERASE_SETUP ? 16 : 17));
    addrCounter++;
  } else if(addrCounter == 3) {
    addrBuffer |= ((written & 0xFF) << (command == CMD_ERASE_SETUP ? 24 : 25));
    addrCounter = 0;
    if(command != CMD_ERASE_SETUP) {
      startCommand();
    }
  }
}

static void NFDATA_write(uint32_t addr, int bytes, void* value) {
  if(commandRunning) {
    if(bytes == 2) {
      dataBuffer[dataCounter++] = U16_VAL(value) & 0xFF;
      dataBuffer[dataCounter++] = (U16_VAL(value) >> 8) & 0xFF;
    } else {
      dataBuffer[dataCounter++] = U8_VAL(value) & 0xFF;
    }      
  } else {
    fprintf(stderr, "Tried to write NFDATA when command was not running\n");
  }
}

static void NFDATA_read(uint32_t addr, int bytes, void* ret) {
  if(commandRunning) {
    if(bytes == 2) {
      *((uint16_t*)ret) = (dataBuffer[dataCounter+1] << 8) | dataBuffer[dataCounter];
      dataCounter += 2;
    } else {
      *((uint8_t*)ret) = dataBuffer[dataCounter++];
    }      
  } else {
    fprintf(stderr, "Tried to read NFDATA when command was not running\n");
  }
}

static int init(add_mem_callback add_read_callback, add_mem_callback add_write_callback) {
  nandFp = fopen("data/nand.bin", "rb+");
  if(nandFp == NULL) {
    fprintf(stderr, "Error opening NAND image\n");
    return 1;
  }
  padFileTo(nandFp, 512*1024);

  add_read_callback(REG(MEMNANDCTRLW), MEMNANDCTRLW_read);
  add_write_callback(REG(MEMNANDCTRLW), MEMNANDCTRLW_write);
  add_write_callback(REG(MEMNANDTIMEW), MEMNANDTIMEW_write);
  add_write_callback(NANDREG(NFCMD), NFCMD_write);
  add_write_callback(NANDREG(NFADDR), NFADDR_write);
  add_read_callback(NANDREG(NFDATA), NFDATA_read);
  add_write_callback(NANDREG(NFDATA), NFDATA_write);
  return 0;
}

dolos_peripheral peri_nand = {
  .name = "NAND",
  .init = &init
};

