//
//  main.cpp
//  CPE_381_F16_project_PhaseII
//
//  Created by Todd Dick in 11/16.
//  Copyright © 2016 Todd Dick. All rights reserved.
//
//  Created for CPE381 Fall 2016 UAH
//  Dr. Emil Jovanov, Instructor
//
//  This program takes an input of a .wav file, processes it for a filter designed in Matlab, and included with preprocessor directives, then writes a new filtered .wav file to disk.
//  Compile using the command:
//      g++ main.cpp -o "whatever"
//  where "whatever" is the name of the executable you desire without the quotes...

#include <iostream>
#include <fstream>
#include <cmath>
#include <cstdint>
#include <vector>
#include <string>
#include <cstring>
#include <ctime>
#include "bsfcoefs.h"
#include "lpfcoefs.h"    //contains "B" array and "BL" const for length of "B" array


#define PI 3.14159265
#define MAX 32767
#define MIN -32768
using namespace std;




// function to shift the samples array to the left
void ShiftSamples(int arrayLength, double samples[]){
    for(int i = arrayLength-1; i > 0; i--){
        samples[i] = samples[i-1];
    }
}

// function to do realtime convolution sum on array A
int16_t OutSampleA(int arrayLength, double samples[]){
    double tempValue = 0.0;
    int16_t outValue;
    for (int i =0; i < arrayLength; i++){
        tempValue += A[i]*samples[i];
    }
    outValue = (int16_t)tempValue;
    
    return outValue;
}

// function to do realtime convolution sum on array B
int16_t OutSampleB(int arrayLength, double samples[]){
    double tempValue = 0.0;
    int16_t outValue;
    for (int i =0; i < arrayLength; i++){
        tempValue += B[i]*samples[i];
    }
    outValue = (int16_t)tempValue;
    
    return outValue;
}






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
        uint32_t frequency;
        uint32_t bytesPerSec;
        uint16_t blockalign;
        uint16_t bitsPerSample;
        uint8_t data[4];    //should be "data"
        uint32_t dataSize;
    };
    
    WAVE_HEADER header;
    FILE * infile;
    uint32_t headerSize;
    
    //logic to get the filepath to the signal to be processed
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
    
    // open a file for writing. Choose a name based on filter being used
    FILE * outFile;
    if (header.frequency == 22050){
        outFile = fopen("Dick_T_bs.wav", "wb");
    }
    else if (header.frequency == 11025){
        outFile = fopen("Dick_T_lp.wav", "wb");
    }
    else{
        cout << "This program will not filter your file. Please use a file with frequency 22050Hz or 11025Hz." << endl;
        return 1;
    }
    if (!outFile){
        printf("Error opening an output file");
        return 1;
    }
    // write header
    
    fwrite(&header,sizeof(header),1,outFile);
    
    
    // set some useful variables
    uint32_t sampcount=0;   // drives the while loop for processing
    int16_t sample[1], writeValue[1]; // arrays for read sample and the output
    int16_t yValue;     // this is the output value
    
    // get the array length for the samples array
    int arrayLength;
    if (header.frequency == 22050){
        arrayLength = AL;
    }
    else {
        arrayLength = BL;
    }
    
    // create and initialize to 0's the array to hold samples from the input file
    double samples[arrayLength];
    for (int i = 0; i < arrayLength; i++){
        samples[i] = 0;
    }
    
    // read data, modify it, and write it.
    while (sampcount < header.dataSize/2) {
        size_t readStuff = fread(&sample,1,sizeof(int16_t),infile); //read 16 bits in
        //after first read sample, start building the array of read samples by shifting them
        if (sampcount > 0){
            ShiftSamples(arrayLength, samples);
        }
        //put the newest sample in the samples array at index [0]
        samples[0] = (double)sample[0];
        // process the samples and coefficient arrays. Chooses from almost identical convolution sum algorithms
        if (arrayLength == AL){
            yValue = OutSampleA(arrayLength, samples);
        }
        else{
            yValue = OutSampleB(arrayLength, samples);
        }
        //handle overflow if any
        if (yValue > MAX){
            yValue = MAX;
        }
        else if (yValue < MIN){
            yValue = MIN;
        }
        // put the writevalue in its container for writing
        writeValue[0] = yValue;
        //write the value
        fwrite(&writeValue,sizeof(int16_t),1,outFile);
        //error check
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
    // this code works like the part 1 code with metrics from the program execution
    ofstream summary;
    uint32_t recordTime = numSamplesOrig/(header.frequency);
    if (arrayLength == AL){
        summary.open ("summarybs.txt");
    }
    else {
        summary.open ("summarylp.txt");
    }
    summary << "The sampling frequency of the file : " << header.frequency << "Hz." << endl;
    summary << "The record length of the file : " << recordTime << " seconds." << endl;
    summary << "The execution time of this program : " << (float)t/CLOCKS_PER_SEC << " seconds." << endl;
    summary.close();
    return 0;
}
