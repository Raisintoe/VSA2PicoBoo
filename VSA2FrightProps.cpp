// VSA2FrightProps.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

/*
 * This program translates a VSA file into a FrightProps file for the PicoBoo controller
 * The input file will be called "TRexLoop1.vsa" and the frightprops file will be called "000.BIN"
 * 
 */

#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <stdint.h>

#pragma pack(1) //pack structs to one byte

using namespace std;

const string sFilePathVSA = "TRexLoop1.vsa";
const string sFilePathFP = "000.BIN";

ifstream iFileVSA;
fstream oFileFP;

const ios::off_type TO_BODY_VSA = 0x4C;
const ios::off_type TO_BODY_FP = 0x20;

const ios::off_type OFFSET_VSA_EVENT_COUNT = 0x34;

uint16_t temp16;
uint32_t temp32;

uint8_t *pBuf = NULL;
uint16_t frameLength;
char tempChar;
uint8_t channelShift;

const double VSA_MULT = (1 - 0.01); //VSA is dumb, and doesn't propperly represent 30 fps. This multiplier is used to correct the actual save frame location in the FrightProp file
streamoff offset;

struct HeaderFP {
    uint16_t lengthComp = 0x0000; //default to maximum length
    const uint16_t unknown02 = 0xFE17;
    const uint32_t unknown04 = 0xFFE6E6F6;
    const uint32_t unknown08 = 0xFFFFFFFF;
    const uint32_t unknown0C = 0xFFFFFFFF;
    const uint32_t unknown10 = 0xFFFF67FF;
    const uint32_t unknown14 = 0xFFFFFFFF;
    const uint32_t unknown18 = 0xFFFFFFFF;
    const uint32_t unknown1C = 0xFFFFFFFF;
} headerFP;

struct BlockVSA {
    uint16_t channel;
    uint32_t startFrame;
    uint32_t endFrame;
    uint32_t value;
    uint32_t unknownA; //0x0000 0000
    uint32_t unknownB; //0x0000 0000
    uint32_t unknownC; //0x0000 0000
    uint32_t unknownD; //0x0000 0100
    uint32_t unknownE; //0x0000 0100
    uint16_t unknownF; //0x0100
    uint8_t end; //0x80
} blockVSA;

struct BlockFP {

} blockFP;

int main()
{
    std::cout << "Hello World!\n";
    
    //Open the files
    iFileVSA.open(sFilePathVSA, ios::in | ios::binary);
    if (!iFileVSA)
    {
        cout << "Cannot open VSA file.\n";
        return 1;
    }
    oFileFP.open(sFilePathFP, ios::out | ios::in | ios::trunc | ios::binary);
    if (!oFileFP)
    {
        cout << "Cannot open FrightProps file.\n";
        return 1;
    }

    //Write the header
    pBuf = (uint8_t*) & headerFP; //point to the start of headerFP
    for (uint8_t i = 0; i < sizeof(headerFP); i++)
    {
        oFileFP.put(pBuf[i]);
    }

    //Interpret the VSA header
    pBuf = (uint8_t*) & temp16;
    iFileVSA.seekg(OFFSET_VSA_EVENT_COUNT, ios::beg);
    for (uint8_t b = 0; b < sizeof(temp16); b++)
    {
        pBuf[b] = iFileVSA.get();
    }

    //Fill the body with 0x00 to the number of frames over the whole durration
    // Not sure yet how to obtain the length from the header. Just set to the maximum length of 2^16-1 (0xFFFF)
    for (uint16_t i = 0; i < 0xFFFF; i++)
    {
        oFileFP.put(0x00);
    }

    //Transfer the body
    iFileVSA.seekg(TO_BODY_VSA, ios::beg);
    for (uint16_t i = 0; i < temp16; i++)
    {
        pBuf = (uint8_t*)&blockVSA;
        for (uint8_t b = 0; b < sizeof(blockVSA); b++)
        {
            pBuf[b] = iFileVSA.get();
        }

        frameLength = blockVSA.endFrame - blockVSA.startFrame;
        if (frameLength == 0)
        {
            cout << "\nFramelength error!";
            return 1;
        }

        if (blockVSA.end != 0x80)
        {
            cout << "\nFrame End check error at frame " << i << ", file location " << iFileVSA.tellg() << '!';
            //return 1; //last frame does not end with 0x80, so allow execution to continue.
        }

        cout << "\nVSA Start Frame: " << blockVSA.startFrame << " at event " << i;

        for (uint16_t j = 0; j < frameLength; j++)
        {
            streamoff offset = (streamoff)((TO_BODY_FP + blockVSA.startFrame + j) * VSA_MULT);
            oFileFP.seekp(offset, ios::beg); //go to the frame location
            oFileFP.seekg(offset, ios::beg);
            tempChar = oFileFP.get();
            channelShift = 0x01;
            tempChar |= (channelShift << blockVSA.channel);
            oFileFP.seekp(offset, ios::beg); //go to the frame location
            oFileFP.seekg(offset, ios::beg);
            oFileFP.put(tempChar);
            oFileFP.seekp(offset, ios::beg); //go to the frame location
            oFileFP.seekg(offset, ios::beg);
        }

    }

    oFileFP.close();
    iFileVSA.close();

}
