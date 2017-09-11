//
//  main.cpp
//  CPE_381_F16_project
//
//  Created by Todd Dick on 10/4/16.
//  Copyright © 2016 Todd Dick. All rights reserved.
//
//  Created for CPE381 Fall 2016 UAH
//  Dr. Emil Jovanov, Instructor
//
//  This program takes an input of a .wav file recorded with 16 bit samples in mono, downsamples the file, and adds noise in the form of a sine wave of 1/4 of the max amplitude of the original .wav file.
//  Compile using the command:
//      g++ main.cpp -o "whatever"
//  where "whatever" is the name of the executable you desire without the quotes...

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdio>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>

#define PI 3.14159265
#define MAX 32767
#define MIN -32768
using namespace std;

int main(int argc, const char * argv[]) {

    //create a struct for header
    struct WAVE_HEADER {
        uint8_t RIFF[4];   //should be "RIFF"
        uint32_t chunkSize;     //RIFF chunk size
        uint8_t WAVE[4];   //should be "WAVE"
        uint8_t fmt[4];    //should be "fmt "
        uint32_t SubChunkSize;  //fmt chunk size
        uint16_t format;        //should be 1
        uint16_t numChannels;   //should be 1
        uint32_t frequency;     //should be 22050
        uint32_t bytesPerSec;
        uint16_t blockalign;
        uint16_t bitsPerSample;
        uint8_t data[4];    //should be "data"
        uint32_t dataSize;
    };
    
    WAVE_HEADER header;
    FILE * infile;
    uint32_t headerSize;
    
    const char* filePath;
    string getName;
    if (argc <=1) {
        cout << "Input wave file name: ";
        cin >> getName;
        cin.get();
        filePath = getName.c_str();
    }
    else {
        filePath = argv[1];
        cout << "Input wave file name: " << filePath << endl;
    }
    infile = fopen(filePath, "rb");
    if (infile==NULL){
        cout << "Put the wave file in this folder" << endl;
        return 1;
    }
    headerSize = sizeof(header);
    
    // start timing
    clock_t t;
    t = clock();
    // read the header in
    size_t bytesRead = fread (&header,1,headerSize,infile);
    
    // test to see we can handle the input file
    if (header.bitsPerSample != 16 || header.numChannels != 1){
        printf("This program won't properly process your file.\n Please use a file with 16 bit samples recorded in mono.\n");
    }
    
    //used later...
    unsigned long long numSamplesOrig = (header.dataSize)/(header.numChannels*header.bitsPerSample/8);
    
    // open file for writing
    FILE * outFile;
    outFile = fopen("Dick_T_mod.wav", "wb");
    if (!outFile){
        printf("Error with output file");
    }
    // modify header and write it
    header.frequency = header.frequency/2;
    header.dataSize = header.dataSize/2;
    header.bytesPerSec = header.bytesPerSec/2;
    fwrite(&header,sizeof(header),1,outFile);
    
    
    // set some useful variables
    uint32_t sampcount=0;
    int16_t sample[2], writeValue[1]; // arrays for original samples and the downsample
    float sinExpression, parenthesized;
    float amplitude = MAX/4;
    
    // read data, modify it, and write it.
    while (sampcount < header.dataSize/2) {
        size_t readStuff = fread(&sample,2,sizeof(int16_t),infile);
        parenthesized = 2*PI*2000*sampcount/header.frequency;
        sinExpression = amplitude*sin(parenthesized);
        // the following deals with any number larger than 16 bits... overflow
        if (sinExpression > MAX) {
            sinExpression = (int16_t)MAX;
        }
        else if (sinExpression < MIN){
            sinExpression = (int16_t)MIN;
        }
        else{
            sinExpression = (int16_t)sinExpression;
        }
        // This averages the samples without conversion to a 17-bit value... no overflow. Found on internet.
        int16_t average = ((sample[0]/2) + (sample[1]/2) + (sample[0] & sample[1] & 0x1));
        int16_t sum = average + sinExpression;
        // The following deals with any overflow in the sum
        if (sum > MAX){
            sum = MAX;
        }
        if (sum < MIN){
            sum = MIN;
        }
        writeValue[0] = sum;
        fwrite(&writeValue,sizeof(int16_t),1,outFile);
        if (!outFile){
            printf("Error with output file");
            exit(1);
        }
        sampcount++;
    }
    // A .wav file needs even numbers of bytes. This pads a "0" byte onto the end of the file if needed
    if (sampcount %2 != 0){
        int16_t x = 0;
        int16_t padding[1] = {x};
        fwrite(&padding,sizeof(int16_t),1,outFile);
    }
    
    // close the files that were used
    fclose(infile);
    fclose(outFile);
    
    // stop clock
    t = clock() - t;
    // write summary.txt file required as part of the assignment
    ofstream summary;
    uint32_t recordTime = numSamplesOrig/(header.frequency*2);
    summary.open ("summary.txt");
    summary << "The sampling frequency of the original file : " << header.frequency*2 << "Hz." << endl;
    summary << "The record length of the original file : " << recordTime << " seconds." << endl;
    summary << "The execution time of this program : " << (float)t/CLOCKS_PER_SEC << " seconds." << endl;
    summary.close();
    return 0;
}
