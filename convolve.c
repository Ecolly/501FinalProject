/******************************************************************************
*
*     Program:       readReverseWrite
*     
*     Description:   This program reads a .wav file and takes all the data 
*                    and reverses it.
*
*     Author:        Ali Al-Khaz'Aly
*
*     Date:          November 18 2023
*
******************************************************************************/


/*  input_HEADER FILES  ************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <stdint.h>
// struct to hold all data up until the end of subchunk1
// you still have 8 bytes to read before actual data, namely being subchunk2ID & subChunk2Size
// note it may actually be more than 8 bytes as subchunk1size may not be 16!
typedef struct {
    char chunk_id[4];
    int chunk_size;
    char format[4];
    char subchunk1_id[4];
    int subchunk1_size;
    short audio_format;
    short num_channels;
    int sample_rate;
    int byte_rate;
    short block_align;
    short bits_per_sample;
} WavHeader;

// Function definitions (:
void readTone(char *inputFileName,char *IRFileName, char *outputFileName);
float bytesToFloat(char firstByte, char secondByte) ;
void convolve(float x[], int N, float h[], int M, float y[], int P);
void printFloatArray(const float arr[], int size);
float shortToFloat(short s);
float findMaxFloat(float arr[], int n);
int main (int argc, char *argv[])
{
    char *inputFileName = NULL;
    char *IRFileName = NULL;
    char *outputFileName = NULL;

    /*  Process the command line arguments  */
    if (argc == 4) {
        /*  Set a pointer to the output filename  */
        inputFileName = argv[1];
        IRFileName = argv[2];
        outputFileName = argv[3];


    }
    else {
        /*  The user did not supply the correct number of command-line
            arguments.  Print out a usage message and abort the program.  */
        fprintf(stderr, "Usage:  %s inputFileName IRfilename outputFileName\n", argv[0]);
        exit(-1);
    }

    readTone(inputFileName, IRFileName, outputFileName);
}

void readTone(char *inputFileName, char *IRFileName, char *outputFileName){
    //opens the file for readig in binary
    FILE *inputFileStream = fopen(inputFileName, "rb");
    FILE *IRFileStream = fopen(IRFileName, "rb");
    FILE *outputFileStream = fopen(outputFileName, "wb");

    WavHeader input_header;
    WavHeader IR_header;
    // read the input_header subchunk 1, write the input_header into a new file
    fread(&input_header, sizeof(input_header), 1, inputFileStream);
    fread(&IR_header, sizeof(IR_header), 1, IRFileStream);
    fwrite(&input_header, sizeof(input_header), 1, outputFileStream);

    //input file information
    printf("///////////////Input information//////////////////////// \n");
    printf("audio format: %s\n", input_header.format); 
    printf("number of channels: %d\n", input_header.num_channels);
    printf("size of subchunk1: %d\n", input_header.subchunk1_size);

    printf("///////////////IR information////////////////////////\n");
    printf("audio format: %s\n", IR_header.format); 
    printf("number of channels: %d\n", IR_header.num_channels);
    printf("size of subchunk1: %d\n", IR_header.subchunk1_size);

    if (input_header.subchunk1_size != 16){
        // the remaining bytes in subchunk1 will be null bytes if there is more than 16
        // so read the junk!
        printf("input subchunk1 is too big! Skipping some bytes!\n");
        int remainder = input_header.subchunk1_size -16;
        char randomVar[remainder];
        fread(randomVar, remainder, 1, inputFileStream);
        fwrite(randomVar, remainder, 1, outputFileStream);
    }
    if (IR_header.subchunk1_size != 16){
        // the remaining bytes in subchunk1 will be null bytes if there is more than 16
        // so read the junk!
        printf("IR subchunk1 is too big! Skipping some bytes!\n");
        int remainder = IR_header.subchunk1_size -16;
        char randomVar[remainder];
        fread(randomVar, remainder, 1, IRFileStream);
    }
    // an integer is 4 bytes
    char subchunk2_id_input[4]; //"data"
    int subchunk2_size_input;   //size of subchunk in bytes
    char subchunk2_id_IR[4];    //"data"
    int subchunk2_size_IR; 
    
    //copying the "Data" chunk identifier and its size to the input wav to the output wav
    fread(&subchunk2_id_input, sizeof(subchunk2_id_input), 1, inputFileStream);
    fread(&subchunk2_size_input, sizeof(subchunk2_size_input), 1, inputFileStream);
    fread(&subchunk2_id_IR, sizeof(subchunk2_id_IR), 1, IRFileStream);
    fread(&subchunk2_size_IR, sizeof(subchunk2_size_IR), 1, IRFileStream);
    //write to output file
    fwrite(&subchunk2_id_input, sizeof(subchunk2_id_input), 1, outputFileStream);
    fwrite(&subchunk2_size_input, sizeof(subchunk2_size_input), 1, outputFileStream);

    //number of audio sample in input and IR, number of bytes
    int num_samples_input = subchunk2_size_input / (input_header.bits_per_sample / 8);
    //total bytes/(byte per sample) = number of samples of adudio
    int num_samples_IR = subchunk2_size_IR / (IR_header.bits_per_sample / 8);
    int num_output = num_samples_input + num_samples_IR -1;

    //represent the size in bytes of data
    size_t data_size_input = subchunk2_size_input; //var = bytes
    size_t data_size_IR = subchunk2_size_IR;

    //calculate datasize for output file:
    //output Datasize =(IRNumSamples+DryNumSamples−1)×Channels×BytesPerSample
    size_t data_size_output = num_output*input_header.num_channels*(input_header.bits_per_sample/8);

    
    // allocate a bunch of space to hold all of the audio data
    //allocate memory blocks to store arrays of short integers
    short *audio_data = (short *)malloc(data_size_input);
    short *IR_data = (short *)malloc(data_size_IR);

    // Print out some more info
    printf("bits per sample: %d\n", input_header.bits_per_sample);
    printf("subchunk2 size: %d\n", subchunk2_size_input);
    printf("number of samples: %d\n", num_samples_input);

    // // read it into an array of shorts
    fread(audio_data, sizeof(short), data_size_input, inputFileStream);
    fread(IR_data, sizeof(short), data_size_IR, IRFileStream);
    //printf("\n input: %d", audio_data[80000]);

    float *audioFloats = (float *)malloc(data_size_input/sizeof(short)*sizeof(float));
    float *IRFloats = (float *)malloc(data_size_IR/sizeof(short)*sizeof(float));
    float *outputFloats = (float *)malloc(data_size_output/sizeof(short)*sizeof(float));
        
    for (int i = 0; i < num_samples_input; i++) {
        //printf("\n input: %d", audio_data[80000]);
        audioFloats[i] = shortToFloat(audio_data[i]);
        //printf("\n input: %f", audioFloats[i]);
    }
    for (int i = 0; i < num_samples_IR; i++) {
        IRFloats[i] = shortToFloat(IR_data[i]);
        //printf("\n IR: %f", IRFloats[i]);
    }

    short data;
    convolve(audioFloats, num_samples_input, IRFloats, num_samples_IR, outputFloats , num_output);
    for (int i =0; i<num_output;i++){
        data = (short)(outputFloats[i]*32768);
        //printf("\n data: %d",data);
        fwrite(&data,sizeof(data),1,outputFileStream);
    }

    free(audio_data);
    free(audioFloats);
    // free(IRFloats);
    // Close the files
    fclose(inputFileStream);
    fclose(outputFileStream);
}


// float findMaxFloat(float arr[], int n) {
//     if (n <= 0) {
//         return -1; // Return an error value or handle this case as appropriate
//     }

//     float max = arr[0]; // Start with the first element as the maximum

//     for (int i = 1; i < n; i++) {
//         if (arr[i] > max) {
//             max = arr[i]; // Update max if the current element is greater
//         }
//     }
//     printf("max value is: %d", max);
//     return max;
// }


float shortToFloat(short s) {
    // Convert to range from -1 to (just below) 1
    return s / 32768.0/2;//scale down the numbers
}

void convolve(float x[], int N, float h[], int M, float y[], int P)
{
    int n,m;

    /* Clear Output Buffer y[] */
    for (n=0; n < P; n++)
    {
        y[n] = 0.0;
    }

    /* Outer Loop: process each input value x[n] in turn */
    for (n=0; n<N; n++){
        /* Inner loop: process x[n] with each sample of h[n] */
        for (m=0; m<M; m++){
            y[n+m] += x[n] * h[m];
        }
    }
}

void printFloatArray(const float arr[], int size) {
    for (int i = 0; i < size; i++) {
        printf("%.6f ", arr[i]);
    }
    printf("\n");
}