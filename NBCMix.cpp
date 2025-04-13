#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <cmath>
#include <chrono>
#include <cstdint>
#include <cstring>

using namespace std;

class Timer
{
private:
    using clock_t = std::chrono::high_resolution_clock;
    using second_t = std::chrono::duration<double, std::ratio<1> >;

    std::chrono::time_point<clock_t> m_beg;

public:
    Timer() : m_beg(clock_t::now())
    {
    }

    void reset()
    {
        m_beg = clock_t::now();
    }

    double elapsed() const
    {
        return std::chrono::duration_cast<second_t>(clock_t::now() - m_beg).count();
    }
};


#define MAX_CODES 100000000
#define MAX_DIGITS 48

uint32_t maxCode;
int checkSize = 10;
uint64_t bestFullBitsSz = 10000000000000000000ull;
uint8_t bestDigitsSt[MAX_DIGITS];
uint8_t minSt[MAX_DIGITS] = {3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
uint8_t maxSt[MAX_DIGITS] = {8, 8, 8, 8, 8, 8, 4, 4, 4, 4, 4, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3};
uint64_t* PSum;
int32_t idx = 0;
int32_t bits = 0;
uint32_t encoded[MAX_CODES];
uint32_t encodedSize = 1;
uint32_t fileIdx = 0;
uint32_t decoded[MAX_CODES];
uint32_t decodedSize = 0;
uint32_t file[MAX_CODES];
uint32_t fileSize;
uint64_t mask[MAX_DIGITS];
uint64_t pows[MAX_DIGITS];
uint64_t pref[MAX_DIGITS];

//adding bit to the encoded bitstream
void encAdd(uint32_t bit) {
    encoded[idx] |= (bit << (31-bits));
    bits++;
    if (bits == 32) {
        bits = 0;
        idx++;
        encodedSize++;
    }
}

//converting a (st)-ary number cur to a binary representation
void encode(uint32_t cur, uint32_t st) {
    if (fileIdx < checkSize) cout << cur;
    uint32_t l = ceil(log2(st));
    uint32_t x = (1 << l) - st;
    uint32_t f;
    if (cur < x) {
        f = cur;
    } else {
        f = x + (cur-x)/2;
    }
    for (uint32_t i = 0; i < l-1; i++) {
        encAdd((f>>(l-2-i))&1);
    }
    if (cur >= x) {
        encAdd((cur-x)&1);
    }
}

//encode file to encoded bitstream
void encodeFile() {
    //encoded = new uint16_t[minEncodedSize+100];
    for (uint32_t i = 0; i < 10; i++){
        encoded[i] = bestDigitsSt[i];
    }
    idx += 10;
    encodedSize += 10;
    cout << "Codes :       ";
    for (fileIdx = 0; fileIdx < fileSize; fileIdx++) {
        uint32_t cur = file[fileIdx];
        for (int j = 0; j < MAX_DIGITS; j++) {
            uint32_t st = bestDigitsSt[j];
            if (cur == 0) {
                encode(mask[j], st);  //delimiter
                break;
            } else {
                encode((cur-1)%mask[j], st); //next digit
                cur = (cur-1)/mask[j];
            }
        }
        if (fileIdx < checkSize) cout << " ";
    }
    cout << endl;
}

//returns count of numbers from [l, r] in file
uint64_t getSum(uint32_t l, uint32_t r)
{
    uint64_t res = 0;

    res += r > maxCode ? PSum[maxCode] : PSum[r];
    res -= l == 0 ? 0 : PSum[l - 1];
    return res;
}


//best code search finds the best digit sizes and best bases for digits
void BCS(uint8_t dSt[MAX_DIGITS], uint32_t dN, uint64_t fullN,
    uint64_t incN, double cLen, uint64_t fullSz)
{
    if (dN > 12 || fullN > maxCode)
    {
        if (fullN > maxCode)
        {
            dSt[dN] = 3;

            if (fullSz < bestFullBitsSz)
            {
                bestFullBitsSz = fullSz;
                for (uint32_t i = 0; i < MAX_DIGITS; i++)
                {
                    bestDigitsSt[i] = dSt[i];
                }
            }
            return;
        }

        dSt[dN] = 3;
        uint64_t newSz = fullSz + (cLen + log2(3)) * getSum(fullN, fullN + incN - 1);
        BCS(dSt, dN + 1, fullN + incN, incN * 2, cLen + 2, newSz);
    }
    else {
        for(int i = minSt[dN]; i <= maxSt[dN]; i++) {
            dSt[dN] = i;
            uint64_t newSz = fullSz + (cLen + log2(i)) * getSum(fullN, fullN + incN - 1);
            BCS(dSt, dN + 1, fullN + incN, incN * (i-1), cLen + log2(i), newSz);
        }
    }
}

//calculate prefix sums for BCS and masks of received from BCS digits
void preCalc() {
    for (uint32_t i = 0; i < fileSize; i++) {
        maxCode = max(maxCode, file[i]);
    }
    PSum = (uint64_t*)calloc(maxCode+1, sizeof(uint64_t));
    for (uint32_t i = 0; i < fileSize; i++) {
        PSum[file[i]]++;
    }
    for (uint32_t i = 1; i <= maxCode; i++) {
        PSum[i] += PSum[i-1];
    }
    uint8_t digitSt[MAX_DIGITS];
    memset(digitSt, 3, sizeof(uint8_t) * MAX_DIGITS);
    for (uint32_t i = 0; i < MAX_DIGITS; i++) {
        bestDigitsSt[i] = 3;
    }
    BCS(digitSt, 0, 0, 1, 0, 0);

    for (int i = 0; i < MAX_DIGITS; i++) {
        mask[i] = (uint32_t)bestDigitsSt[i]-1;
    }
}


uint32_t decodeIdx = 10;
uint32_t bitPos = 0;

//gets next bit from the encoded bitstream
uint32_t getBit() {
    uint32_t bit = (encoded[decodeIdx] >> (31 - bitPos)) & 1;
    bitPos++;
    if (bitPos == 32) {
        bitPos = 0;
        decodeIdx++;
    }
    //if (decodedSize < checkSize) cout << bit;
    return bit;
};

//decodes next (st)-ary digit in encoded bitstream
uint32_t decode(uint32_t st) {
    int l = ceil(log2(st));
    int x = (1 << l) - st;
    uint32_t f = 0;

    for (int i = 0; i < l - 1; i++) {
        f <<= 1;
        f += getBit();
    }

    if (f >= x) {
        f <<= 1;
        f += getBit();
        f -= x;
    }
    //if (decodedSize < 10) cout << endl << "Decoded : " << f << endl;
    return f;
};


//decodes file from encoded bitstream
void decodeFile() {

    decodedSize = 0;

    while (decodeIdx <= encodedSize) {
        uint32_t cur = 0;
        //decode next BCMix digit
        for (int j = 0; j < MAX_DIGITS; j++) {
            uint32_t st = bestDigitsSt[j];

            uint32_t f = decode(st);
            //if (decodedSize < checkSize) cout << "val: " << val << endl;

            //end of number
            if (f == mask[j]) {
                cur += pref[j];
                //if (decodedSize < 10) cout << "Add : " << pref[j] << endl;
                break;
            }
            cur += f*pows[j];
            //if (decodedSize < 10) cout << "Add : " <<  val*pows[j] << endl;
        }

        //if (decodedSize < checkSize) cout << endl << "Decoded : " << cur << endl;
        decoded[decodedSize++] = cur;
    }


}

const uint8_t blockSz = 10;

struct TableDecode
{
    uint32_t L[1 << blockSz];
    uint8_t n[1 << blockSz];
    uint8_t shift[1 << blockSz];
};

TableDecode ts[3];

uint8_t shift = 0;
uint8_t n = 0;
uint32_t val = 0;
uint32_t bitsBlock = 0;

uint8_t getBitFromBlock() {
    uint8_t res = ((bitsBlock >> (31 - bitsBlock)) & 1);
    bitsBlock <<= 1;
    return res;
}

uint32_t fastDecode(uint32_t st) {
    int l = ceil(log2(st));
    int x = (1 << l) - st;
    uint32_t f = 0;

    for (int i = 0; i < l - 1; i++) {
        f <<= 1;
        f += getBitFromBlock();
        shift += 1;
    }

    if (f >= x) {
        f <<= 1;
        f += getBitFromBlock();
        shift += 1;
        f -= x;
    }
    //if (decodedSize < 10) cout << endl << "Decoded : " << f << endl;
    return f;
};

uint8_t maxSize(uint8_t st) {
    return ceil(log2(st));
}

void buildTables() {
    uint8_t digitLen[3] = {0, 0, 0};
    int j = 0, x = 0;
    for (int i = 0; i < MAX_DIGITS; i++) {
        if (x + maxSize(bestDigitsSt[i]) <= blockSz) {
            digitLen[j]++;
            x += maxSize(bestDigitsSt[i]);
        } else {
            j++;
            x = maxSize(bestDigitsSt[i]);
            if (j == 3) break;
            digitLen[j] = 1;
        }
    }
    for (int i = 0; i < (1 << blockSz); i++) {
        shift = 0;
        n = 0;
        val = 0;
        bitsBlock = i << (32-blockSz);
        for (int j = 0; j < digitLen[0]; j++) {
            uint32_t f = fastDecode(bestDigitsSt[j]);
            if (f == mask[j]) {
                n = 1;
                val += pref[j];
                break;
            }
            val += f*pows[j];
        }
        ts[0].n[i] = n;
        ts[0].L[i] = val;
        ts[0].shift[i] = shift;
    }
    for (int i = 0; i < (1 << blockSz); i++) {
        shift = 0;
        n = 0;
        val = 0;
        bitsBlock = i << (32-blockSz);
        for (int j = digitLen[0]; j < digitLen[0]+digitLen[1]; j++) {
            uint32_t f = fastDecode(bestDigitsSt[j]);
            if (f == mask[j]) {
                n = 1;
                val += pref[j];
                break;
            }
            val += f*pows[j];
        }
        ts[1].n[i] = n;
        ts[1].L[i] = val;
        ts[1].shift[i] = shift;
    }
    for (int i = 0; i < (1 << blockSz); i++) {
        shift = 0;
        n = 0;
        val = 0;
        bitsBlock = i << (32-blockSz);
        for (int j = digitLen[0]+digitLen[1]; j < digitLen[0]+digitLen[1]+digitLen[2]; j++) {
            uint32_t f = fastDecode(bestDigitsSt[j]);
            if (f == mask[j]) {
                n = 1;
                val += pref[j];
                break;
            }
            val += f*pows[j];
        }
        ts[2].n[i] = n;
        ts[2].L[i] = val;
        ts[2].shift[i] = shift;
    }
}

uint32_t nextBlock(size_t bitOffset) {
    size_t wordIndex = bitOffset / 32;
    size_t bitInWord = bitOffset % 32;

    uint64_t value = static_cast<uint64_t>(encoded[wordIndex]) << 32;

    if (bitInWord + blockSz > 32) {
        value |= encoded[wordIndex + 1];
    }

    uint64_t shifted = value << bitInWord;
    uint32_t result = static_cast<uint32_t>(shifted >> (64 - blockSz));
    return result;
}

void fastDecodeFile() {
    decodedSize = 0;
    size_t bitOffset = 10*32;
    while (bitOffset/32 <= encodedSize) {
        uint32_t cur = 0;

        for (int j = 0; j < 3; j++) {
            uint32_t block = nextBlock(bitOffset);
            cur += ts[j].L[block];
            bitOffset += ts[j].shift[block];
            if (ts[j].n[block] == 1) {
                break;
            }
        }
        //if (decodedSize < checkSize) cout << endl << "Decoded : " << cur << endl;
        decoded[decodedSize++] = cur;
    }
}




void check() {
    cout << "Checking..." << endl;
    bool diff = false;
    for (int i = 0; i < decodedSize; i++) {
        if (decoded[i] != file[i]) {
            cout << i << " " << decoded[i] << " " << file[i] << endl;
            diff = true;
        }
    }
    if (diff) {
        cout << "Failed" << endl;
    } else {
        cout << "Ok" << endl;
    }
}

int main() {

    FILE* in = fopen("resources/sonnets.txt.enc", "rb");
    //FILE* in = fopen("resources/data04", "rb");

    fseek(in, 0, SEEK_END);
    fileSize = ftell(in) / sizeof(uint32_t);
    fseek(in, 0, SEEK_SET);
    fread(file, sizeof(uint32_t), fileSize, in);

    fclose(in);

    Timer t0;
    preCalc();
    cout << "PreCalc time : " << t0.elapsed() << endl;

    cout << "Digit base : ";
    for (int i = 0; i < MAX_DIGITS; i++) {
        cout << (int)bestDigitsSt[i] << " ";
    }

    cout << endl;

    cout << "Input file  : ";
    for (uint32_t i = 0; i < checkSize; i++) {
        cout << file[i] << " ";
    }
    cout << endl;

    pows[0] = 1;
    pref[0] = 0;
    for (int i = 1; i < MAX_DIGITS; i++) {
        pows[i] = pows[i-1]*mask[i-1];
        pref[i] = pref[i-1]+pows[i-1];
    }
    cout << endl;

    Timer t1;

    encodeFile();

    cout << "Encode time: " << t1.elapsed() << endl;


    cout << "Encoded file: ";
    for (uint32_t i = 10; i < checkSize+10; i++) {
        for (int j = 0; j < 32; j++) {
            cout << ((encoded[i]&(1<<31-j))>>(31-j));
        }
    }
    cout << endl;

    Timer t5;

    decodeFile();

    cout << "Decode time: " << t5.elapsed() << endl;

    buildTables();

    double time = 0;
    for (int i = 0; i < 10; i++) {
        Timer t2;

        fastDecodeFile();

        time += t2.elapsed();
    }
    cout << "Fast decode time: " << time/10 << endl;
    cout << "Decoded file: ";
    for (uint32_t i = 0; i < checkSize; i++) {
        cout << decoded[i] << " ";
    }
    cout << endl;

    cout << "Input file size (bytes): " << fileSize*4 << "\n" << "Encoded file size (bytes): " << encodedSize*4-20 << endl;
    cout << "BestEncodedSize (bytes): " << bestFullBitsSz/8 << endl;

    cout << "Decoded file size (bytes) : " << decodedSize*4 << endl;
    check();

    FILE* out = fopen("decoded", "wb");
    fwrite(decoded, sizeof(uint32_t), decodedSize, out);
    fclose(out);


    return 0;
}

//bible.txt:
//884186 BC
//875428 NBC
//3064432

//harry_potter1.txt:
//98606 BC
//97776 NBC
//310396

//shakespeare.txt
//1224619 BC
//1219672 NBC
//3597084

//sonnets.txt
//22566 BC
//22396 NBC
//70944

