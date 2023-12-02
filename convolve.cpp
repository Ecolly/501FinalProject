/*
Create an initial version of your program where the convolution is implemented directly in the
time domain (i.e., use the input-side convolution algorithm found on p. 112-115 in the Smith
text). You will find this version of your program quite slow. Measure the run-time performance
of your program using a dry recording that is at least thirty seconds long, and an impulse response
that is at least 2 seconds long. You will reuse these same inputs for timing measurements after
each optimization that you do in the later stages of the assignment. There are many suitable dry
recordings and impulse responses available on the Internet as well as on the course D2L site. You
can use utility programs such as sox to convert sound files of different types (e.g. .aiff or .snd) to
the .wav format.
Although the program may be implemented in any programming language, it would be best to
use a language supported by the gcc compiler, since it is available on our servers, or can be
installed on a home machine. The gcc compiler supports the C, C++, and Objective-C languages*/

#include <iostream>
#include <fstream>
#include <string>
#include <vector>


struct Wave {
    char chunkID[4];
    unsigned int chunkSize;
    char format[4];
    char subchunk1ID[4];
    unsigned int subchunk1Size;
    unsigned short audioFormat;
    unsigned short numChannels;
    unsigned int sampleRate;
    unsigned int byteRate;
    unsigned short blockAlign;
    unsigned short bitsPerSample;
    char subchunk2ID[4];
    unsigned int subchunk2Size;
};

void readWave(const std::string &inputFileName, const std::string &IRFileName, const std::string &outputFileName) {
    std::ifstream inputFile(inputFileName, std::ios::binary);
    std::ifstream IRFile(IRFileName, std::ios::binary);
    std::ofstream outputFile(outputFileName, std::ios::binary);
    

    //error checking
    if(!inputFile.is_open()||!IRFile.is_open()){
        std::cerr << "Unable to open file" << std::endl;
        return;
    }
    //read in the headers
    Wave header_input;
    Wave header_IR;
    inputFile.read((char*)&header_input, sizeof(header_input));
    IRFile.read((char*)&header_IR, sizeof(header_IR));
    outputFile.write((char*)&header_input, sizeof(header_input));



    //checking hex
    inputFile.seekg(0, std::ios::end);
    size_t fileSize = inputFile.tellg();
    inputFile.seekg(0, std::ios::beg);

    std::vector<char> buffer(sizeof(Wave));
    inputFile.read(buffer.data(), sizeof(Wave));
    for (unsigned char byte : buffer) {
        std::cout << std::hex << static_cast<int>(byte) << " ";
    }


    // std::cout << "Chunk ID: " << std::string(header_input.chunkID, 4) << "\n";
    // std::cout << "Chunk Size: " << header_input.chunkSize << "\n";
    // std::cout << "Format: " << std::string(header_input.format, 4) << "\n";
    // std::cout << "Subchunk1 ID: " << std::string(header_input.subchunk1ID, 4) << "\n";
    // std::cout << "Subchunk1 Size: " << header_input.subchunk1Size << "\n";

    //handle extra bytes in subchunk 1:

    if (header_input.subchunk1Size > 16) {
        int skipBytes = header_input.subchunk1Size - 16; //to see how many extrabytes there are
        std::cout << "Subchunk1 is larger than standard, skipping " << skipBytes << " extra bytes." << std::endl;
        //std::cout << std::int64_t<< skipBytes;
        std::vector<char> extraData(skipBytes); //make a vector that is the size of the number of byte skipped
        inputFile.read(extraData.data(), skipBytes); //read the bytes
        // outputFile.write(extraData.data(), skipBytes);
    }

    if (header_IR.subchunk1Size > 16) {
        std::cout << "Subchunk1 for IR is larger than standard, skipping extra bytes." << std::endl;
        int skipBytes = header_IR.subchunk1Size - 16;
        std::vector<char> extraData(skipBytes);
        inputFile.read(extraData.data(), skipBytes);
        // outputFile.write(extraData.data(), skipBytes);
    }

    char subchunk2_id[4];
    int subchunk2_size;

    //Reading from inputFileStream
    inputFile.read((char*)&subchunk2_id, sizeof(subchunk2_id));
    inputFile.read((char*)&subchunk2_size, sizeof(subchunk2_size));

    // Writing to outputFileStream
    outputFile.write((const char*)&subchunk2_id, sizeof(subchunk2_id));
    outputFile.write((const char*)&subchunk2_size, sizeof(subchunk2_size));

    int num_samples = subchunk2_size / (header_input.bitsPerSample / 8);
    size_t data_size = subchunk2_size;
    // Use a std::vector to handle the audio data
    std::vector<short> audio_data(data_size / sizeof(short));

    


    std::cout << "Audio Format: " << header_input.audioFormat << "\n";
    std::cout << "Number of Channels: " << header_input.numChannels << "\n";
    std::cout << "Sample Rate: " << header_input.sampleRate << "\n";
    std::cout << "Byte Rate: " << header_input.byteRate << "\n";
    std::cout << "Block Align: " << header_input.blockAlign << "\n";
    std::cout << "Bits per Sample: " << header_input.bitsPerSample << "\n";

    std::cout << "Subchunk2 ID: " << std::string(header_input.subchunk2ID, 4) << "\n";
    std::cout << "Subchunk2 ID: " << header_input.subchunk2Size << "\n";
    
    std::cout << "Subchunk2 ID: " << std::string(subchunk2_id, 4) << "\n";
    std::cout << "Subchunk2 Size: " << subchunk2_size << "\n";
    // You can add more logic here to handle uncommon byte sizes

    inputFile.close();
}

//convolution code

void convolve(const std::vector<float> &x, const std::vector<float> &h, std::vector<float> &y) {
    // Clear output buffer y
    std::fill(y.begin(), y.end(), 0.0f);

    // Ensure y has enough space to store the result
    y.resize(x.size() + h.size() - 1, 0.0f);

    // Outer loop: process each input value x[n] in turn
    for (size_t n = 0; n < x.size(); n++) {
        // Inner loop: process x[n] with each sample of h[m]
        for (size_t m = 0; m < h.size(); m++) {
            y[n + m] += x[n] * h[m];
        }
    }
}

int main(int argc, char *argv[]) {
     if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " <inputfile> <IRfile> <outputfile>" << std::endl;
        return 1;
    }

    std::string inputFile = argv[1];
    std::string irFile = argv[2];
    std::string outputFile = argv[3];
    readWave(inputFile, irFile, outputFile);

    // Output the header_input information
    
    return 0;
}

