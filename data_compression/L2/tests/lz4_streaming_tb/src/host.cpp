/*
 * Copyright 2019 Xilinx, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "xil_lz4_streaming.hpp"
#include <fstream>
#include <vector>
#include "cmdlineparser.h"

static uint64_t getFileSize(std::ifstream& file) {
    file.seekg(0, file.end);
    uint64_t file_size = file.tellg();
    file.seekg(0, file.beg);
    return file_size;
}

void xilCompressTop(std::string& compress_mod, uint32_t block_size, std::string& compress_bin) {
    // Xilinx LZ4 object
    xfLz4Streaming xlz;

    // LZ4 Compression Binary Name
    std::string binaryFileName;
    binaryFileName = compress_bin;
    xlz.m_bin_flow = 1;
    // Create xfLz4Streaming object
    xlz.init(binaryFileName);
#ifdef VERBOSE
    std::cout << "\n";
    std::cout << "E2E(MBps)\tKT(MBps)\tLZ4_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";
#endif

    std::ifstream inFile(compress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }
    uint64_t input_size = getFileSize(inFile);
    inFile.close();

    std::string lz_compress_in = compress_mod;
    std::string lz_compress_out = compress_mod;
    lz_compress_out = lz_compress_out + ".lz4";

    // Update class membery with block_size
    xlz.m_block_size_in_kb = block_size;

    // 0 means Xilinx flow
    xlz.m_switch_flow = 0;

#ifdef EVENT_PROFILE
    auto total_start = std::chrono::high_resolution_clock::now();
#endif
    // Call LZ4 compression
    uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size);
#ifdef EVENT_PROFILE
    auto total_end = std::chrono::high_resolution_clock::now();
    auto total_time_ns = std::chrono::duration<double, std::nano>(total_end - total_start);
#endif

#ifdef VERBOSE
    std::cout.precision(3);
    std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << (double)input_size / 1000000 << "\t\t\t"
              << lz_compress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_compress_out.c_str() << std::endl;
    std::cout << "Compressed file size: " << enbytes << std::endl;
#endif

#ifdef EVENT_PROFILE
    std::cout << "Total Time (milli sec): " << total_time_ns.count() / 1000000 << std::endl;
#endif

    xlz.release();
}

void xilValidate(std::string& file_list, std::string& ext) {
    std::cout << "\n";
    std::cout << "Status\t\tFile Name" << std::endl;
    std::cout << "\n";

    std::ifstream infilelist_val(file_list.c_str());
    std::string line_val;

    while (std::getline(infilelist_val, line_val)) {
        std::string line_in = line_val;
        std::string line_out = line_in + ext;

        int ret = 0;
        // Validate input and output files
        ret = validate(line_in, line_out);
        if (ret == 0) {
            std::cout << (ret ? "FAILED\t" : "PASSED\t") << "\t" << line_in << std::endl;
        } else {
            std::cout << "Validation Failed" << line_out.c_str() << std::endl;
            //        exit(1);
        }
    }
}

void xilCompressDecompressList(std::string& file_list,
                               std::string& ext1,
                               std::string& ext2,
                               bool c_flow,
                               bool d_flow,
                               uint32_t block_size,
                               std::string& compress_bin,
                               std::string& decompress_bin) {
    // Compression
    // LZ4 Compression Binary Name
    std::string binaryFileName;
    binaryFileName = compress_bin;

    // Create xfLz4Streaming object
    xfLz4Streaming xlz;

    if (c_flow == 0) {
        std::cout << "\n";
        xlz.m_bin_flow = 1;
        xlz.init(binaryFileName);
    }
    std::cout << "\n";

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (c_flow == 0)
        std::cout << "                     Xilinx Compress                          " << std::endl;
    else
        std::cout << "                     Standard Compress                        " << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;

    if (c_flow == 0) {
        std::cout << "\n";
        std::cout << "E2E(MBps)\tKT(MBps)\tLZ4_CR\t\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    }

    std::ifstream infilelist(file_list.c_str());
    std::string line;

    // Compress list of files
    // This loop does LZ4 compression on list
    // of files.
    while (std::getline(infilelist, line)) {
        std::ifstream inFile(line.c_str(), std::ifstream::binary);
        if (!inFile) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = getFileSize(inFile);
        inFile.close();

        std::string lz_compress_in = line;
        std::string lz_compress_out = line;
        lz_compress_out = lz_compress_out + ext1;

        xlz.m_block_size_in_kb = block_size;
        xlz.m_switch_flow = c_flow;

        // Call LZ4 compression
        uint64_t enbytes = xlz.compressFile(lz_compress_in, lz_compress_out, input_size);
        if (c_flow == 0) {
            std::cout << "\t\t" << (double)input_size / enbytes << "\t\t" << (double)input_size / 1000000 << "\t\t\t"
                      << lz_compress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t\t" << lz_compress_in << std::endl;
        }
    }

    if (c_flow == 0) {
        xlz.release();
        std::cout << "Kernel Released" << std::endl;
    }

    // De-Compression
    // LZ4 Decompression Binary Name
    std::string binaryFileName_decompress;
    binaryFileName_decompress = decompress_bin;

    // Xilinx LZ4 object
    if (d_flow == 0) {
        // init xfLz4Streaming object
        std::cout << "\n";
        xlz.m_bin_flow = 0;
        xlz.init(binaryFileName_decompress);
    }

    std::ifstream infilelist_dec(file_list.c_str());
    std::string line_dec;

    std::cout << "\n";
    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0)
        std::cout << "                     Xilinx De-Compress                       " << std::endl;
    else
        std::cout << "                     Standard De-Compress                     " << std::endl;

    std::cout << "--------------------------------------------------------------" << std::endl;
    if (d_flow == 0) {
        std::cout << "\n";
        std::cout << "E2E(MBps)\tKT(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
        std::cout << "\n";
    } else {
        std::cout << "\n";
        std::cout << "File Size(MB)\tFile Name" << std::endl;
        std::cout << "\n";
    }

    // Decompress list of files
    // This loop does LZ4 decompress on list
    // of files.
    while (std::getline(infilelist_dec, line_dec)) {
        std::string file_line = line_dec;
        file_line = file_line + ext2;

        std::ifstream inFile_dec(file_line.c_str(), std::ifstream::binary);
        if (!inFile_dec) {
            std::cout << "Unable to open file";
            exit(1);
        }

        uint64_t input_size = getFileSize(inFile_dec);
        inFile_dec.close();

        std::string lz_decompress_in = file_line;
        std::string lz_decompress_out = file_line;
        lz_decompress_out = lz_decompress_out + ".orig";

        // Call LZ4 decompression
        xlz.m_switch_flow = d_flow;
        xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size);

        if (d_flow == 0) {
            std::cout << "\t\t" << (double)input_size / 1000000 << "\t\t" << lz_decompress_in << std::endl;
        } else {
            std::cout << std::fixed << std::setprecision(3);
            std::cout << (double)input_size / 1000000 << "\t\t" << lz_decompress_in << std::endl;
        }
    } // While loop ends

    if (d_flow == 0) {
        xlz.release();
    }
}
void xilBatchVerify(
    std::string& file_list, int f, uint32_t block_size, std::string& compress_bin, std::string& decompress_bin) {
    if (f == 0) { // All flows are tested (Xilinx, Standard)

        // Xilinx LZ4 flow

        // Flow : Xilinx LZ4 Compress vs Xilinx LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2xd.lz4";
            std::string ext2 = ".xe2xd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Xilinx LZ4 Decompress           "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.lz4.orig";
            xilValidate(file_list, ext3);
        }

        // Standard LZ4 flow

        // Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext2 = ".xe2sd.lz4";
            std::string ext1 = ".xe2sd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 1, block_size, compress_bin, decompress_bin);

            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Standard LZ4 Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress

        { // Start of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

            // Standard LZ4 compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 1, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx LZ4 Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.lz4.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

    }                  // Flow = 0 ends here
    else if (f == 1) { // Only Xilinx flows are tested

        // Flow : Xilinx LZ4 Compress vs Xilinx LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2xd.lz4";
            std::string ext2 = ".xe2xd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Xilinx LZ4 Decompress           "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2xd.lz4.orig";
            xilValidate(file_list, ext3);
        }

    } // Flow = 1 ends here
    else if (f == 2) {
        // Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress
        {
            // Xilinx LZ4 compression
            std::string ext1 = ".xe2sd.lz4";
            std::string ext2 = ".xe2sd.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 0, 1, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Xilinx LZ4 Compress vs Standard LZ4 Decompress        "
                      << std::endl;
            std::cout << "---------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext3 = ".xe2sd";
            xilValidate(file_list, ext3);

        } // End of Flow : Xilinx LZ4 Compress vs Standard LZ4 Decompress

    } // Flow = 2 ends here
    else if (f == 3) {
        { // Start of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress

            // Standard LZ4 compression
            std::string ext1 = ".se2xd";
            std::string ext2 = ".std.lz4";
            xilCompressDecompressList(file_list, ext1, ext2, 1, 0, block_size, compress_bin, decompress_bin);

            // Validate
            std::cout << "\n";
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::cout << "                       Validate: Standard Compress vs Xilinx LZ4 Decompress             "
                      << std::endl;
            std::cout << "----------------------------------------------------------------------------------------"
                      << std::endl;
            std::string ext = ".std.lz4.orig";
            xilValidate(file_list, ext);

        } // End of Flow : Standard LZ4 Compress vs Xilinx LZ4 Decompress
    }     // Flow = 3 ends here
    else {
        std::cout << "-x option is wrong" << f << std::endl;
        std::cout << "-x - 0 all features" << std::endl;
        std::cout << "-x - 1 Xilinx (C/D)" << std::endl;
        std::cout << "-x - 2 Xilinx Compress vs Standard Decompress" << std::endl;
        std::cout << "-x - 3 Standard Compress vs Xilinx Decompress" << std::endl;
    }
}

void xilDecompressTop(std::string& decompress_mod, std::string& decompress_bin) {
    // Create xfLz4Streaming object
    xfLz4Streaming xlz;

    // LZ4 Decompression Binary Name
    std::string binaryFileName;
    binaryFileName = decompress_bin;
    xlz.m_bin_flow = 0;
    xlz.init(binaryFileName);

#ifdef VERBOSE
    std::cout << "\n";
    std::cout << "E2E(MBps)\tKT(MBps)\tFile Size(MB)\t\tFile Name" << std::endl;
    std::cout << "\n";
#endif

    std::ifstream inFile(decompress_mod.c_str(), std::ifstream::binary);
    if (!inFile) {
        std::cout << "Unable to open file";
        exit(1);
    }

    uint64_t input_size = getFileSize(inFile);
    inFile.close();

    string lz_decompress_in = decompress_mod;
    string lz_decompress_out = decompress_mod;
    lz_decompress_out = lz_decompress_out + ".orig";

    xlz.m_switch_flow = 0;

    // Call LZ4 decompression
    xlz.decompressFile(lz_decompress_in, lz_decompress_out, input_size);
#ifdef VERBOSE
    std::cout << "\t\t" << (double)input_size / 1000000 << "\t\t\t" << lz_decompress_in << std::endl;
    std::cout << "\n";
    std::cout << "Output Location: " << lz_decompress_out.c_str() << std::endl;
#endif
    xlz.release();
}

int main(int argc, char* argv[]) {
    sda::utils::CmdLineParser parser;
    parser.addSwitch("--compress_xclbin", "-cx", "Compress XCLBIN", "compress");
    parser.addSwitch("--decompress_xclbin", "-dx", "DeCompress XCLBIN", "decompress");
    parser.addSwitch("--compress", "-c", "Compress", "");
    parser.addSwitch("--file_list", "-l", "List of Input Files", "");
    parser.addSwitch("--decompress", "-d", "Decompress", "");
    parser.addSwitch("--block_size", "-B", "Compress Block Size [0-64: 1-256: 2-1024: 3-4096]", "0");
    parser.addSwitch("--flow", "-x", "Validation [0-All: 1-XcXd: 2-XcSd: 3-ScXd]", "1");
    parser.parse(argc, argv);

    std::string compress_bin = parser.value("compress_xclbin");
    std::string decompress_bin = parser.value("decompress_xclbin");
    std::string compress_mod = parser.value("compress");
    std::string filelist = parser.value("file_list");
    std::string decompress_mod = parser.value("decompress");
    std::string flow = parser.value("flow");
    std::string block_size = parser.value("block_size");

    uint32_t bSize = 0;
    // Block Size
    if (!(block_size.empty())) {
        bSize = atoi(block_size.c_str());

        switch (bSize) {
            case 0:
                bSize = 64;
                break;
            case 1:
                bSize = 256;
                break;
            case 2:
                bSize = 1024;
                break;
            case 3:
                bSize = 4096;
                break;
            default:
                std::cout << "Invalid Block Size provided" << std::endl;
                parser.printHelp();
                exit(1);
        }
    } else {
        // Default Block Size - 64KB
        bSize = BLOCK_SIZE_IN_KB;
    }

    int fopt = 0;
    if (!(flow.empty()))
        fopt = atoi(flow.c_str());
    else
        fopt = 1;

    // "-c" - Compress Mode
    if (!compress_mod.empty()) xilCompressTop(compress_mod, bSize, compress_bin);

    // "-d" Decompress Mode
    if (!decompress_mod.empty()) xilDecompressTop(decompress_mod, decompress_bin);

    // "-l" List of Files
    if (!filelist.empty()) {
        if (fopt == 0 || fopt == 2 || fopt == 3) {
            std::cout << "\n" << std::endl;
            std::cout << "Validation flows with Standard LZ4 ";
            std::cout << "requires executable" << std::endl;
            std::cout << "Please build LZ4 executable ";
            std::cout << "from following source ";
            std::cout << "https://github.com/lz4/lz4.git" << std::endl;
        }
        xilBatchVerify(filelist, fopt, bSize, compress_bin, decompress_bin);
    }
}