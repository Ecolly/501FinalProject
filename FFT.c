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
#include <limits.h>

#define SWAP(a,b)  tempr=(a);(a)=(b);(b)=tempr

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
void printFloatArray(const float arr[], int size);
float shortToFloat(short s);
float findMaxFloat(float arr[], int n);
void four1(double data[], int nn, int isign);
void pad_zeros_to(double *arr, int current_length, int M);
int next_power_of_2(int n);
void convolution(double *x, int K, double *h, double *y);

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
    return 0;
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
    printf("number of DW samples: %d\n", num_samples_input);
     printf("number of IR samples: %d\n", num_samples_IR);

    
    // // read it into an array of shorts
    fread(audio_data, sizeof(short), data_size_input, inputFileStream);
    fread(IR_data, sizeof(short), data_size_IR, IRFileStream);
    // for (int i = 0; i < num_samples_input; i++) {
    //     printf("\n input: %d", audio_data[i]);
    // }
    // for (int i = 0; i < num_samples_IR; i++) {
    //     printf("\n IR: %d", IR_data[i]);
    // }
    printf("\n input: %d", audio_data[80000]);
   
    //both arrays should be prepared for FFT
    //zero padding them to same length so that the length is a power of 2



    //start
    
    int K_input = next_power_of_2(2*num_samples_input); //could be just the max of 2 samples??
    
    double *audioDouble = (double *)calloc((2*K_input),sizeof(double));
    for(int i=0; i< num_samples_input; i++)
    {
        audioDouble[(i<<1)] = audio_data[i]/32768.0;
        audioDouble[(i<<1)+1] = 0.0;
    }
   
    double *IRDouble = (double *)calloc(2*K_input,sizeof(double));
    for(int i=0; i< num_samples_IR; i++)
    {
        IRDouble[(i<<1)] = (double)IR_data[i]/32768.0;
        IRDouble[(i<<1)+1] = 0.0;
    }


    pad_zeros_to(audioDouble, 2*num_samples_input, 2*K_input);

    // for(int f = 0; f<2*K_input; f++){
    //     printf("%f\n", audioDouble[f]);
    // }
    pad_zeros_to(IRDouble, 2*num_samples_IR, K_input*2);
    //Read in data and converting it to doubles from shorts

    four1(audioDouble-1, K_input, 1);
    four1(IRDouble-1, K_input, 1);

    double *y = (double *)calloc(2*K_input,sizeof(double));

    convolution(audioDouble,K_input,IRDouble,y);

    four1(y-1,K_input,-1);
    for(int k = 0, i=0; k<num_output; k++, i += 2)
    {
        y[i] /= (double)K_input;
        y[i+1] /= (double)K_input;
    }

    //normalization:
    double maxVal = y[0];

    for (int i = 1; i < 2*K_input; i++) {
        if (y[i] > maxVal) {
            maxVal = y[i]; // Update max if a larger value is found
        }
    }

     for(int k = 0, i=0; k<num_output; k++, i += 2)
    {
        //printf("scaling by %d",K_input);
        y[i] /= (double)maxVal;
        y[i+1] /= (double)maxVal;
    }

    short data;
    for (int i = 0; i < 2*(num_samples_input+num_samples_IR-1); i+=2) { //only read in 
        data = (short)(y[i]*32768);
        fwrite(&data,sizeof(data),1,outputFileStream);// Convert to short
    }   

    free(audio_data);
    free(IR_data);
    free(audioDouble);
    free(IRDouble);
    fclose(inputFileStream);
    fclose(outputFileStream);
}

float shortToFloat(short s) {
    // Convert to range from -1 to (just below) 1
    return s / 32768.0;//scale down the numbers
}

void pad_zeros_to(double *arr, int current_length, int M) {
    int padding = M - current_length;
    int i = 0;
    for (i = 0; i < padding-1; i+=2) {
        arr[current_length + i] = 0.0;
        arr[current_length + i+1] = 0.0;
    }
    if(i== padding-1)
        arr[padding-1] = padding-1;

}
int next_power_of_2(int n) {
    return pow(2, (int)(log2(n - 1) + 1));
}
void four1(double data[], int nn, int isign)
{
    unsigned long n, mmax, m, j, istep, i;
    double wtemp, wr, wpr, wpi, wi, theta;
    double tempr, tempi;

    n = nn << 1;
    j = 1;

    for (i = 1; i < n; i += 2) {
	if (j > i) {
	    SWAP(data[j], data[i]);
	    SWAP(data[j+1], data[i+1]);
	}
	m = nn;
	while (m >= 2 && j > m) {
	    j -= m;
	    m >>= 1;
	}
	j += m;
    }

    mmax = 2;
    while (n > mmax) {
	istep = mmax << 1;
	theta = isign * (6.28318530717959 / mmax);
	wtemp = sin(0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin(theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 1; m < mmax; m += 2) {
	    for (i = m; i <= n; i += istep) {
		j = i + mmax;
		tempr = wr * data[j] - wi * data[j+1];
		tempi = wr * data[j+1] + wi * data[j];
		data[j] = data[i] - tempr;
		data[j+1] = data[i+1] - tempi;
		data[i] += tempr;
		data[i+1] += tempi;
	    }
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}
	mmax = istep;
    }
    
}

void convolution(double *x, int K, double *h, double *y) {
    // Perform the DFT
    for (int k = 0, nn = 0; k < K; k++, nn += 2)
    {
	    y[nn] = ((x[nn] * h[nn]) - (x[nn+1] * h [nn+1]));
	    y[nn+1] = ((x[nn] * h[nn+1]) + (x[nn+1] * h[nn]));
	}
    
}
